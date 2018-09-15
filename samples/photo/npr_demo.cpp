/*
* npr_demo.cpp
*
* Author:
* Siddharth Kherada <siddharthkherada27[at]gmail[dot]com>
*
* This tutorial demonstrates how to use OpenCV Non-Photorealistic Rendering Module.
* 1) Edge Preserve Smoothing
*    -> Using Normalized convolution Filter
*    -> Using Recursive Filter
* 2) Detail Enhancement
* 3) Pencil sketch/Color Pencil Drawing
* 4) Stylization
*
*/

#include "opencv2/opencv_modules.hpp"
#include "opencv2/photo.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/imgcodecs.hpp"

#ifdef HAVE_OPENCV_HIGHGUI
#include "opencv2/highgui.hpp"
#endif

#include <iostream>

using namespace std;
using namespace cv;

int main(int argc, char* argv[])
{
    cv::CommandLineParser parser(argc, argv, "{help h||show help message}{@image|../data/lena.jpg|input image}");
    if (parser.has("help"))
    {
        parser.printMessage();
        exit(0);
    }
    if (parser.get<string>("@image").empty())
    {
        parser.printMessage();
        exit(0);
    }

    Mat I = imread(parser.get<string>("@image"));

    int num,type;

    if(I.empty())
    {
        cout <<  "Image not found" << endl;
        exit(0);
    }

    cout << endl;
    cout << " Edge Preserve Filter" << endl;
    cout << "----------------------" << endl;

    cout << "Options: " << endl;
    cout << endl;

    cout << "1) Edge Preserve Smoothing" << endl;
    cout << "   -> Using Normalized convolution Filter" << endl;
    cout << "   -> Using Recursive Filter" << endl;
    cout << "2) Detail Enhancement" << endl;
    cout << "3) Pencil sketch/Color Pencil Drawing" << endl;
    cout << "4) Stylization" << endl;
    cout << endl;

    cout << "Press number 1-4 to choose from above techniques: ";

    cin >> num;

    Mat img;

    if(num == 1)
    {
        cout << endl;
        cout << "Press 1 for Normalized Convolution Filter and 2 for Recursive Filter: ";

        cin >> type;

        edgePreservingFilter(I,img,type);
        
         #ifdef HAVE_OPENCV_HIGHGUI
        imshow("Edge Preserve Smoothing", img);
        #else
        imwrite("Edge_Preserve_Smoothing.jpg", img);
        #endif
    }
    else if(num == 2)
    {
        detailEnhance(I,img);
        #ifdef HAVE_OPENCV_HIGHGUI
        imshow("Detail Enhanced", img);
        #else
        imwrite("Detail_Enhanced.jpg", img);
        #endif
        
    }
    else if(num == 3)
    {
        Mat img1;
        pencilSketch(I,img1, img, 10 , 0.1f, 0.03f);

        #ifdef HAVE_OPENCV_HIGHGUI
        imshow("Pencil Sketch", img1);
        imshow("Color Pencil Sketch", img);
        #else
        imwrite("Pencil_Sketch.jpg", img1);
        imwrite("Color_Pencil_Sketch.jpg", img);
        #endif
    }
    else if(num == 4)
    {
        stylization(I,img);
        #ifdef HAVE_OPENCV_HIGHGUI
        imshow("Stylization", img);
        #else
        imwrite("Stylization.jpg", img);
        #endif
        
    }
    waitKey(0);
}
