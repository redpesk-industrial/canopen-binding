message(STATUS "Custom options: 00-debian-config.cmake --")
add_definitions(-DSUSE_LUA_INCDIR)
list(APPEND PKG_REQUIRED_LIST lua53-c++>=5.3)

# liblely-core is not part of standard Linux Distro (https://liblely-core.org/)
#set(ENV{PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH}:/opt/AGL/lib64/pkgconfig:/opt/liblely-coapp/pkgconfig:")

# Remeber sharelib path to simplify test & debug
#set(BINDINGS_LINK_FLAG "-Xlinker -rpath=/opt/libmodbus-3.1.6/lib64")
