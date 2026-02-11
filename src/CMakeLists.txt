# CLI-specific dependencies
CPMAddPackage("gh:pdinklag/cmdline#v1.0.0")

add_executable(alz-cli alz.cpp)
target_compile_definitions(alz-cli PRIVATE ALZ_VERBOSE)
target_link_libraries(alz-cli PRIVATE alz cmdline pm-malloc)
set_target_properties(alz-cli PROPERTIES OUTPUT_NAME "alz")
