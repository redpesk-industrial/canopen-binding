message(STATUS "Custom options: 00-opensuse-config.cmake --")
add_definitions(-DSUSE_LUA_INCDIR)
list(APPEND PKG_REQUIRED_LIST lua>=5.3)

