cmake_minimum_required (VERSION 2.6)
project (vulkan-tests)

find_package(Vulkan REQUIRED)

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
	message(STATUS "Include ${Vulkan_INCLUDE_DIRS}")
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
      --target-env vulkan1.2
    OUTPUT ${SHADER_BINARY_DIR}/${FILENAME}.spv
    DEPENDS ${source} ${SHADER_BINARY_DIR}
    COMMENT "Compiling ${FILENAME} with vulkan1.2"
  )
  list(APPEND SPV_SHADERS ${SHADER_BINARY_DIR}/${FILENAME}.spv)
endforeach()

add_custom_target(shaders ALL DEPENDS ${SPV_SHADERS})

if (WIN32)
		# For Windows have to specify locations of all libs
		# Use all libs as static
	set ( LIBS_ROOT     "d:/Libraries"      )
	set ( GLM           "${LIBS_ROOT}/glm"  )
	set ( GLI           "${LIBS_ROOT}/gli"  )
	set ( VOLK          "${LIBS_ROOT}/volk" )
	set ( GLFW          "${LIBS_ROOT}/glfw-3.3.bin.WIN32" )
	set ( GLFW_LIB_PATH "${GLFW}/lib-vc2012"              )
	set ( ASSIMP_DIR    "${LIBS_ROOT}/assimp-3.3.1-64"    )
	set ( GLFW_LIB	    glfw3 )
	set ( ASSIMP_LIB    assimp-vc140-mt zlibstaticd  )
	#set ( ASSIMP_LIB   assimp-vc140-mtd zlibstaticd  )

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

	include_directories ( ${GLM} ${GLI} ${GLFW}/include ${ASSIMP_DIR}/include "${Vulkan_INCLUDE_DIRS}" "ext" )
	link_directories    ( ${GLFW_LIB_PATH} ${ASSIMP_LIB_PATH} ${ZLIB_PATH} ) 
	
	 add_definitions( -D_CRT_SECURE_NO_WARNINGS )
else ()
	set ( GLFW_LIB 		glfw )
	set ( ASSIMP_LIB	assimp )
	set ( GLI           "ext/gli"  )
	include_directories ( "${Vulkan_INCLUDE_DIRS}" "ext" "${GLI}")

endif ()

set ( VK_FILES   VulkanWindow.cpp Log.cpp Data.cpp DescriptorSet.cpp Device.cpp Texture.cpp Dds.cpp Mesh.cpp CommandBuffer.cpp )
set ( MATH_FILES bbox.cpp plane.cpp Camera.cpp sphere.cpp ray.cpp )

add_executable ( base-1 base-1.cpp Log.cpp )
target_link_libraries ( base-1 ${GLFW_LIB} "${Vulkan_LIBRARY}" )

add_executable ( base-2 base-2.cpp Log.cpp )
target_link_libraries ( base-2 ${GLFW_LIB} "${Vulkan_LIBRARY}" )

add_executable ( base-3 base-3.cpp Log.cpp )
target_link_libraries ( base-3 ${GLFW_LIB} "${Vulkan_LIBRARY}" )

add_executable ( base-4 base-4.cpp Log.cpp )
target_link_libraries ( base-4 ${GLFW_LIB} "${Vulkan_LIBRARY}" )

add_executable ( base-5 base-5.cpp Log.cpp )
target_link_libraries ( base-5 ${GLFW_LIB} "${Vulkan_LIBRARY}" )

add_executable ( base-6 base-6.cpp Log.cpp )
target_link_libraries ( base-6 ${GLFW_LIB} "${Vulkan_LIBRARY}" )

add_executable ( base-7 base-7.cpp Log.cpp Data.cpp )
target_link_libraries ( base-7 ${GLFW_LIB} "${Vulkan_LIBRARY}" )

add_executable ( base-8 base-8.cpp Log.cpp Data.cpp )
target_link_libraries ( base-8 ${GLFW_LIB} "${Vulkan_LIBRARY}" )

add_executable ( base-9 base-9.cpp Log.cpp Data.cpp )
target_link_libraries ( base-9 ${GLFW_LIB} "${Vulkan_LIBRARY}" )

add_executable ( example-1 example-1.cpp VulkanWindow.cpp Log.cpp Data.cpp DescriptorSet.cpp Device.cpp Texture.cpp Dds.cpp CommandBuffer.cpp )
target_link_libraries ( example-1 ${GLFW_LIB} "${Vulkan_LIBRARY}" )

add_executable ( example-2 example-2.cpp VulkanWindow.cpp Log.cpp Data.cpp DescriptorSet.cpp Device.cpp Texture.cpp Dds.cpp CommandBuffer.cpp )
target_link_libraries ( example-2 ${GLFW_LIB} "${Vulkan_LIBRARY}" )

add_executable ( example-3 example-3.cpp VulkanWindow.cpp Log.cpp Data.cpp DescriptorSet.cpp Device.cpp Texture.cpp Dds.cpp CommandBuffer.cpp )
target_link_libraries ( example-3 ${GLFW_LIB} "${Vulkan_LIBRARY}" )

add_executable ( example-4 example-4.cpp VulkanWindow.cpp Log.cpp Data.cpp DescriptorSet.cpp Device.cpp Texture.cpp Dds.cpp bbox.cpp plane.cpp ray.cpp CommandBuffer.cpp )
target_link_libraries ( example-4 ${GLFW_LIB} "${Vulkan_LIBRARY}" ${ASSIMP_LIB} )

add_executable ( example-deferred example-deferred.cpp VulkanWindow.cpp Log.cpp Data.cpp DescriptorSet.cpp Device.cpp Texture.cpp Dds.cpp Mesh.cpp bbox.cpp plane.cpp Camera.cpp sphere.cpp ray.cpp CommandBuffer.cpp )
target_link_libraries ( example-deferred ${GLFW_LIB} "${Vulkan_LIBRARY}" ${ASSIMP_LIB} )

add_executable ( example-particles example-particles.cpp VulkanWindow.cpp Log.cpp Data.cpp DescriptorSet.cpp Device.cpp Texture.cpp Dds.cpp Mesh.cpp bbox.cpp plane.cpp Camera.cpp sphere.cpp ray.cpp CommandBuffer.cpp )
target_link_libraries ( example-particles ${GLFW_LIB} "${Vulkan_LIBRARY}" ${ASSIMP_LIB} )

add_executable ( example-mesh example-mesh.cpp VulkanWindow.cpp Log.cpp Data.cpp DescriptorSet.cpp Device.cpp Texture.cpp Dds.cpp Mesh.cpp bbox.cpp plane.cpp Camera.cpp sphere.cpp ray.cpp CommandBuffer.cpp )
target_link_libraries ( example-mesh ${GLFW_LIB} "${Vulkan_LIBRARY}" ${ASSIMP_LIB} )

add_executable ( example-pbr example-pbr.cpp VulkanWindow.cpp Log.cpp Data.cpp DescriptorSet.cpp Device.cpp Texture.cpp Dds.cpp Mesh.cpp bbox.cpp plane.cpp Camera.cpp sphere.cpp ray.cpp CommandBuffer.cpp )
target_link_libraries ( example-pbr ${GLFW_LIB} "${Vulkan_LIBRARY}" ${ASSIMP_LIB} )

add_executable ( example-instanced example-instanced.cpp VulkanWindow.cpp Log.cpp Data.cpp DescriptorSet.cpp Device.cpp Texture.cpp Dds.cpp Mesh.cpp bbox.cpp plane.cpp Camera.cpp sphere.cpp ray.cpp CommandBuffer.cpp )
target_link_libraries ( example-instanced ${GLFW_LIB} "${Vulkan_LIBRARY}" ${ASSIMP_LIB} )

add_executable ( example-hedgehog example-hedgehog.cpp VulkanWindow.cpp Log.cpp Data.cpp DescriptorSet.cpp Device.cpp Texture.cpp Dds.cpp Mesh.cpp bbox.cpp plane.cpp Camera.cpp sphere.cpp ray.cpp CommandBuffer.cpp )
target_link_libraries ( example-hedgehog ${GLFW_LIB} "${Vulkan_LIBRARY}" ${ASSIMP_LIB} )

add_executable ( example-shadow-map example-shadow-map.cpp VulkanWindow.cpp Log.cpp Data.cpp DescriptorSet.cpp Device.cpp Texture.cpp Dds.cpp Mesh.cpp bbox.cpp plane.cpp Camera.cpp sphere.cpp ray.cpp CommandBuffer.cpp )
target_link_libraries ( example-shadow-map ${GLFW_LIB} "${Vulkan_LIBRARY}" ${ASSIMP_LIB} )

add_executable ( example-text example-text.cpp VulkanWindow.cpp Log.cpp Data.cpp DescriptorSet.cpp Device.cpp Texture.cpp Dds.cpp Mesh.cpp bbox.cpp plane.cpp Camera.cpp sphere.cpp ray.cpp CommandBuffer.cpp )
target_link_libraries ( example-text ${GLFW_LIB} "${Vulkan_LIBRARY}" ${ASSIMP_LIB} )

add_executable ( example-texture-array example-texture-array.cpp VulkanWindow.cpp Log.cpp Data.cpp DescriptorSet.cpp Device.cpp Texture.cpp Dds.cpp Mesh.cpp bbox.cpp plane.cpp Camera.cpp sphere.cpp ray.cpp CommandBuffer.cpp )
target_link_libraries ( example-texture-array ${GLFW_LIB} "${Vulkan_LIBRARY}" ${ASSIMP_LIB} )

add_executable ( example-bezier example-bezier.cpp VulkanWindow.cpp Log.cpp Data.cpp DescriptorSet.cpp Device.cpp Texture.cpp Dds.cpp Mesh.cpp bbox.cpp plane.cpp Camera.cpp sphere.cpp ray.cpp CommandBuffer.cpp )
target_link_libraries ( example-bezier ${GLFW_LIB} "${Vulkan_LIBRARY}" ${ASSIMP_LIB} )

add_executable ( example-push-constants example-push-constants.cpp VulkanWindow.cpp Log.cpp Data.cpp DescriptorSet.cpp Device.cpp Texture.cpp Dds.cpp Mesh.cpp bbox.cpp plane.cpp Camera.cpp sphere.cpp ray.cpp CommandBuffer.cpp )
target_link_libraries ( example-push-constants ${GLFW_LIB} "${Vulkan_LIBRARY}" ${ASSIMP_LIB} )

add_executable ( example-dynamic-uniform example-dynamic-uniform.cpp VulkanWindow.cpp Log.cpp Data.cpp DescriptorSet.cpp Device.cpp Texture.cpp Dds.cpp Mesh.cpp bbox.cpp plane.cpp Camera.cpp sphere.cpp ray.cpp CommandBuffer.cpp )
target_link_libraries ( example-dynamic-uniform ${GLFW_LIB} "${Vulkan_LIBRARY}" ${ASSIMP_LIB} )

add_executable ( example-buffer-address example-buffer-address.cpp VulkanWindow.cpp Log.cpp Data.cpp DescriptorSet.cpp Device.cpp Texture.cpp Dds.cpp Mesh.cpp bbox.cpp plane.cpp Camera.cpp sphere.cpp ray.cpp CommandBuffer.cpp )
target_link_libraries ( example-buffer-address ${GLFW_LIB} "${Vulkan_LIBRARY}" ${ASSIMP_LIB} )

add_executable ( example-bindless example-bindless.cpp VulkanWindow.cpp Log.cpp Data.cpp DescriptorSet.cpp Device.cpp Texture.cpp Dds.cpp Mesh.cpp bbox.cpp plane.cpp Camera.cpp sphere.cpp ray.cpp CommandBuffer.cpp )
target_link_libraries ( example-bindless ${GLFW_LIB} "${Vulkan_LIBRARY}" ${ASSIMP_LIB} )

add_executable ( example-descriptor-buffer example-descriptor-buffer.cpp VulkanWindow.cpp Log.cpp Data.cpp DescriptorSet.cpp Device.cpp Texture.cpp Dds.cpp Mesh.cpp bbox.cpp plane.cpp Camera.cpp sphere.cpp ray.cpp CommandBuffer.cpp )
target_link_libraries ( example-descriptor-buffer ${GLFW_LIB} "${Vulkan_LIBRARY}" ${ASSIMP_LIB} )

add_executable ( example-dynamic-rendering example-dynamic-rendering.cpp VulkanWindow.cpp Log.cpp Data.cpp DescriptorSet.cpp Device.cpp Texture.cpp Dds.cpp Mesh.cpp bbox.cpp plane.cpp Camera.cpp sphere.cpp ray.cpp CommandBuffer.cpp )
target_link_libraries ( example-dynamic-rendering ${GLFW_LIB} "${Vulkan_LIBRARY}" ${ASSIMP_LIB} )

