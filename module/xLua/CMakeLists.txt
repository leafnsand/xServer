aux_source_directory(. xLua_src)
add_library(xLua SHARED ${xLua_src})
set_target_properties(xLua PROPERTIES PREFIX "")
target_link_libraries(xLua lua xServer)