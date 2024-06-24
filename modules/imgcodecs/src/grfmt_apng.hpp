// This file is part of OpenCV project.
// It is subject to the license terms in the LICENSE file found in the top-level
// directory of this distribution and at http://opencv.org/license.html

/****************************************************************************\
 *
 *  this file includes some modified part of apngasm and APNG Optimizer 1.4
 *  both have zlib license.
 *
 ****************************************************************************/


 /*  apngasm
 *
 *  The next generation of apngasm, the APNG Assembler.
 *  The apngasm CLI tool and library can assemble and disassemble APNG image files.
 *
 *  https://github.com/apngasm/apngasm


 /* APNG Optimizer 1.4
 *
 * Makes APNG files smaller.
 *
 * http://sourceforge.net/projects/apng/files
 *
 * Copyright (c) 2011-2015 Max Stepin
 * maxst at users.sourceforge.net
 *
 * zlib license
 * ------------
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 */

#ifndef _GRFMT_APNG_H_
#define _GRFMT_APNG_H_

#ifdef HAVE_PNG

#include "grfmt_base.hpp"
#include "bitstrm.hpp"
#include <png.h>
#include <zlib.h>

namespace cv
{

const uint DEFAULT_FRAME_NUMERATOR = 100; //!< @brief The default numerator for the frame delay fraction.
const uint DEFAULT_FRAME_DENOMINATOR =  1000; //!< @brief The default denominator for the frame delay fraction.

typedef struct {
    uchar r, g, b;
} rgb;

typedef struct {
    uchar r, g, b, a;
} rgba;


class APNGFrame
{
public:
    APNGFrame();
    virtual ~APNGFrame() {}

    /**
     * @brief Creates an APNGFrame from a bitmapped array of RBGA pixel data.
     * @param pixels The RGBA pixel data.
     * @param width The width of the pixel data.
     * @param height The height of the pixel data.
     * @param delayNum The delay numerator for this frame (defaults to DEFAULT_FRAME_NUMERATOR).
     * @param delayDen The delay denominator for this frame (defaults to DEFAULT_FRAME_DENMINATOR).
     */
    APNGFrame(rgba* pixels, int width, int height,
        int delayNum = DEFAULT_FRAME_NUMERATOR,
        int delayDen = DEFAULT_FRAME_DENOMINATOR);

    /**
     * @brief Creates an APNGFrame from a PNG file.
     * @param filePath The relative or absolute path to an image file.
     * @param delayNum The delay numerator for this frame (defaults to DEFAULT_FRAME_NUMERATOR).
     * @param delayDen The delay denominator for this frame (defaults to DEFAULT_FRAME_DENMINATOR).
     */
    APNGFrame(const std::string& filePath,
        int delayNum = DEFAULT_FRAME_NUMERATOR,
        int delayDen = DEFAULT_FRAME_DENOMINATOR);

    /**
     * @brief Creates an APNGFrame from a bitmapped array of RBG pixel data.
     * @param pixels The RGB pixel data.
     * @param width The width of the pixel data.
     * @param height The height of the pixel data.
     * @param delayNum The delay numerator for this frame (defaults to DEFAULT_FRAME_NUMERATOR).
     * @param delayDen The delay denominator for this frame (defaults to DEFAULT_FRAME_DENMINATOR).
     */
    APNGFrame(rgb* pixels, int width, int height,
        int delayNum = DEFAULT_FRAME_NUMERATOR,
        int delayDen = DEFAULT_FRAME_DENOMINATOR);

    /**
     * @brief Creates an APNGFrame from a bitmapped array of RBG pixel data.
     * @param pixels The RGB pixel data.
     * @param width The width of the pixel data.
     * @param height The height of the pixel data.
     * @param trns_color An array of transparency data.
     * @param delayNum The delay numerator for this frame (defaults to DEFAULT_FRAME_NUMERATOR).
     * @param delayDen The delay denominator for this frame (defaults to DEFAULT_FRAME_DENMINATOR).
     */
    APNGFrame(rgb* pixels, int width, int height,
        rgb* trns_color = NULL, int delayNum = DEFAULT_FRAME_NUMERATOR,
        int delayDen = DEFAULT_FRAME_DENOMINATOR);

    /**
     * @brief Saves this frame as a single PNG file.
     * @param outPath The relative or absolute path to save the image file to.
     * @return Returns true if save was successful.
     */
    bool save(const std::string& outPath) const;

    int width() const { return m_width; }
    int height() const { return m_height; }
    virtual int type() const { return m_type; }

    // Raw pixel data
    uchar* pixels(uchar* setPixels = NULL);

    // Palette into
    rgb* palette(rgb* setPalette = NULL);
    rgb _palette[256];

    // Transparency info
    uchar* transparency(uchar* setTransparency = NULL);

    // Sizes for palette and transparency records
    int paletteSize(int setPaletteSize = 0);
    int transparencySize(int setTransparencySize = 0);

    // Delay is numerator/denominator ratio, in seconds
    int delayNum(int setDelayNum = 0);
    int delayDen(int setDelayDen = 0);
    uchar** rows(uchar** setRows = NULL);


protected:
    uchar* m_pixels;
    int  m_width;
    int  m_height;
    int  m_type;
    int  m_color_type;
    int _paletteSize;
    int _transparencySize;
    uchar _transparency[256];
    int _delayNum;
    int _delayDen;
    uchar** _rows;
};

}

#endif

#endif/*_GRFMT_PNG_H_*/
