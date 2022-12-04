#include "opencv2/core.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/photo.hpp"
#include <iostream>

using namespace std;
using namespace cv;

int main(int argc, char** argv)
{
    Mat img0 = imread("/home/travis/build/sturkmen72/opencv/source1.png");
    TickMeter tm;
    Mat inImage, SplicingImage;
    cv::resize(img0, inImage, Size(937, 937)); //Set the size of the input image
    tm.reset();
    tm.start();
    hconcat(inImage, inImage, SplicingImage);
    vconcat(SplicingImage, SplicingImage, SplicingImage);
    Mat mask = 255 * Mat::ones(inImage.rows, inImage.cols, inImage.depth());
    Point centerPt = Point(SplicingImage.cols / 2, SplicingImage.rows / 2);
    TickMeter tm1;
    tm1.start();
    seamlessClone(inImage, SplicingImage, mask, centerPt, SplicingImage, NORMAL_CLONE);
    tm1.stop();
    tm.stop();
    imwrite("cloned1.png", SplicingImage);
    cout << "seamlessClone time :" << tm1 << endl;
    cout << tm << endl;

    cv::resize(img0, inImage, Size(936, 936)); //Set the size of the input image
    tm.reset();
    tm.start();
    hconcat(inImage, inImage, SplicingImage);
    vconcat(SplicingImage, SplicingImage, SplicingImage);
    Mat mask2 = 255 * Mat::ones(inImage.rows, inImage.cols, inImage.depth());
    tm1.reset();
    tm1.start();
    seamlessClone(inImage, SplicingImage, mask2, centerPt, SplicingImage, NORMAL_CLONE);
    tm1.stop();
    tm.stop();
    cout << "seamlessClone time :" << tm1 << endl;
    cout << tm << endl;
    imwrite("cloned2.png", SplicingImage);
    return 0;
}
