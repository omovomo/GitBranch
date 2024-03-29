find_package(spdlog CONFIG REQUIRED)

add_library(gitbranch MODULE
    GitBranch.cpp
    GitBranch.rc
    GitBranchW.vc.def
    command.h
    guid.hpp
    version.hpp
    starship.toml)

target_link_libraries(gitbranch PRIVATE spdlog::spdlog_header_only far)
target_compile_definitions(gitbranch 
    PRIVATE PLUGIN_MAJOR=${PROJECT_VERSION_MAJOR}
    PRIVATE PLUGIN_MINOR=${PROJECT_VERSION_MINOR}
    PRIVATE PLUGIN_BUILD=${PROJECT_VERSION_PATCH}
    PRIVATE PLUGIN_REVISION=${PROJECT_VERSION_TWEAK})


set_property(
    TARGET gitbranch
    PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
set(CPACK_GENERATOR ZIP)
set(CPACK_BINARY_ZIP ON)

install(TARGETS gitbranch
    LIBRARY DESTINATION GitBranch)
install_dependencies_for(TARGETS gitbranch DESTINATION GitBranch )
install(FILES starship.toml DESTINATION GitBranch )

set(CPACK_INSTALL_CMAKE_PROJECTS "${CMAKE_CURRENT_BINARY_DIR};gitbranch;ALL;.")
include(CPack)
