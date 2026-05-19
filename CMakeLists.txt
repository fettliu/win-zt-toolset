cmake_minimum_required(VERSION 3.15)
project(ZeroTierTools C CXX)

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)

# Use compilers from environment variables or PATH
# Users can set CC and CXX environment variables before running cmake
if(NOT DEFINED CMAKE_C_COMPILER)
    if(DEFINED ENV{CC})
        set(CMAKE_C_COMPILER $ENV{CC})
        message(STATUS "Using CC from environment: $ENV{CC}")
    else()
        find_program(CMAKE_C_COMPILER NAMES gcc cc)
        if(CMAKE_C_COMPILER)
            message(STATUS "Found C compiler: ${CMAKE_C_COMPILER}")
        else()
            message(FATAL_ERROR "GCC compiler not found. Please install MinGW and add it to PATH, or set CC environment variable")
        endif()
    endif()
endif()

if(NOT DEFINED CMAKE_CXX_COMPILER)
    if(DEFINED ENV{CXX})
        set(CMAKE_CXX_COMPILER $ENV{CXX})
        message(STATUS "Using CXX from environment: $ENV{CXX}")
    else()
        find_program(CMAKE_CXX_COMPILER NAMES g++ c++)
        if(CMAKE_CXX_COMPILER)
            message(STATUS "Found C++ compiler: ${CMAKE_CXX_COMPILER}")
        else()
            message(FATAL_ERROR "G++ compiler not found. Please install MinGW G++ and add it to PATH, or set CXX environment variable")
        endif()
    endif()
endif()

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DADD_EXPORTS -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DADD_EXPORTS -Wall")

# Linker flags for Windows (including static linking and snprintf fix)
set(LINKER_FLAGS "")
if(WIN32 AND CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    set(LINKER_FLAGS "-Wl,--defsym,__imp__snprintf=_snprintf -static-libgcc -static-libstdc++")
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    set(LINKER_FLAGS "-static-libgcc -static-libstdc++")
endif()

# Extra libraries for Windows
set(EXTRA_LIBS "")
if(WIN32)
    set(EXTRA_LIBS ws2_32 wsock32 iphlpapi shlwapi pthread)
endif()

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/build/include)
link_directories(${CMAKE_SOURCE_DIR}/build)

# Build libzt first (optional - can be done manually)
option(BUILD_LIBZT "Build libzt library first" OFF)
if(BUILD_LIBZT)
    add_custom_target(libzt
        COMMAND ${CMAKE_COMMAND} -E echo "Please run build-libzt.ps1 first"
        COMMENT "Building libzt..."
    )
endif()

# zt-join executable
add_executable(zt-join zt-join.c)
target_link_libraries(zt-join PRIVATE zt ${LINKER_FLAGS} ${EXTRA_LIBS})
target_compile_definitions(zt-join PRIVATE ZTS_DISABLE_CENTRAL_API)
set_target_properties(zt-join PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build"
)

# zt-forward executable
add_executable(zt-forward zt-forward.c)
target_link_libraries(zt-forward PRIVATE zt ${LINKER_FLAGS} ${EXTRA_LIBS})
set_target_properties(zt-forward PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build"
)

# pylon executable
add_executable(pylon pylon.c)
target_link_libraries(pylon PRIVATE zt ${LINKER_FLAGS} ${EXTRA_LIBS})
set_target_properties(pylon PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build"
)

# zt-ping executable
add_executable(zt-ping zt-ping.c)
target_link_libraries(zt-ping PRIVATE zt ${LINKER_FLAGS} ${EXTRA_LIBS})
set_target_properties(zt-ping PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build"
)

# Print configuration summary
message(STATUS "========================================")
message(STATUS "ZeroTier Tools Configuration")
message(STATUS "========================================")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C compiler: ${CMAKE_C_COMPILER}")
message(STATUS "CXX compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "========================================")
