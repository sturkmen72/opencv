// This file is part of OpenCV project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution and at http://opencv.org/license.html
#include "perf_precomp.hpp"

namespace opencv_test
{
    using namespace perf;

    typedef perf::TestBaseWithParam<std::string> PNG;

    PERF_TEST(PNG, decode)
    {
        String imagePath = getDataPath("cv/shared/lena.png");

        TEST_CYCLE() imread(imagePath);

        SANITY_CHECK_NOTHING();
    }

    PERF_TEST(PNG, encode)
    {
        String imagePath = getDataPath("cv/shared/lena.png");
        cv::Mat image = imread(imagePath);
        auto tempFile = cv::tempfile(".png");


        TEST_CYCLE() imwrite(tempFile, image);
        remove(tempFile.c_str());
        SANITY_CHECK_NOTHING();
    }

} // namespace
