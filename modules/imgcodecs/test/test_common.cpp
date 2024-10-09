// This file is part of OpenCV project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution and at http://opencv.org/license.html
#include "test_precomp.hpp"
#include "test_common.hpp"

namespace opencv_test {

static
Mat generateTestImageBGR_()
{
    Size sz(320, 240);
    Mat result(sz, CV_8UC3, Scalar::all(0));  // Create a black image

    // Load the image
    const string fname = cvtest::findDataFile("../cv/shared/baboon.png");
    Mat image = imread(fname, IMREAD_REDUCED_COLOR_2);
    CV_Assert(!image.empty());  // Ensure the image is loaded
    CV_CheckEQ(image.size(), Size(256, 256), "Image size mismatch");

    // Define the ROI to copy the baboon image into
    Rect roi((320 - 256) / 2, 0, 256, 240);
    image(Rect(0, 0, 256, 240)).copyTo(result(roi));  // Copy the image to result

    // Add colored squares
    result(Rect(0,  0, 5, 5)).setTo(Scalar(0, 0, 255));  // Red
    result(Rect(5,  0, 5, 5)).setTo(Scalar(0, 255, 0));  // Green
    result(Rect(10, 0, 5, 5)).setTo(Scalar(255, 0, 0));  // Blue
    result(Rect(0,  5, 5, 5)).setTo(Scalar(128, 128, 128));  // Gray

#if 0
    // Display the result
    imshow("test_image", result); 
    waitKey();  // Wait for a key press
#endif

    return result;  // Return the generated image
}
Mat generateTestImageBGR()
{
    static Mat image = generateTestImageBGR_();  // initialize once
    CV_Assert(!image.empty());
    return image;
}

static
Mat generateTestImageGrayscale_()
{
    Mat imageBGR = generateTestImageBGR();
    CV_Assert(!imageBGR.empty());

    Mat result;
    cvtColor(imageBGR, result, COLOR_BGR2GRAY);
    return result;
}
Mat generateTestImageGrayscale()
{
    static Mat image = generateTestImageGrayscale_();  // initialize once
    return image;
}

}  // namespace
