cmake_minimum_required(VERSION 3.10)
project(MFSS_Verification)

# =============================================================

# =============================================================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# =============================================================

# =============================================================
if(NOT MSVC)
    
    add_compile_options(-O3 -march=native -Wall -pthread -fPIC -DIL_STD)
endif()

# =============================================================

# =============================================================


set(CPLEX_ROOT_DIR "/cplex") 



set(CPLEX_INCLUDE_DIR "${CPLEX_ROOT_DIR}/cplex/include")
set(CONCERT_INCLUDE_DIR "${CPLEX_ROOT_DIR}/concert/include")


set(CPLEX_LIB_DIR "${CPLEX_ROOT_DIR}/cplex/lib/x86-64_linux/static_pic")
set(CONCERT_LIB_DIR "${CPLEX_ROOT_DIR}/concert/lib/x86-64_linux/static_pic")


message(STATUS "CPLEX Include: ${CPLEX_INCLUDE_DIR}")
message(STATUS "CPLEX Lib Dir: ${CPLEX_LIB_DIR}")

include_directories(
    ${CPLEX_INCLUDE_DIR}
    ${CONCERT_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/src  
)

link_directories(
    ${CPLEX_LIB_DIR}
    ${CONCERT_LIB_DIR}
)

# =============================================================

# =============================================================


file(GLOB SOURCE_FILES "src/*.cpp")


file(GLOB HEADER_FILES "src/*.h")


add_executable(CplexVerifier
    ${SOURCE_FILES}
    ${HEADER_FILES}
)

# =============================================================

# =============================================================

target_link_libraries(CplexVerifier
    ilocplex
    concert
    cplex
    m
    pthread
    dl
)
