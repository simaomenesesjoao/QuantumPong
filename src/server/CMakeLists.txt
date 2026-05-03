cmake_minimum_required(VERSION 3.16)
project(QuantumPong CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

find_package(OpenCL REQUIRED)
find_package(Threads REQUIRED)

add_executable(QuantumPong
  connection_handler.cpp
  game_engine.cpp
  simulator.cpp
  main.cpp
)

target_compile_options(QuantumPong PRIVATE -Wall)
target_link_libraries(QuantumPong PRIVATE OpenCL::OpenCL Threads::Threads)

# kernel.cpp is loaded at runtime via std::ifstream("kernel.cpp") (relative to
# cwd, which is the directory holding the binary). Stage it next to the binary
# whenever kernel.cpp changes, even if no .cpp sources changed.
set(STAGED_KERNEL ${CMAKE_CURRENT_BINARY_DIR}/kernel.cpp)
add_custom_command(
  OUTPUT  ${STAGED_KERNEL}
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_CURRENT_SOURCE_DIR}/kernel.cpp
    ${STAGED_KERNEL}
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/kernel.cpp
  COMMENT "Staging kernel.cpp next to the binary"
  VERBATIM
)
add_custom_target(stage_kernel ALL DEPENDS ${STAGED_KERNEL})
add_dependencies(QuantumPong stage_kernel)
