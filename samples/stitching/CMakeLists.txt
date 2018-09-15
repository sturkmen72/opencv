ocv_install_example_src(stitching *.cpp *.hpp CMakeLists.txt)

set(OPENCV_STITCHING_SAMPLES_REQUIRED_DEPS
  opencv_core
  opencv_imgproc
  opencv_imgcodecs
  opencv_videoio
  opencv_highgui
  opencv_stitching)
ocv_check_dependencies(${OPENCV_STITCHING_SAMPLES_REQUIRED_DEPS})

if(NOT BUILD_EXAMPLES OR NOT OCV_DEPENDENCIES_FOUND)
  return()
endif()

project(stitching_samples)
ocv_include_modules_recurse(${OPENCV_STITCHING_SAMPLES_REQUIRED_DEPS})
file(GLOB_RECURSE stitching_samples RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.cpp)
foreach(sample_filename ${stitching_samples})
  ocv_define_sample(tgt ${sample_filename} stitching)
  ocv_target_link_libraries(${tgt} ${OPENCV_LINKER_LIBS} ${OPENCV_STITCHING_SAMPLES_REQUIRED_DEPS})
endforeach()
