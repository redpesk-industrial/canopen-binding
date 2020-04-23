message(STATUS "Custom options: 00-fedora-config.cmake --")
add_definitions(-DSUSE_LUA_INCDIR)
list(APPEND PKG_REQUIRED_LIST lua>=5.3)

