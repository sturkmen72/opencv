#!/usr/bin/python

'''
This example illustrates how to use cv.imquery() class and imread() function.

Usage:
    imquery.py
'''

# Python 2/3 compatibility
from __future__ import print_function

import numpy as np
import cv2 as cv
import random as rng

rng.seed(cv.getTickCount())

def main():

    fn1 = cv.samples.findFileOrKeep('starry_night.jpg')
    fn2 = cv.samples.findFileOrKeep('squirrel_cls.jpg')

    img_info1 = cv.imquery(fn1)
    img_info2 = cv.imquery(fn2)

    if img_info1.page_count() > 0 and img_info2.page_count() > 0 :

        images = []
        img1 = cv.imread(fn1)
        img2 = cv.imread(fn2)

        for i in range(10):
            resized = cv.resize(img2, (rng.randint(240, 480), rng.randint(200, 400)), 0, 0)
            cv.putText(resized, str(i), (100,100), cv.FONT_HERSHEY_COMPLEX, 3, (0, 0, 255), 5)
            if i > 5:
                resized = cv.cvtColor( resized, cv.COLOR_BGR2GRAY)
            images.append(resized)
            cv.imwritemulti("images.tif",images)

        img_info3 = cv.imquery("images.tif")

        for i in range(10):

            print('image dimensions of page #', i, img_info3.width(i), img_info3.height(i))

            column = int((img_info1.width() - img_info3.width(i))/2)
            row = int((img_info1.height() - img_info3.height(i))/2)

            roi = img1[row:row + img_info3.height(i), column:column + img_info3.width(i)]
            _,roi = cv.imread("images.tif", roi, cv.IMREAD_REDUCED_COLOR_2, i)
            cv.imshow('roi', roi)
            cv.imshow('imread into roi', img1)
            cv.waitKey(0)
    else :
        print('there is a problem opening files, resultcode 1:', img_info1.result_code(), ', resultcode 2:', img_info2.result_code())

    print('Done')

if __name__ == '__main__':
    print(__doc__)
    main()
    cv.destroyAllWindows()
