idf_component_register(
  SRCS "main.cpp" "Ina228IdfI2cTransport.cpp"
  INCLUDE_DIRS "."
  REQUIRES INA228 esp_driver_i2c esp_driver_gpio esp_timer freertos
)

target_compile_features(${COMPONENT_LIB} PUBLIC cxx_std_17)

# Refresh the source identity before every build so an incremental build cannot
# retain provenance captured by an earlier CMake configure. The script changes
# the generated header only when its contents change, avoiding needless rebuilds.
set(INA228_REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../..")
set(INA228_PROVENANCE_HEADER
    "${CMAKE_CURRENT_BINARY_DIR}/ina228_git_provenance.h")
find_package(Git QUIET)
set(INA228_GIT_EXECUTABLE "")
if(GIT_FOUND)
  set(INA228_GIT_EXECUTABLE "${GIT_EXECUTABLE}")
endif()

add_custom_target(ina228_example_git_provenance ALL
  COMMAND "${CMAKE_COMMAND}"
    "-DINA228_REPO_ROOT=${INA228_REPO_ROOT}"
    "-DINA228_PROVENANCE_HEADER=${INA228_PROVENANCE_HEADER}"
    "-DINA228_GIT_EXECUTABLE=${INA228_GIT_EXECUTABLE}"
    -P "${CMAKE_CURRENT_LIST_DIR}/GenerateGitProvenance.cmake"
  BYPRODUCTS "${INA228_PROVENANCE_HEADER}"
  VERBATIM
)
add_dependencies(${COMPONENT_LIB} ina228_example_git_provenance)
target_include_directories(${COMPONENT_LIB} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
