
add_library(far INTERFACE)
target_include_directories(far
  INTERFACE 
    ${CMAKE_CURRENT_SOURCE_DIR}/FarManager/plugins/common/unicode
    ${CMAKE_CURRENT_SOURCE_DIR}/FarManager/plugins/common)
