/*M///////////////////////////////////////////////////////////////////////////////////////
//
//  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
//
//  By downloading, copying, installing or using the software you agree to this license.
//  If you do not agree to this license, do not download, install,
//  copy or use the software.
//
//
//                           License Agreement
//                For Open Source Computer Vision Library
//
// Copyright (C) 2000-2008, Intel Corporation, all rights reserved.
// Copyright (C) 2009, Willow Garage Inc., all rights reserved.
// Third party copyrights are property of their respective owners.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//   * Redistribution's of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//
//   * Redistribution's in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
//   * The name of the copyright holders may not be used to endorse or promote products
//     derived from this software without specific prior written permission.
//
// This software is provided by the copyright holders and contributors "as is" and
// any express or implied warranties, including, but not limited to, the implied
// warranties of merchantability and fitness for a particular purpose are disclaimed.
// In no event shall the Intel Corporation or contributors be liable for any direct,
// indirect, incidental, special, exemplary, or consequential damages
// (including, but not limited to, procurement of substitute goods or services;
// loss of use, data, or profits; or business interruption) however caused
// and on any theory of liability, whether in contract, strict liability,
// or tort (including negligence or otherwise) arising in any way out of
// the use of this software, even if advised of the possibility of such damage.
//
//M*/

#include "precomp.hpp"

namespace cv
{
    struct point_sorter_x_asc // sorts points by their x ascending
    {
        bool operator ()(const Point& a, const Point& b)
        {
            return a.x < b.x;
        }
    };

    struct contour_sorter // 'less' for contours
    {
        bool operator ()(const std::vector<Point>& a, const std::vector<Point>& b)
        {
            Rect ra(boundingRect(a));
            Rect rb(boundingRect(b));
            // scale factor for y should be larger than img.width
            return ((ra.x + 1000 * ra.y) < (rb.x + 1000 * rb.y));
        }
    };

    static void adjustRotatedRect(RotatedRect& rrect)
    {
        if (rrect.angle < -45.)
        {
            rrect.angle += 90.0;
            swap(rrect.size.width, rrect.size.height);
        }
    }

    static Point2f getCentroid(InputArray Points)
    {
        Moments mm = moments(Points, false);

        Point2f Coord = Point2f(static_cast<float>(mm.m10 / (mm.m00 + 1e-5)),
            static_cast<float>(mm.m01 / (mm.m00 + 1e-5)));
        return Coord;
    }


    HopeOMr::HopeOMr(InputOutputArray src, bool dbg)
    {
        debug = dbg;
        image = src.getMat();
		gray.create(image.size(), CV_8U);
        thresh0.create(image.size(), CV_8U);
        thresh1.create(image.size(), CV_8U);
        cvtColor(image, gray, COLOR_BGR2GRAY);
    }

    void HopeOMr::drawRotatedRect(InputOutputArray src, RotatedRect rrect, Scalar color, int thickness)
    {
        Point2f vtx[4];
        rrect.points(vtx);
        for (int i = 0; i < 4; i++)
            line(src, vtx[i], vtx[(i + 1) % 4], color, thickness);
    }

    bool getRects2helper(Mat& cwindow, Mat& gwindow, Mat& twindow)
    {
        cvtColor(cwindow, gwindow, COLOR_BGR2GRAY);
        adaptiveThreshold(gwindow, twindow, 255, ADAPTIVE_THRESH_MEAN_C, THRESH_BINARY_INV, 11, 10);

        std::vector<std::vector<Point> > contours;
        findContours(twindow, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

        for (size_t i = 0; i < contours.size(); i++)
        {
            polylines(cwindow, contours[i], false, Scalar(0, 0, 255), 1);
        }
        return true;
    }

    bool HopeOMr::getRects2()
    {
        Rect center_rect(image.cols / 2, image.rows / 2, 60, 60);
        Mat cwindow = image(center_rect);
        Mat gwindow = gray(center_rect);
        Mat twindow = thresh0(center_rect);

        getRects2helper(cwindow, gwindow, twindow);
        //cwindow.setTo(Scalar(255, 0, 0), twindow);
        return true;
     }

    bool HopeOMr::getRects()
    {
        Point pt;
        adaptiveThreshold(gray, thresh0, 255, ADAPTIVE_THRESH_MEAN_C, THRESH_BINARY_INV, 31, 10);
        erode(thresh0, thresh1, Mat(), Point(-1, -1), erosion_n);

        std::vector<std::vector<Point> > contours;
        findContours(thresh1, contours, RETR_TREE, CHAIN_APPROX_SIMPLE);

        Point2f vtx[4];

        for (size_t i = 0; i < contours.size(); i++)
        {
            ContourWithData contourWithData;
            contourWithData.contour = contours[i];
            contourWithData.boundingRect = boundingRect(contourWithData.contour);
            contourWithData.rotRect = minAreaRect(contours[i]);
            contourWithData.center = contourWithData.rotRect.center;

            contourWithData.area = contourWithData.boundingRect.width * contourWithData.boundingRect.height;

            if (contourWithData.isContourValid())
            {
                contourWithData.groupID = 1;
                validContoursWithData.push_back(contourWithData);
            }
        }

        Point chain_pt0;
        if (validContoursWithData.size() > 0)
            chain_pt0 = validContoursWithData[0].center;

        int avgdim = thresh1.cols / 90;
        int mindim = cvCeil(avgdim * 0.6);
        int maxdim = avgdim * 3;

        std::vector<Point> pts;

        for (size_t i = 0; i < validContoursWithData.size(); i++)
        {
            if (debug)
                polylines(image, validContoursWithData[i].contour, true, colors[6], 1);

            Rect r = validContoursWithData[i].rotRect.boundingRect();

            int rdim = abs(r.width - r.height) < r.width / 2 ? (r.width + r.height) / 2 : 0;

            if ((rdim > mindim) && (rdim < maxdim))
            {
                Point center_of_rect = (r.br() + r.tl()) * 0.5;
                pts.push_back(center_of_rect);
                r.x = center_of_rect.x - (rdim / 2);
                r.y = center_of_rect.y - (rdim / 2);
                r.width = r.height = rdim;
                pts.push_back(r.br());
                pts.push_back(r.tl());
                rects.push_back(r);

                if (debug)
                    polylines(image, validContoursWithData[i].contour, true, colors[5], 1);
            }
        }


        if (pts.size() > 60)
        {
            RRect = minAreaRect(pts);
            adjustRotatedRect(RRect);
            return true;
        }

        return false;
    }

    bool HopeOMr::drawRects()
    {
        return getRRectParts();
    }

    RotatedRect HopeOMr::getRRectPart(std::vector<Point> contour)
    {
        //Point contour_center = (contour[0] + contour[2]) * 0.5;
        std::vector<Point> pts;
        std::vector<Point> hull;
        std::vector <std::vector<Point>> hullpts;

        for (size_t i = 0; i < rects.size(); i++)
        {
            double d = pointPolygonTest(contour, rects[i].br(), false);


            if (d > 0)
            {
                Point pt_center = (rects[i].br() + rects[i].tl()) * 0.5;
                pts.push_back(pt_center);
            }
        }

        vec_pts.push_back(pts);

        if (pts.size() > 0)
        {
            convexHull(pts, hull, true);
            Point center_pt = getCentroid(hull);

            double min_dist = INT16_MAX;
            Point nearest_pt;

            for (size_t i = 0; i < pts.size(); i++)
            {
                double dist = norm(center_pt - pts[i]);
                if (dist < min_dist)
                {
                    min_dist = dist;
                    nearest_pt = pts[i];
                }
            }

            center_pt = nearest_pt;
            min_dist = 0;
            for (size_t i = 0; i < hull.size(); i++)
            {
                double dist = norm(center_pt - pts[i]);
                if (dist > min_dist)
                {
                    min_dist = dist;
                    nearest_pt = pts[i];
                }
            }

            RotatedRect r = minAreaRect(hull);
            adjustRotatedRect(r);

            r.size.width = (float)(r.size.width * 1.03);
            r.size.height = (float)(r.size.height * 1.15);

            if (debug)
                drawRotatedRect(image, r);

            return r;
        }
        return RotatedRect();
    }

    bool HopeOMr::getRRectParts()
    {
        bool result = false;
        std::vector<Point> contour4;
        std::vector<Point> contour3;
        std::vector<Point> contour2;
        std::vector<Point> contour1;

        RotatedRect RRect2 = RRect;

        Point2f vtx[4];
        RRect.size += Size2f(10, 10);
        RRect.size.height += 10;
        RRect.size.width += 10;

        RRect.points(vtx);

        contour4.push_back(vtx[1]);
        contour4.push_back(vtx[2]);

        contour1.push_back(vtx[0]);
        contour1.push_back(vtx[3]);

        RRect.size.height /= 2;
        RRect.size.height += 5;
        RRect.points(vtx);

        contour4.push_back(vtx[2]);
        contour4.push_back(vtx[1]);

        contour1.push_back(vtx[3]);
        contour1.push_back(vtx[0]);

        contour2.push_back(vtx[3]);
        contour2.push_back(vtx[0]);

        contour3.push_back(vtx[2]);
        contour3.push_back(vtx[1]);

        RRect.size.height = 4;
        RRect.points(vtx);

        contour2.push_back(vtx[0]);
        contour2.push_back(vtx[3]);

        contour3.push_back(vtx[1]);
        contour3.push_back(vtx[2]);

        std::vector<RotatedRect> rr;
        rr.push_back(getRRectPart(contour4));
        rr.push_back(getRRectPart(contour3));
        rr.push_back(getRRectPart(contour2));
        rr.push_back(getRRectPart(contour1));

        float h1 = rr[0].size.height;
        float h2 = rr[1].size.height;
        float h3 = rr[2].size.height;
        float h4 = rr[3].size.height;

        float a1 = rr[0].angle;
        float a2 = rr[1].angle;
        float a3 = rr[2].angle;
        float a4 = rr[3].angle;

        int found_circle_count = 0;

        if ((abs(h1 + h2 - h3 - h4) < 10) & (abs(a1 + a2 - a3 - a4) < 3))
        {

            for (int i = 0; i < 4; i++)
            {
                Rect crect = rr[i].boundingRect() & Rect(0, 0, image.cols, image.rows);

                Mat br1 = thresh0(crect);
                Mat display = image(crect);

                int mindim = br1.cols / 60;
                int maxdim = mindim * 2;

                std::vector<std::vector<Point> > contours;

                findContours(br1, contours, RETR_TREE, CHAIN_APPROX_SIMPLE);

                Mat m(display.size(), CV_8U, Scalar(0));

                for (size_t j = 0; j < contours.size(); j++)
                {
                    Rect r = boundingRect(contours[j]);

                    int rdim = abs(r.width - r.height) < r.width / 2 ? (r.width + r.height) / 2 : 0;

                    if ((rdim > mindim) && (rdim < maxdim))
                    {
                        rectangle(m, r, Scalar(255), -1);
                    }
                }

                findContours(m, contours, RETR_TREE, CHAIN_APPROX_SIMPLE);
                std::sort(contours.begin(), contours.end(), contour_sorter());

                if (contours.size() > 150)
                {
                    drawRotatedRect(image, rr[i], colors[3], 2);
                    found_circle_count++;
                }
            }
            result = found_circle_count > 3;
        }
        return result;
    }

    bool doHopeOMr(InputOutputArray src, int method)
    {
        Mat img = src.getMat();
        int x = img.cols / 10;
        int y = img.rows / 8;
        Rect c_area(x, y, img.cols - (x * 2), img.rows - (y * 2));

        Mat image = img(c_area);
        HopeOMr omr(image);
        omr.debug = method;

        image = image + Scalar(15, 20, 50);

        if (omr.getRects())
        {
            return omr.drawRects();
        }

        return false;
    }
}
