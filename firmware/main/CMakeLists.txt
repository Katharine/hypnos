idf_component_register(SRCS "main.cpp" "apps/root/root_app.cpp" "apps/config/app.cpp" "apps/main/app.cpp" "apps/main/state.cpp" "apps/main/settings/menu.cpp"
                    INCLUDE_DIRS ".")

component_compile_options("-Wno-missing-field-initializers")
