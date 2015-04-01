aux_source_directory(. xLogger_src)
add_library(xLogger SHARED ${xLogger_src})
target_link_libraries(xLogger xServer)
set_target_properties(xLogger PROPERTIES PREFIX "")