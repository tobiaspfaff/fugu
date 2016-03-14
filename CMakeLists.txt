project(Fugu)

cmake_minimum_required(VERSION 2.8)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/src/cmake/")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/bin")

##################################
# Build options
##################################

option(DEBUG "Enable debug compilation" OFF)

message(STATUS "Options - "
    " -DDEBUG='${DEBUG}' "
    )

##################################
# Source files
##################################

set(INCPATHS src)

set(PLATFORM_SOURCES
    src/platform/window.hpp
    src/platform/window.cpp
)
set(VULKAN_SOURCES
    src/vulkan/buffer.hpp
    src/vulkan/buffer.cpp
    src/vulkan/instance.hpp    
    src/vulkan/instance.cpp    
	src/vulkan/shader.hpp
    src/vulkan/shader.cpp
    src/vulkan/vkmain.hpp
	src/vulkan/vkutil.hpp
	src/vulkan/vkutil.cpp
)
set(UTIL_SOURCES
    src/util/util.hpp    
    src/util/util.cpp    
)
set(MISC_SOURCES
    src/main.cpp
)
set(SHADERS
    src/shader/simple.vert
    src/shader/simple.frag
)

if (WIN32)
    source_group("Platform" FILES ${PLATFORM_SOURCES})
    source_group("Vulkan" FILES ${VULKAN_SOURCES})
    source_group("Util" FILES ${UTIL_SOURCES})
    source_group("Misc" FILES ${MISC_SOURCES})
    source_group("Shader" FILES ${SHADERS})
endif()

set(SOURCES 
    ${PLATFORM_SOURCES}
    ${VULKAN_SOURCES}
    ${UTIL_SOURCES}
    ${MISC_SOURCES}
	${SHADERS}
)

##################################
# External libraries
##################################

set(LIBS)

find_package(Vulkan REQUIRED)
list(APPEND LIBS ${VULKAN_LIBRARY})
list(APPEND INCPATHS ${VULKAN_INCLUDE_DIR})

##################################
# Compile shaders
##################################

set(COMPILER "${VULKAN_BIN}/glslangValidator.exe")
foreach(file ${SHADERS})
	set(CPATH "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shader")
	get_filename_component(CFILE ${file} NAME)
	set(CNAME "${CPATH}/${CFILE}.spv")
	file(MAKE_DIRECTORY ${CPATH})
	add_custom_command(OUTPUT ${CNAME}
					   COMMAND ${COMPILER} "-V" "-o" ${CNAME} ${file}
					   WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
					   DEPENDS ${file}
					   )
	list(APPEND COMPILED_SHADERS ${CNAME})
endforeach()
add_custom_target(SHADER_TARGET ALL
                  DEPENDS ${COMPILED_SHADERS})

##################################
# Build executable
##################################

set(AS_FLAGS " ")

if (DEBUG)
    SET(EXECCMD fugud)
    #set(AS_FLAGS "${AS_FLAGS} -g -DDEBUG" )
else()
    SET(EXECCMD fugu)
    #set(AS_FLAGS "${AS_FLAGS} -O3 -Wreturn-type" )
endif()

add_executable(${EXECCMD} 
    ${SOURCES} 
)

target_link_libraries(${EXECCMD} ${LIBS})
set_target_properties(${EXECCMD} PROPERTIES COMPILE_FLAGS ${AS_FLAGS})
target_include_directories(${EXECCMD} PUBLIC ${INCPATHS})
add_dependencies(${EXECCMD} SHADER_TARGET)

#install(TARGETS ${EXECCMD} DESTINATION bin)

