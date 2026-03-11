cmake_minimum_required(VERSION 3.1.0...3.27.0)

project("RayTracer" LANGUAGES CXX CUDA)
# C++ language standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Source
set ( EXTERNAL
  src/external/stb_image.h
)

# CUDA配置
set(CMAKE_CUDA_STANDARD 17)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)
set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -O3")

# 对于不同的GPU架构, 取消注释对应行
# Volta (V100): -arch=sm_70
# Turing (RTX 2080): -arch=sm_75
# Ampere (A100/RTX 3000): -arch=sm_80 or -arch=sm_86
# Ada (RTX 4060): -arch=sm_89

set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -arch=sm_89 -gencode=arch=compute_89,code=sm_89")

file(GLOB SOURCE_ONEWEEKEND "src/inOneWeekend/*.cpp" "src/inOneWeekend/*.h")
file(GLOB SOURCE_NEXTWEEK "src/nextWeek/*.cpp" "src/nextWeek/*.h")
file(GLOB SOURCE_RESTOFYOURLIFE "src/restOfYourLife/*.cpp" "src/restOfYourLife/*.h")
file(GLOB SOURCE_CUDA_RESTOFYOURLIFE "src/cuda_restOfYourLife/*.cpp" "src/cuda_restOfYourLife/*.cu" "src/cuda_restOfYourLife/*.h")

include_directories(src)

# Executables
add_executable(inOneWeekend ${EXTERNAL} ${SOURCE_ONEWEEKEND}) 
add_executable(nextWeek ${EXTERNAL} ${SOURCE_NEXTWEEK}) 
add_executable(restOfYourLife ${EXTERNAL} ${SOURCE_RESTOFYOURLIFE}) 
add_executable(cuda_restOfYourLife ${EXTERNAL} ${SOURCE_CUDA_RESTOFYOURLIFE})

# Set CUDA properties for cuda_restOfYourLife
set_target_properties(cuda_restOfYourLife PROPERTIES
    CUDA_STANDARD 17
    CUDA_STANDARD_REQUIRED ON
    CUDA_SEPARABLE_COMPILATION ON
)

# Specific compiler flags below. We're not going to add options for all possible compilers, but if
# you're new to CMake (like we are), the following may be a helpful example if you're using a
# different compiler or want to set different compiler options.

message (STATUS "Compiler ID: " ${CMAKE_CXX_COMPILER_ID})
message (STATUS "Release flags: " ${CMAKE_CXX_FLAGS_RELEASE})
message (STATUS "Debug flags: " ${CMAKE_CXX_FLAGS_DEBUG})

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    # /wd #### - Disable warning
    # /we #### - treat warning as error
    add_compile_options("/W4")      # Enable level-4 warnings
    add_compile_options("/we 4265") # Class has virtual functions, but its non-trivial destructor is not virtual
    add_compile_options("/we 5204") # Class has virtual functions, but its trivial destructor is not virtual
    add_compile_options("/wd 4100") # unreferenced formal parameter
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_compile_options(-Wnon-virtual-dtor) # Class has virtual functions, but its destructor is not virtual
    add_compile_options(-Wreorder) # Data member will be initialized after [other] data member
    add_compile_options(-Wmaybe-uninitialized) # Variable improperly initialized
    add_compile_options(-Wunused-variable) # Variable is defined but unused
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-Wnon-virtual-dtor) # Class has virtual functions, but its destructor is not virtual
    add_compile_options(-Wreorder) # Data member will be initialized after [other] data member
    add_compile_options(-Wsometimes-uninitialized) # Variable improperly initialized
    add_compile_options(-Wunused-variable) # Variable is defined but unused
endif()