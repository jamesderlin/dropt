# A deliberately rudimentary command-line option parser.
#
# Copyright (C) 2006-2018 James D. Lin <jamesdlin@berkeley.edu>
#
# The latest version of this file can be downloaded from:
# <http://www.taenarum.com/software/dropt/>
#
# This software is provided 'as-is', without any express or implied
# warranty.  In no event will the authors be held liable for any damages
# arising from the use of this software.
#
# Permission is granted to anyone to use this software for any purpose,
# including commercial applications, and to alter it and redistribute it
# freely, subject to the following restrictions:
#
# 1. The origin of this software must not be misrepresented; you must not
#    claim that you wrote the original software. If you use this software
#    in a product, an acknowledgment in the product documentation would be
#    appreciated but is not required.
#
# 2. Altered source versions must be plainly marked as such, and must not be
#    misrepresented as being the original software.
#
# 3. This notice may not be removed or altered from any source distribution.
#

cmake_minimum_required(VERSION 2.8.11)

project(dropt)
set(PROJECT_VERSION 2.0.1)
set(PROJECT_DESCRIPTION "${PROJECT_NAME} is yet another C and C++ library for parsing command-line options.")
set(PROJECT_HOMEPAGE_URL "https://github.com/jamesderlin/dropt")

option(BUILD_SHARED_LIBS "Build shared libraries" ON)

include(GNUInstallDirs)
include(CPackComponent)
include(CMakePackageConfigHelpers)

set(PackagingTemplatesDir "${CMAKE_CURRENT_SOURCE_DIR}/packaging")
set(IncludeDir "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(SrcDir "${CMAKE_CURRENT_SOURCE_DIR}/src")

function(configure_pkgconfig_file PKG_CONFIG_NAME componentName)
	set(PKG_CONFIG_FILE_NAME "${CMAKE_CURRENT_BINARY_DIR}/${PKG_CONFIG_NAME}.pc")
	configure_file("${PackagingTemplatesDir}/pkgconfig.pc.in" "${PKG_CONFIG_FILE_NAME}" @ONLY)
	install(FILES "${PKG_CONFIG_FILE_NAME}"
		DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig"
		COMPONENT ${componentName}
	)
endfunction()

set(${PROJECT_NAME}_h_files
    ${IncludeDir}/${PROJECT_NAME}.h
    ${IncludeDir}/${PROJECT_NAME}_string.h
)

set(${PROJECT_NAME}_c_files
    ${SrcDir}/${PROJECT_NAME}.c
    ${SrcDir}/${PROJECT_NAME}_handlers.c
    ${SrcDir}/${PROJECT_NAME}_string.c
)

add_library(${PROJECT_NAME}
    ${${PROJECT_NAME}_c_files}
    ${${PROJECT_NAME}_h_files}
)

target_include_directories(${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include> $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

cpack_add_component(c_lib
	DISPLAY_NAME "${PROJECT_NAME}"
	DESCRIPTION "The C library"
	REQUIRED
)
cpack_add_component(c_lib_dev
	DISPLAY_NAME "${PROJECT_NAME} SDK"
	DESCRIPTION "SDK for the C library"
	DEPENDS c_lib
)
set(CPACK_DEBIAN_C_LIB_PACKAGE_NAME "lib${PROJECT_NAME}")

set(CPACK_DEBIAN_C_LIB_DEV_PACKAGE_NAME "${CPACK_DEBIAN_C_LIB_PACKAGE_NAME}-dev")
set(CPACK_DEBIAN_C_LIB_DEV_PACKAGE_SUGGESTS "cmake, pkg-config, pkg-conf")
set(CPACK_DEBIAN_C_LIB_DEV_PACKAGE_DEPENDS "${CPACK_DEBIAN_C_LIB_PACKAGE_NAME}")

set(CPACK_RPM_C_LIB_PACKAGE_NAME "${CPACK_DEBIAN_C_LIB_PACKAGE_NAME}")

set(CPACK_RPM_C_LIB_DEV_PACKAGE_NAME "${CPACK_RPM_C_LIB_PACKAGE_NAME}-devel")
set(CPACK_RPM_C_LIB_DEV_PACKAGE_SUGGESTS "${CPACK_DEBIAN_C_LIB_DEV_PACKAGE_SUGGESTS}")
set(CPACK_RPM_C_LIB_DEV_PACKAGE_DEPENDS "${CPACK_RPM_C_LIB_PACKAGE_NAME}")

install(TARGETS ${PROJECT_NAME}
	EXPORT ${PROJECT_NAME}
	LIBRARY
		DESTINATION "${CMAKE_INSTALL_LIBDIR}"
		COMPONENT c_lib
	ARCHIVE
		DESTINATION "${CMAKE_INSTALL_LIBDIR}"
		COMPONENT c_lib_dev
	INCLUDES
		DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
		# COMPONENT c_lib_dev # component is not allowed for includes! Headers are installed separately with `install(FILES`! Includes only marks the headers for export
)
install(FILES ${${PROJECT_NAME}_h_files}
	DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
	COMPONENT c_lib_dev
)

install(EXPORT ${PROJECT_NAME}
	FILE "${PROJECT_NAME}Config.cmake"
	NAMESPACE dropt::
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
	COMPONENT c_lib_dev
)
configure_pkgconfig_file(${PROJECT_NAME} c_lib_dev)

set(${PROJECT_NAME}xx_hpp_files
    ${IncludeDir}/${PROJECT_NAME}xx.hpp
)

set(${PROJECT_NAME}xx_cpp_files
    ${SrcDir}/${PROJECT_NAME}xx.cpp
)

cpack_add_component(cpp_lib
	DISPLAY_NAME "${PROJECT_NAME}xx"
	DESCRIPTION "The C++ library"
	DEPENDS c_lib
)
cpack_add_component(cpp_lib_dev
	DISPLAY_NAME "${PROJECT_NAME}xx SDK"
	DESCRIPTION "SDK for the C++ library"
	DEPENDS c_lib_dev cpp_lib
)
set(CPACK_DEBIAN_CPP_LIB_PACKAGE_NAME "lib${PROJECT_NAME}xx")
set(CPACK_DEBIAN_CPP_LIB_PACKAGE_DEPENDS "${CPACK_DEBIAN_C_LIB_PACKAGE_NAME}")

set(CPACK_DEBIAN_CPP_LIB_DEV_PACKAGE_NAME "${CPACK_DEBIAN_CPP_LIB_PACKAGE_NAME}-dev")
set(CPACK_DEBIAN_CPP_LIB_DEV_PACKAGE_DEPENDS "${CPACK_DEBIAN_CPP_LIB_PACKAGE_NAME}, ${CPACK_DEBIAN_C_LIB_DEV_PACKAGE_NAME}")
set(CPACK_DEBIAN_CPP_LIB_DEV_PACKAGE_SUGGESTS "${CPACK_DEBIAN_C_LIB_DEV_PACKAGE_SUGGESTS}")

set(CPACK_RPM_CPP_LIB_PACKAGE_NAME "${CPACK_DEBIAN_CPP_LIB_PACKAGE_NAME}")
set(CPACK_RPM_CPP_LIB_PACKAGE_DEPENDS "${CPACK_RPM_C_LIB_PACKAGE_NAME}")

set(CPACK_RPM_CPP_LIB_DEV_PACKAGE_NAME "${CPACK_RPM_CPP_LIB_PACKAGE_NAME}-devel")
set(CPACK_RPM_CPP_LIB_DEV_PACKAGE_DEPENDS "${CPACK_RPM_CPP_LIB_PACKAGE_NAME}, ${CPACK_RPM_C_LIB_DEV_PACKAGE_NAME}")
set(CPACK_RPM_CPP_LIB_DEV_PACKAGE_SUGGESTS "${CPACK_DEBIAN_CPP_LIB_DEV_PACKAGE_SUGGESTS}")

add_library(${PROJECT_NAME}xx
    ${${PROJECT_NAME}xx_cpp_files}
    ${${PROJECT_NAME}xx_hpp_files}
)

target_include_directories(${PROJECT_NAME}xx PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include> $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
install(TARGETS "${PROJECT_NAME}xx"
	EXPORT "${PROJECT_NAME}xx"
	LIBRARY
		DESTINATION "${CMAKE_INSTALL_LIBDIR}"
		COMPONENT cpp_lib
	ARCHIVE
		DESTINATION "${CMAKE_INSTALL_LIBDIR}"
		COMPONENT cpp_lib_dev
	INCLUDES
		DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
		# COMPONENT cpp_lib_dev # component is not allowed for includes! Headers are installed separately with `install(FILES`! Includes only marks the headers for export
)
install(FILES ${${PROJECT_NAME}xx_hpp_files}
	DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
	COMPONENT cpp_lib_dev
)

install(EXPORT ${PROJECT_NAME}xx
	FILE "${PROJECT_NAME}xxConfig.cmake"
	NAMESPACE dropt::
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}xx
	COMPONENT cpp_lib_dev
)
configure_pkgconfig_file(${PROJECT_NAME}xx cpp_lib_dev)



set(test_${PROJECT_NAME}_c_files
    ${SrcDir}/test_${PROJECT_NAME}.c
)

add_executable(test_${PROJECT_NAME}
    ${test_${PROJECT_NAME}_c_files}
)

target_link_libraries(test_${PROJECT_NAME} ${PROJECT_NAME})

set(${PROJECT_NAME}_example_h_files
    ${IncludeDir}/${PROJECT_NAME}.h
    ${IncludeDir}/${PROJECT_NAME}_string.h
)

set(${PROJECT_NAME}_example_c_files
    ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}_example.c
)

add_executable(${PROJECT_NAME}_example
    ${${PROJECT_NAME}_example_c_files}
    ${${PROJECT_NAME}_example_h_files}
)

target_link_libraries(${PROJECT_NAME}_example ${PROJECT_NAME})

set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_VENDOR "${PROJECT_NAME} developers")
set(CPACK_PACKAGE_DESCRIPTION "${PROJECT_DESCRIPTION}")
set(CPACK_DEBIAN_PACKAGE_NAME "${CPACK_PACKAGE_NAME}")
set(CPACK_RPM_PACKAGE_NAME "${CPACK_PACKAGE_NAME}")
set(CPACK_PACKAGE_HOMEPAGE_URL "${PROJECT_HOMEPAGE_URL}")
set(CPACK_PACKAGE_MAINTAINER "${CPACK_PACKAGE_VENDOR}")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "${CPACK_PACKAGE_MAINTAINER}")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

set(CPACK_COMPONENTS_GROUPING IGNORE)
set(CPACK_DEB_COMPONENT_INSTALL ON)
set(CPACK_RPM_COMPONENT_INSTALL ON)
set(CPACK_NSIS_COMPONENT_INSTALL ON)
set(CPACK_DEBIAN_COMPRESSION_TYPE "xz")

set(SHARED_LIBS_COMPONENTS "c_lib;cpp_lib;")
set(CPACK_COMPONENTS_ALL "c_lib_dev;cpp_lib_dev")  # not populated automatically.
if("${BUILD_SHARED_LIBS}")
	set(CPACK_COMPONENTS_ALL "${CPACK_COMPONENTS_ALL};${SHARED_LIBS_COMPONENTS}")
endif()

include(CPack)
