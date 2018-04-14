#########################################################################################
#
# Minimal cmake file to support other platforms or build systems.
#
# This cmake file is only used for compiling and linking. It does not find out
# system configuration and it does not install.
#
# For Linux use the Makefile in same directory than this file.
# The Makefile will automatically setup everything for linux, uses cmake
# for compiling/linking and also has an install target.
#
#########################################################################################
#
# Use the following variables to configure compiling & linking:
#

SET ( LUAJACK_COMPILE_FLAGS  CACHE STRING "" )
SET ( LUAJACK_LINK_FLAGS     CACHE STRING "" )

#########################################################################################

PROJECT ( luajack C )
CMAKE_MINIMUM_REQUIRED ( VERSION 2.8 )

SET ( CMAKE_C_FLAGS  
      "${CMAKE_C_FLAGS} ${LUAJACK_COMPILE_FLAGS}" )

SET ( CMAKE_SHARED_LINKER_FLAGS 
      "${CMAKE_SHARED_LINKER_FLAGS} ${LUAJACK_LINK_FLAGS}")

ADD_LIBRARY (
	luajack SHARED
	src/util.c
	src/client.c  src/client_util.c
	src/port.c    src/port_util.c
	src/rbuf.c    src/rbuf_util.c
	src/process.c src/process_util.c
	src/thread.c  src/thread_util.c
	src/buffer.c
	src/main.c
)

