message(STATUS "Custom options: 00-fedora-config.cmake --")
add_definitions(-DSUSE_LUA_INCDIR)
list(APPEND PKG_REQUIRED_LIST lua>=5.3)

# LibModbus is not part of standard Linux Distro (https://libmodbus.org/)
#set(ENV{PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH}:/opt/AGL/lib64/pkgconfig:/opt/libmodbus-3.1.6/lib/pkgconfig:")

# Remeber sharelib path to simplify test & debug
#set(BINDINGS_LINK_FLAG "-Xlinker -rpath=/opt/libmodbus-3.1.6/lib")
