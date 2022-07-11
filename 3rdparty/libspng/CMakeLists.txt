# ----------------------------------------------------------------------------
#  CMake file for libspng. See root CMakeLists.txt
#
# ----------------------------------------------------------------------------

project(${SPNG_LIBRARY} C)

ocv_include_directories("${CMAKE_CURRENT_SOURCE_DIR}" ${ZLIB_INCLUDE_DIRS})

file(GLOB lib_srcs *.c)
file(GLOB lib_hdrs *.h)

# ----------------------------------------------------------------------------------
#         Define the library target:
# ----------------------------------------------------------------------------------

if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
endif(MSVC)

add_library(${SPNG_LIBRARY} STATIC ${OPENCV_3RDPARTY_EXCLUDE_FROM_ALL} ${lib_srcs} ${lib_hdrs})
ocv_warnings_disable(CMAKE_C_FLAGS -Wunused-variable)
target_link_libraries(${SPNG_LIBRARY} ${ZLIB_LIBRARIES})

#ocv_warnings_disable(CMAKE_C_FLAGS -Wundef -Wcast-align -Wimplicit-fallthrough -Wunused-parameter -Wsign-compare
#    -Wmaybe-uninitialized
#    -Wnull-pointer-subtraction                   # clang15
#    -Wunused-but-set-variable                    # clang15
#)

set_target_properties(${SPNG_LIBRARY}
        PROPERTIES OUTPUT_NAME ${SPNG_LIBRARY}
        DEBUG_POSTFIX "${OPENCV_DEBUG_POSTFIX}"
        COMPILE_PDB_NAME ${SPNG_LIBRARY}
        COMPILE_PDB_NAME_DEBUG "${SPNG_LIBRARY}${OPENCV_DEBUG_POSTFIX}"
        ARCHIVE_OUTPUT_DIRECTORY ${3P_LIBRARY_OUTPUT_PATH}
        )

if(ENABLE_SOLUTION_FOLDERS)
    set_target_properties(${SPNG_LIBRARY} PROPERTIES FOLDER "3rdparty")
endif()

if(NOT BUILD_SHARED_LIBS)
    ocv_install_target(${SPNG_LIBRARY} EXPORT OpenCVModules ARCHIVE DESTINATION ${OPENCV_3P_LIB_INSTALL_PATH} COMPONENT dev OPTIONAL)
endif()

ocv_install_3rdparty_licenses(libspng LICENSE)