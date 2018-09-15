ocv_install_example_src(photo *.cpp *.hpp CMakeLists.txt)

set(OPENCV_PHOTO_SAMPLES_REQUIRED_DEPS
  opencv_core
  opencv_imgproc
  opencv_imgcodecs  
  opencv_photo
  opencv_highgui)
ocv_check_dependencies(${OPENCV_PHOTO_SAMPLES_REQUIRED_DEPS})

if(NOT BUILD_EXAMPLES OR NOT OCV_DEPENDENCIES_FOUND)
  return()
endif()

project(stitching_samples)
ocv_include_modules_recurse(${OPENCV_PHOTO_SAMPLES_REQUIRED_DEPS})
file(GLOB_RECURSE photo_samples RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.cpp)
foreach(sample_filename ${photo_samples})
  ocv_define_sample(tgt ${sample_filename} photo)
  ocv_target_link_libraries(${tgt} ${OPENCV_LINKER_LIBS} ${OPENCV_PHOTO_SAMPLES_REQUIRED_DEPS})
endforeach()
