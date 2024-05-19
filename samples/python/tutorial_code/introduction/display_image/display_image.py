## [imports]
import cv2 as cv
import sys
## [imports]
## [imread]
tm = cv.TickMeter()
tm.start()
img = cv.imread(cv.samples.findFile("animated-webp-supported.webp"))
tm.stop()
print(tm.getTimeSec())
## [imread]
## [empty]
if img is None:
    sys.exit("Could not read the image.")
## [empty]
## [imshow]
cv.imshow("Display window", img)
k = cv.waitKey(0)
## [imshow]
## [imsave]
if k == ord("s"):
    cv.imwrite("aanimated-webp-supported.webp", img)
## [imsave]
