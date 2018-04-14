---------------------------------------------------------------------------------------------
--
-- This examples handles jack clients from the gui main thread.
--
---------------------------------------------------------------------------------------------

local fl = require("moonfltk")
local timer = require("moonfltk.timer")

timer.init()

local jack = require("luajack")

jack.verbose("on")

local function newGcSentinel(name) -- for debugging
    local gcSentinel = {}
    setmetatable(gcSentinel, { 
        __gc = function() print("*** lua objects for '"..name.."' are garbage collected ***") end 
    })
    return gcSentinel
end


local PROCESS = [[
    local client, port_in, port_out, mute_rbuf = ...

    local mute = false

    local function process(nframes)
        local tag, data = jack.ringbuffer_read(mute_rbuf)
        if tag then 
           if     tag==1 then mute=true        -- mute
           elseif tag==2 then mute=false end  -- unmute
        end
        if not mute then
            port_out:copy_from(port_in)
        else
            port_out:clear()
        end
    end
    
    client:process_callback(process)

]]

local clientIdCounter = 0
local startNewClient

startNewClient = function()

    clientIdCounter = clientIdCounter + 1
    
    local clientId = clientIdCounter
    local clientName = "gui_thru - client-" .. clientId



    local mute_rbuf = jack.ringbuffer(1000)

    local W, H = 430, 200
    local dx, dy = 10, 10
    local nlines = 2
    local lineH = (H - 2*dy - nlines*dy) / nlines
    
    local win = fl.window(W, H, clientName)
    
    local x, y = dx, dy
    
    local line1 = fl.pack(x, y, W - 2*dy, lineH)
    line1:type("horizontal") line1:spacing(dy)
    local muteButton      = fl.toggle_button(0, 0, 200, 10, "MUTE")
    local activateButton  = fl.toggle_button(0, 0, 200, 10, "client is active")
    line1:done()
    
    y = y + dy + lineH
    local line2 = fl.pack(x, y, W - 2*dy, lineH)
    line2:type("horizontal") line2:spacing(dy)
    local newClientButton = fl.button(0, 0, 200, 10, "new Client")
    -- TODO: error handling for closed client
    --local closeButton     = fl.button(0, 0, (200 - dy)/2, 10, "close")
    local gcButton        = fl.button(0, 0, (200 - dy)/2, 10, "collect\ngarbage")
    line2:done()
    
    
    local client = jack.client_open(clientName, { no_start_server=true })

    local port_in  = client:input_audio_port("in")
    local port_out = client:output_audio_port("out")
    
    client:process_load(PROCESS, client, port_in, port_out, mute_rbuf)
    client:activate()

    local function mute_cb(b) -- callback for the 'mute' button
       local gcs2 = gcs1
       local tag = b:value() and 1 or 2 -- mute/unmute
       mute_rbuf:write(tag)
    end

    local gcs1 = newGcSentinel(clientName)
    
    local function activate_cb(b)
        local gcs2 = gcs1
        if b:value() then
            b:label("client is active")
            client:activate()
        else
            b:label("client is not active")
            client:deactivate()
        end
    end
    
    muteButton:callback(mute_cb)
    muteButton:labelfont(fl.BOLD + fl.ITALIC)
    muteButton:labelsize(20)
    muteButton:labeltype('shadow')
    
    activateButton:callback(activate_cb)
    activateButton:value(true)
    
    newClientButton:callback(startNewClient)
    gcButton:callback(function() local gcs2 = gcs1; collectgarbage() end)
    
    --[[ TODO: error handling for closed client
    closeButton:callback(function()
        local gcs2 = gcs1;
        client:close()
    end)
    --]]
    
    win:callback(function(w)
        local gcs2 = gcs1
        w:hide()
        fl.unreference(w)
        --client:close() -- if not invoked, client will be closed by gc
    end)
    
    win:done()
    win:show()
end

startNewClient()

-- GUI thread main loop:
while fl.wait() do 
end
print("Finished.")
