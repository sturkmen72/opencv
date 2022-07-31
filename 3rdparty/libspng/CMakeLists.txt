# ----------------------------------------------------------------------------
#  CMake file for libspng. See root CMakeLists.txt
#
# ----------------------------------------------------------------------------

project(spng C)

set(CURR_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}")
set_property(GLOBAL PROPERTY SPNG_INCLUDE_DIR ${CURR_INCLUDE_DIR})
ocv_include_directories("${CMAKE_CURRENT_SOURCE_DIR}" ${ZLIB_INCLUDE_DIRS})

file(GLOB_RECURSE spng_headers RELATIVE "${CMAKE_CURRENT_LIST_DIR}" "*.h")
file(GLOB_RECURSE spng_sources RELATIVE "${CMAKE_CURRENT_LIST_DIR}" "*.c")

# ----------------------------------------------------------------------------------
#         Define the library target:
# ----------------------------------------------------------------------------------

if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
endif(MSVC)

add_library(${PROJECT_NAME} STATIC ${OPENCV_3RDPARTY_EXCLUDE_FROM_ALL} ${spng_headers} ${spng_sources})
ocv_warnings_disable(CMAKE_C_FLAGS -Wunused-variable)
target_link_libraries(${PROJECT_NAME} ${ZLIB_LIBRARIES})

#ocv_warnings_disable(CMAKE_C_FLAGS -Wundef -Wcast-align -Wimplicit-fallthrough -Wunused-parameter -Wsign-compare
#    -Wmaybe-uninitialized
#    -Wnull-pointer-subtraction                   # clang15
#    -Wunused-but-set-variable                    # clang15
#)

set_target_properties(${PROJECT_NAME}
        PROPERTIES OUTPUT_NAME ${PROJECT_NAME}
        DEBUG_POSTFIX "${OPENCV_DEBUG_POSTFIX}"
        COMPILE_PDB_NAME ${PROJECT_NAME}
        COMPILE_PDB_NAME_DEBUG "${PROJECT_NAME}${OPENCV_DEBUG_POSTFIX}"
        ARCHIVE_OUTPUT_DIRECTORY ${3P_LIBRARY_OUTPUT_PATH}
        )

if(ENABLE_SOLUTION_FOLDERS)
    set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "3rdparty")
endif()

if(NOT BUILD_SHARED_LIBS)
    ocv_install_target(${PROJECT_NAME} EXPORT OpenCVModules ARCHIVE DESTINATION ${OPENCV_3P_LIB_INSTALL_PATH} COMPONENT dev OPTIONAL)
endif()

ocv_install_3rdparty_licenses(${PROJECT_NAME} LICENSE)