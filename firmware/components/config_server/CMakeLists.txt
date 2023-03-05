idf_component_register(SRCS "config_server.cpp"
                    INCLUDE_DIRS "include"
                    REQUIRES hypnos_config wifi eightsleep
                    PRIV_REQUIRES esp_http_server arduinojson log)

component_compile_options("-Wno-missing-field-initializers")

# Magic to include the config webpage.
include(CMakeRC.cmake)
cmrc_add_resource_library(web-contents NAMESPACE web WHENCE resources resources/index.html)
target_link_libraries(${COMPONENT_LIB} PRIVATE web-contents)
