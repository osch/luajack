#####################################################################################
#
# Use this Makefile for building under linux.
#
# This Makefile will configure everything auomatically and uses 
# cmake for compiling and linking.
#
#####################################################################################
#
# Variables
#
# These variables can be overriden by make invocation, e.g: make VERBOSE=1 CMAKE_DEFS="..."
# Overriding is also possible in the included file sandbox.mk

PROJ_DIR   = $(shell pwd)
BUILD_DIR  = $(PROJ_DIR)/build

MAKE       = make
MAKE_FLAGS = --no-print-directory

CMAKE       = cmake
CMAKE_FLAGS = 

C_FLAGS   = -fdiagnostics-color -Wall

VERBOSE = 

PKG_CONFIG           = pkg-config
LUA_PKG_CONFIG_NAME  = lua
JACK_PKG_CONFIG_NAME = jack

define CMAKE_DEFS
    -D LUAJACK_COMPILE_FLAGS="$1" \
    -D LUAJACK_LINK_FLAGS="$2" \
    -D CMAKE_C_FLAGS="$(C_FLAGS)"
endef

#####################################################################################

-include $(PROJ_DIR)/sandbox.mk

.PHONY: default init build clean
default: build

init: $(BUILD_DIR)/Makefile

$(BUILD_DIR)/Makefile: $(PROJ_DIR)/Makefile \
                       $(PROJ_DIR)/CMakeLists.txt \
                       $(PROJ_DIR)/sandbox.mk
	@mkdir -p $(BUILD_DIR) && \
	cd $(BUILD_DIR) && \
	compile_flags="`$(PKG_CONFIG) --cflags $(LUA_PKG_CONFIG_NAME)` \
	               `$(PKG_CONFIG) --cflags $(JACK_PKG_CONFIG_NAME)`" && \
	link_flags="`$(PKG_CONFIG) --libs $(LUA_PKG_CONFIG_NAME)` \
	            `$(PKG_CONFIG) --libs $(JACK_PKG_CONFIG_NAME)`" && \
	$(CMAKE) $(CMAKE_FLAGS) $(call CMAKE_DEFS, $$compile_flags, $$link_flags) $(PROJ_DIR)

$(PROJ_DIR)/sandbox.mk:
	@ echo "**** initializing sandbox.mk for local build definitions..." && \
	  echo "# Here you can override variables from the Makefile" > "$@"

build: init
	@cd $(BUILD_DIR) && \
	$(MAKE) $(MAKE_FLAGS) VERBOSE=$(VERBOSE) && \
	ln -sf libluajack.so luajack.so

clean:
	rm -rf $(BUILD_DIR)

