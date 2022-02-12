//! [includes]
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>

#include <iostream>

using namespace cv;
//! [includes]

int main()
{
    //! [imread]
    String image_path = samples::findFile("starry_night.jpg");
    Mat img = imread(image_path, IMREAD_COLOR);
    //! [imread]

    //! [is_empty]
    if(img.empty())
    {
        std::cout << "Could not read the image: " << image_path << std::endl;
        return 1;
    }
    //! [is_empty]

    //! [imshow]
    imshow("Display window", img);
    int k = waitKey(0); // Wait for a keystroke in the window
    //! [imshow]

    //! [imwrite]
    if(k == 's')
    {
        imwrite("starry_night.png", img);
    }
    //! [imwrite]

    return 0;
}
