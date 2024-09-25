// This file is part of OpenCV project.
// It is subject to the license terms in the LICENSE file found in the top-level
// directory of this distribution and at http://opencv.org/license.html

#include "test_precomp.hpp"

namespace opencv_test { namespace {

/**
 * Test for check whether reading exif orientation tag was processed successfully or not
 * The test info is the set of 8 images named testExifRotate_{1 to 8}.png
 * The test image is the square 10x10 points divided by four sub-squares:
 * (R corresponds to Red, G to Green, B to Blue, W to white)
 * ---------             ---------
 * | R | G |             | G | R |
 * |-------| - (tag 1)   |-------| - (tag 2)
 * | B | W |             | W | B |
 * ---------             ---------
 *
 * ---------             ---------
 * | W | B |             | B | W |
 * |-------| - (tag 3)   |-------| - (tag 4)
 * | G | R |             | R | G |
 * ---------             ---------
 *
 * ---------             ---------
 * | R | B |             | G | W |
 * |-------| - (tag 5)   |-------| - (tag 6)
 * | G | W |             | R | B |
 * ---------             ---------
 *
 * ---------             ---------
 * | W | G |             | B | R |
 * |-------| - (tag 7)   |-------| - (tag 8)
 * | B | R |             | W | G |
 * ---------             ---------
 *
 *
 * Every image contains exif field with orientation tag (0x112)
 * After reading each image and applying the orientation tag,
 * the resulting image should be:
 * ---------
 * | R | G |
 * |-------|
 * | B | W |
 * ---------
 *
 */

typedef testing::TestWithParam<string> Imgcodecs_Exif;

TEST_P(Imgcodecs_Exif, exif_orientation)
{
    const string root = cvtest::TS::ptr()->get_data_path();
    const string filename = root + GetParam();
    const int colorThresholdHigh = 250;
    const int colorThresholdLow = 5;

    Mat m_img = imread(filename, IMREAD_COLOR | IMREAD_ANYCOLOR | IMREAD_ANYDEPTH);
    ASSERT_FALSE(m_img.empty());

    if (m_img.channels() == 3)
    {
        Vec3b vec;

        //Checking the first quadrant (with supposed red)
        vec = m_img.at<Vec3b>(2, 2); //some point inside the square
        EXPECT_LE(vec.val[0], colorThresholdLow);
        EXPECT_LE(vec.val[1], colorThresholdLow);
        EXPECT_GE(vec.val[2], colorThresholdHigh);

        //Checking the second quadrant (with supposed green)
        vec = m_img.at<Vec3b>(2, 7);  //some point inside the square
        EXPECT_LE(vec.val[0], colorThresholdLow);
        EXPECT_GE(vec.val[1], colorThresholdHigh);
        EXPECT_LE(vec.val[2], colorThresholdLow);

        //Checking the third quadrant (with supposed blue)
        vec = m_img.at<Vec3b>(7, 2);  //some point inside the square
        EXPECT_GE(vec.val[0], colorThresholdHigh);
        EXPECT_LE(vec.val[1], colorThresholdLow);
        EXPECT_LE(vec.val[2], colorThresholdLow);
    }
    else
    {
        Vec4b vec4;

        //Checking the first quadrant (with supposed red)
        vec4 = m_img.at<Vec4b>(2, 2); //some point inside the square
        EXPECT_LE(vec4.val[0], colorThresholdLow);
        EXPECT_LE(vec4.val[1], colorThresholdLow);
        EXPECT_GE(vec4.val[2], colorThresholdHigh);

        //Checking the second quadrant (with supposed green)
        vec4 = m_img.at<Vec4b>(2, 7);  //some point inside the square
        EXPECT_LE(vec4.val[0], colorThresholdLow);
        EXPECT_GE(vec4.val[1], colorThresholdHigh);
        EXPECT_LE(vec4.val[2], colorThresholdLow);

        //Checking the third quadrant (with supposed blue)
        vec4 = m_img.at<Vec4b>(7, 2);  //some point inside the square
        EXPECT_GE(vec4.val[0], colorThresholdHigh);
        EXPECT_LE(vec4.val[1], colorThresholdLow);
        EXPECT_LE(vec4.val[2], colorThresholdLow);
    }
}

const string exif_files[] =
{
#ifdef OPENCV_IMGCODECS_PNG_WITH_EXIF
    "readwrite/testExifOrientation_1.png",
    "readwrite/testExifOrientation_2.png",
    "readwrite/testExifOrientation_3.png",
    "readwrite/testExifOrientation_4.png",
    "readwrite/testExifOrientation_5.png",
    "readwrite/testExifOrientation_6.png",
    "readwrite/testExifOrientation_7.png",
    "readwrite/testExifOrientation_8.png"
#endif
};

INSTANTIATE_TEST_CASE_P(ExifFiles, Imgcodecs_Exif,
    testing::ValuesIn(exif_files));

}
}