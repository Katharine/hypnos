idf_component_register(SRCS "eightsleep.cpp" "internal/apiclient.cpp"
                    INCLUDE_DIRS "include"
                    REQUIRES http_cxx expected arduinojson)

component_compile_options("-Wno-missing-field-initializers")
