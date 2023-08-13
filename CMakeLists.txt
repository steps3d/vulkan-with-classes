cmake_minimum_required (VERSION 2.6)
project (vulkan-tests)

find_package(Vulkan)

if (WIN32)
	if (NOT Vulkan_FOUND)
		find_library(Vulkan_LIBRARY NAMES vulkan-1 vulkan PATHS ${CMAKE_SOURCE_DIR}/libs/vulkan)
		if (Vulkan_LIBRARY)
			set(Vulkan_FOUND ON)
			MESSAGE("Using bundled Vulkan library version")
		endif()
	endif()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_WIN32_KHR")
endif ()

if (NOT Vulkan_FOUND)
	message(FATAL_ERROR "Could not find Vulkan library!")
else ()
	message(STATUS ${Vulkan_LIBRARY})
endif ()

set(SHADER_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders)
set(SHADER_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/shaders)
set(glslc_executable glslangValidator)

file(GLOB SHADERS
  ${SHADER_SOURCE_DIR}/*.vert
  ${SHADER_SOURCE_DIR}/*.frag
  ${SHADER_SOURCE_DIR}/*.comp
  ${SHADER_SOURCE_DIR}/*.geom
  ${SHADER_SOURCE_DIR}/*.tesc
  ${SHADER_SOURCE_DIR}/*.tese
  ${SHADER_SOURCE_DIR}/*.mesh
  ${SHADER_SOURCE_DIR}/*.task
  ${SHADER_SOURCE_DIR}/*.rgen
  ${SHADER_SOURCE_DIR}/*.rchit
  ${SHADER_SOURCE_DIR}/*.rmiss)

add_custom_command(
  COMMAND
    ${CMAKE_COMMAND} -E make_directory ${SHADER_BINARY_DIR}
  OUTPUT ${SHADER_BINARY_DIR}
  COMMENT "Creating ${SHADER_BINARY_DIR}"
)

foreach(source IN LISTS SHADERS)
  get_filename_component(FILENAME ${source} NAME)
  add_custom_command(
    COMMAND
      ${glslc_executable} -V ${source}
      -o ${SHADER_BINARY_DIR}/${FILENAME}.spv
    OUTPUT ${SHADER_BINARY_DIR}/${FILENAME}.spv
    DEPENDS ${source} ${SHADER_BINARY_DIR}
    COMMENT "Compiling ${FILENAME}"
  )
  list(APPEND SPV_SHADERS ${SHADER_BINARY_DIR}/${FILENAME}.spv)
endforeach()

add_custom_target(shaders ALL DEPENDS ${SPV_SHADERS})

if (WIN32)
		# For Windows have to specify locations of all libs
		# Use all libs as static
	set ( LIBS_ROOT     "f:/Libraries" )
	set ( GLM           "${LIBS_ROOT}/glm" )
	set ( GLFW          "${LIBS_ROOT}/glfw-3.3.bin.WIN32" )
	set ( GLFW_LIB_PATH "${GLFW}/lib-vc2012" )
	set ( ASSIMP_DIR    "${LIBS_ROOT}/assimp-3.3.1-64" )
	set ( GLFW_LIB	    glfw3 )
	set ( ASSIMP_LIB    assimp-vc140-mt zlibstaticd  )
	#set ( ASSIMP_LIB    assimp-vc140-mtd zlibstaticd  )

	if (CMAKE_CL_64)	# 64-bit build with MSVC
		set ( GLFW              "${LIBS_ROOT}/glfw-3.3" )
		set ( GLFW_LIB_PATH     "${GLFW}/src/Debug" )
		#set ( ASSIMP_DIR      "${LIBS_ROOT}/assimp-3.3.1-64" )
		#set ( ASSIMP_DIR      "${LIBS_ROOT}/assimp" )
		set ( ASSIMP_DIR        "${LIBS_ROOT}/assimp" )
		set ( ASSIMP_INC_PATH   "${ASSIMP_DIR}/include/" )
		set ( ASSIMP_LIB_PATH   "${ASSIMP_DIR}/build64/lib/Debug/" )
		set ( ASSIMP_LIB        assimp-vc140-mtd zlibstaticd )
		set ( ZLIB_PATH		"${ASSIMP_DIR}/buil64/contrib/zlib/" )
		#set ( ASSIMP_LIB_PATH "${ASSIMP_DIR}/lib/Debug" )
	endif ()

	include_directories ( ${GLM} ${GLFW}/include ${ASSIMP_DIR}/include "${Vulkan_INCLUDE_DIRS}" "ext" )
	link_directories    ( ${GLFW_LIB_PATH} ${ASSIMP_LIB_PATH} ${ZLIB_PATH} ) 
	
	 add_definitions( -D_CRT_SECURE_NO_WARNINGS )
else ()
	set ( GLFW_LIB 		glfw )
	set ( ASSIMP_LIB	assimp )
endif ()

add_executable ( example-1 example-1.cpp VulkanWindow.cpp Log.cpp Data.cpp DescriptorSet.cpp Device.cpp Texture.cpp Dds.cpp )
target_link_libraries ( example-1 ${GLFW_LIB} "${Vulkan_LIBRARY}" )

