cmake_minimum_required(VERSION 3.16)
project(Living_Photoshop LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(LP_SRC
    src/main.cpp
    src/image_io.cpp
    src/living_pipeline.cpp
)

set(MICRODB_SRC
    ../MicroDB/src/micro_swarm_api.cpp
    ../MicroDB/src/sim/agent.cpp
    ../MicroDB/src/sim/db_engine.cpp
    ../MicroDB/src/sim/db_sql.cpp
    ../MicroDB/src/sim/dna_memory.cpp
    ../MicroDB/src/sim/environment.cpp
    ../MicroDB/src/sim/fields.cpp
    ../MicroDB/src/sim/io.cpp
    ../MicroDB/src/sim/mycel.cpp
    ../MicroDB/src/sim/report.cpp
    ../MicroDB/src/compute/opencl_loader.cpp
    ../MicroDB/src/compute/opencl_runtime.cpp
)

set(HALO_SRC
    ../fastpath.cpp
)

add_executable(living_photoshop
    ${LP_SRC}
    ${MICRODB_SRC}
    ${HALO_SRC}
)

target_compile_definitions(living_photoshop PRIVATE MICRO_SWARM_STATIC=1)

add_custom_command(TARGET living_photoshop POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:living_photoshop>/kernels"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/kernels/diffuse.cl"
        "$<TARGET_FILE_DIR:living_photoshop>/kernels/diffuse.cl"
)

target_include_directories(living_photoshop PRIVATE
    src
    ../MicroDB/src
)

find_package(OpenCL)
if(OpenCL_FOUND)
    target_compile_definitions(living_photoshop PRIVATE MICRO_SWARM_OPENCL=1 MICRO_SWARM_OPENCL_DYNAMIC=1)
    target_include_directories(living_photoshop PRIVATE ${OpenCL_INCLUDE_DIRS})
    target_link_libraries(living_photoshop PRIVATE ${OpenCL_LIBRARIES})
endif()

if(WIN32)
    target_compile_definitions(living_photoshop PRIVATE NOMINMAX)
endif()
