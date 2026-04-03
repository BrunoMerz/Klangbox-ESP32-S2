
get_filename_component(dir ${CMAKE_CURRENT_LIST_FILE} PATH)
FILE(GLOB_RECURSE app_sources ${dir}/src/*.cpp)

idf_component_register(SRCS ${app_sources}      
                    REQUIRES "ESP32-audioI2S"
                    INCLUDE_DIRS "src"
                    REQUIRES wear_levelling Arduino esp-dsp
)

add_compile_definitions(CORE_DEBUG_LEVEL=3)
target_compile_options(${COMPONENT_TARGET} PRIVATE -Wno-return-type)
target_compile_definitions(${COMPONENT_LIB} PUBLIC CORE_DEBUG_LEVEL=3)