#include "EDLib.h"
#include <iostream>

using namespace cv;
using namespace std;

namespace cv
{
    void testEDCircles(InputArray img, OutputArray circles)
    {
        Mat testImg;
        Mat colorImg = img.getMat();
        cvtColor(colorImg, testImg, COLOR_BGR2GRAY);

        EDCircles testEDCircles = EDCircles(testImg);
        vector<mCircle> mCircles = testEDCircles.getCircles();
        vector<Vec3i> _circles;
        for (int i = 0; i < mCircles.size(); i++)
            _circles.push_back(Vec3i((int)mCircles[i].center.x, (int)mCircles[i].center.y, (int)mCircles[i].r));
        int numCircles = (int)_circles.size();
        Mat(1, numCircles, cv::traits::Type<Vec3i>::value, &_circles[0]).copyTo(circles);
    }
}