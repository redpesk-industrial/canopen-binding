message(STATUS "Custom options: 00-debian-config.cmake --")
add_definitions(-DSUSE_LUA_INCDIR)
list(APPEND PKG_REQUIRED_LIST lua53-c++>=5.3)
