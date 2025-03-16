import cv2 as cv

animation = cv.Animation()
success, animation = cv.imreadanimation("C:/images/apng-test/023-animated.png")

if success:
    animation.loop_count = 10
    cv.imwriteanimation("C:/images/apng-test/023-animated_ocv.png", animation)
    img = cv.imread("C:/projects/build/opencv/bin/Release/033.png",-1)
    cv.imwrite("C:/projects/build/opencv/bin/Release/033_0.png", img)
    print("animation saved")
else:
    print("Failed to load animation frames")
