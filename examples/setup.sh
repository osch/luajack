# Setup script for invoking examples from development sandbox.
# Source this into interactive shell by invoking ". setup.sh" from this directory

THIS_DIR=$(pwd)

export LUAJACK_DIR=$(cd "$THIS_DIR"/..; pwd)

if [ ! -e "$LUAJACK_DIR/examples/setup.sh" -o ! -e "$LUAJACK_DIR/src/main.c" ]; then

    echo '**** ERROR: ". setup.sh" must be invoked from "examples" directory ***'

else

    echo "Setting up lua 5.3 paths for: $LUAJACK_DIR"
    
    export LUA_CPATH_5_3="$LUAJACK_DIR/build/?.so;$LUA_CPATH_5_3"
    export LUA_PATH_5_3="$LUAJACK_DIR/?.lua;$LUAJACK_DIR/?/init.lua;$LUA_PATH_5_3"
fi
