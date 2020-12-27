#ifndef _OPENCV_OMR_H_
#define _OPENCV_OMR_H_
#ifdef __cplusplus

#include <opencv2/core.hpp>

namespace cv
{
    class CV_EXPORTS_W ContourWithData {
    public:
        int erosion_n = 1;
        int MIN_CONTOUR_AREA = 6;
        int MAX_CONTOUR_AREA = 300;
        std::vector<Point> contour;
        Rect boundingRect;
        RotatedRect rotRect;
        Point center;
        double area;
        int groupID;

        bool isContourValid()
        {
            float height(rotRect.size.height);
            float width(rotRect.size.width);
            float short_side = height < width ? height : width;
            float long_side = height > width ? height : width;
            short_side *= 2;

            return ((short_side / long_side > 0.4) & (area > MIN_CONTOUR_AREA));
        }

        static bool sortByBoundingRectCenterY(const ContourWithData& cwdLeft, const ContourWithData& cwdRight)
        {
            return (cwdLeft.center.x > cwdRight.center.x);
        }
        static bool sortByBoundingRectX(const ContourWithData& cwdLeft, const ContourWithData& cwdRight)
        {
            return ((cwdLeft.boundingRect.x + 100000 * (cwdLeft.boundingRect.y)) > (cwdRight.boundingRect.x + 100000 * (cwdRight.boundingRect.y)));
            //return ((cwdLeft.boundingRect.x + 10000 * (cwdLeft.boundingRect.y / 25 * 25)) > (cwdRight.boundingRect.x + 10000 * (cwdRight.boundingRect.y / 25 * 25)));
        }
        static bool sortByBoundingRectWidth(const ContourWithData& cwdLeft, const ContourWithData& cwdRight)
        {
            return ((cwdLeft.boundingRect.width - cwdLeft.boundingRect.height) > (cwdRight.boundingRect.width - cwdRight.boundingRect.height));
        }

        static bool sortByBoundingRectCenter(const ContourWithData& cwdLeft, const ContourWithData& cwdRight)
        {
            return ((cwdLeft.center.y + 10000 * (cwdLeft.center.x)) > (cwdRight.center.y + 10000 * (cwdRight.center.x)));
        }
    };

    class CV_EXPORTS_W HopeOMr
    {
    public:

        HopeOMr(InputOutputArray _src, bool dbg = false);
        CV_WRAP void drawRotatedRect(InputOutputArray src, RotatedRect rrect, Scalar color = Scalar(0, 255, 0), int thickness = 1);
        CV_WRAP bool getRects();
        CV_WRAP bool getRects2();
        CV_WRAP bool drawRects();
        CV_PROP RotatedRect RRect;
        CV_PROP std::vector<std::vector<Point>> vec_pts;
        CV_PROP Mat thresh0;
        CV_PROP Mat thresh1;
        CV_PROP bool debug;

    protected:
        Mat image;
        Mat gray;
        std::vector<ContourWithData> validContoursWithData;
        std::vector<Rect> rects;
        int erosion_n = 2;
        bool getRRectParts();
        RotatedRect getRRectPart(std::vector<Point> contour);
    };

    CV_EXPORTS_W bool doHopeOMr(InputOutputArray src, int method = 0);

    const static Scalar colors[] =
    {
        Scalar(0,0,0),       // 0 Black  
        Scalar(255,255,255), // 1 White
        Scalar(255,0,0),     // 2 Blue
        Scalar(0,255,0),     // 3 Lime
        Scalar(0,0,255),     // 4 Red
        Scalar(0,255,255),   // 5 Yellow
        Scalar(255,255,0),   // 6 Cyan
        Scalar(255,0,255),   // 7 Magenta
        Scalar(128,0,0),
        Scalar(0,128,0),
        Scalar(0,0,128),
        Scalar(0,128,128),
        Scalar(128,128,0),
        Scalar(128,0,128),
        Scalar(64,0,0),
        Scalar(0,64,0),
        Scalar(0,0,64),
        Scalar(0,64,64),
        Scalar(64,64,0),
        Scalar(64,0,64),
    };
}

#endif
#endif
