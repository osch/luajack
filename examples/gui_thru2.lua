---------------------------------------------------------------------------------------------
--
-- This examples uses LuaLanes for handling jack clients in a separate thread
--
---------------------------------------------------------------------------------------------

local lanes = require"lanes".configure()
local fl = require("moonfltk")
local timer = require("moonfltk.timer")

timer.init()

local jack = require("luajack")

jack.verbose("on")

local linda = lanes.linda()

---------------------------------------------------------------------------------------------
-- Thread for handling the jack clients
local function jack_main_thread_code()

    local lanes = require"lanes"
    local jack  = require"luajack"

    local PROCESS = [[
        local client, port_in, port_out, rbuf = ...
    
        local mute = false
    
        local function process(nframes)
            local tag, data = jack.ringbuffer_read(rbuf)
            if tag then 
               if     tag==1 then mute=true       -- mute
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
    
    local clients = {}
    local cmds = 
    {
        startNewClient = function(clientName)
            local rbuf     = jack.ringbuffer(1000)
            local client   = jack.client_open(clientName, { no_start_server=true })
            local port_in  = client:input_audio_port("in")
            local port_out = client:output_audio_port("out")
            
            client:process_load(PROCESS, client, port_in, port_out, rbuf)
            client:activate()
    
            local newClient = {
                rbuf   = rbuf,
                client = client
            }
            clients[clientName] = newClient
        end,
        closeClient = function(clientName)
            clients[clientName].client:close()
            clients[clientName] = nil
        end,
        activateClient =  function(clientName)
            clients[clientName].client:activate()
        end,
        deactivateClient = function(clientName)
            clients[clientName].client:deactivate()
        end,
        muteClient = function(clientName)
            clients[clientName].rbuf:write(1)
        end,
        unmuteClient = function(clientName)
            clients[clientName].rbuf:write(2)
        end
    }
    repeat
        local k, v = linda:receive("cmd", "quit")
        print("jackThread: received from gui:", k, table.unpack(v))
        if k == "cmd" then
            local cmd = cmds[v[1]]
            if cmd then
                cmd(table.unpack(v, 2))
            else
                error("*** error: invalid command received from linda "..tostring(v[1]))
            end
        end
    until k == "quit"
end
---------------------------------------------------------------------------------------------

local jackThread = lanes.gen("*", jack_main_thread_code)()

local clientIdCounter = 0
local startNewClientGui

startNewClientGui = function()

    clientIdCounter = clientIdCounter + 1
    
    local clientId = clientIdCounter
    local clientName = "gui_thru - client-" .. clientId



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
    
    linda:send("cmd", {"startNewClient", clientName})
    
    local function mute_cb(b) -- callback for the 'mute' button
        if b:value() then
            linda:send("cmd", {"muteClient", clientName})
        else
            linda:send("cmd", {"unmuteClient", clientName})
        end
    end

    local function activate_cb(b)
        if b:value() then
            b:label("client is active")
            linda:send("cmd", {"activateClient", clientName})
        else
            b:label("client is not active")
            linda:send("cmd", {"deactivateClient", clientName})
        end
    end
    
    muteButton:callback(mute_cb)
    muteButton:labelfont(fl.BOLD + fl.ITALIC)
    muteButton:labelsize(20)
    muteButton:labeltype('shadow')
    
    activateButton:callback(activate_cb)
    activateButton:value(true)
    
    newClientButton:callback(startNewClientGui)
    gcButton:callback(function() collectgarbage() end)
    
    win:callback(function(w)
        w:hide()
        fl.unreference(w)
        linda:send("cmd", {"closeClient", clientName})
    end)
    
    win:done()
    win:show()
end

startNewClientGui()

-- GUI thread main loop:
while fl.wait() do 
    if jackThread.status == "error" then
        error("error in jack thread", jackThread[1])
    end
end
print("Finished.")
