// This file is part of OpenCV project.
// It is subject to the license terms in the LICENSE file found in the top-level
// directory of this distribution and at http://opencv.org/license.html

#include "precomp.hpp"

#ifdef HAVE_PNG

/****************************************************************************************\
    This part of the file implements PNG codec on base of libpng library,
    in particular, this code is based on example.c from libpng
    (see otherlibs/_graphics/readme.txt for copyright notice)
    and png2bmp sample from libpng distribution (Copyright (C) 1999-2001 MIYASAKA Masaru)
\****************************************************************************************/

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

#ifndef _LFS64_LARGEFILE
#  define _LFS64_LARGEFILE 0
#endif
#ifndef _FILE_OFFSET_BITS
#  define _FILE_OFFSET_BITS 0
#endif

#include <png.h>
#include <zlib.h>

#include "grfmt_apng.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <array>
#include <vector>
#include <memory>
#include <string>
#include <utility>
#include <stdexcept>
#include <iomanip>

#if defined _MSC_VER && _MSC_VER >= 1200
    // interaction between '_setjmp' and C++ object destruction is non-portable
    #pragma warning( disable: 4611 )
    #pragma warning( disable: 4244 )
#endif

// the following defines are a hack to avoid multiple problems with frame pointer handling and setjmp
// see http://gcc.gnu.org/ml/gcc/2011-10/msg00324.html for some details
#define mingw_getsp(...) 0
#define __builtin_frame_address(...) 0


#define notabc(c) ((c) < 65 || (c) > 122 || ((c) > 90 && (c) < 97))

#define id_IHDR 0x52444849
#define id_acTL 0x4C546361
#define id_fcTL 0x4C546366
#define id_IDAT 0x54414449
#define id_fdAT 0x54416466
#define id_IEND 0x444E4549

namespace cv
{
    APNGFrame::APNGFrame()
        : m_pixels(NULL), m_width(0), m_height(0), m_color_type(0), m_paletteSize(0),
        m_transparencySize(0), m_delayNum(0), m_delayDen(0), m_rows(NULL)
    {
        memset(m_palette, 0, sizeof(m_palette));
        memset(m_transparency, 0, sizeof(m_transparency));
    }

    APNGFrame::APNGFrame(rgb* pixels, int width, int height,
        rgb* trns_color, int delayNum, int delayDen)
        : m_pixels(NULL), m_width(0), m_height(0), m_color_type(0), m_paletteSize(0),
        m_transparencySize(0), m_delayNum(delayNum), m_delayDen(delayDen), m_rows(NULL)
    {
        memset(m_palette, 0, sizeof(m_palette));
        memset(m_transparency, 0, sizeof(m_transparency));

        if (pixels != NULL) {
            png_uint_32 rowbytes = width * 3;

            m_width = width;
            m_height = height;
            m_color_type = PNG_COLOR_MASK_COLOR;

            m_pixels = new uchar[m_height * rowbytes];
            m_rows = new png_bytep[m_height * sizeof(png_bytep)];

            memcpy(m_pixels, pixels, m_height * rowbytes);

            for (int i = 0; i < m_height; ++i)
                m_rows[i] = m_pixels + i * rowbytes;

            if (trns_color != NULL) {
                m_transparency[0] = 0;
                m_transparency[1] = trns_color->r;
                m_transparency[2] = 0;
                m_transparency[3] = trns_color->g;
                m_transparency[4] = 0;
                m_transparency[5] = trns_color->b;
                m_transparencySize = 6;
            }
        }
    }

    APNGFrame::APNGFrame(rgba* pixels, int width, int height,
        int delayNum, int delayDen)
        : m_pixels(NULL), m_width(0), m_height(0), m_color_type(0), m_paletteSize(0),
        m_transparencySize(0), m_delayNum(delayNum), m_delayDen(delayDen), m_rows(NULL)
    {
        memset(m_palette, 0, sizeof(m_palette));
        memset(m_transparency, 0, sizeof(m_transparency));

        if (pixels != NULL) {
            png_uint_32 rowbytes = width * 4;

            m_width = width;
            m_height = height;
            m_color_type = PNG_COLOR_TYPE_RGB_ALPHA;

            m_pixels = new uchar[m_height * rowbytes];
            m_rows = new png_bytep[m_height * sizeof(png_bytep)];

            memcpy(m_pixels, pixels, m_height * rowbytes);

            for (int i = 0; i < m_height; ++i)
                m_rows[i] = m_pixels + i * rowbytes;
        }
    }

    APNGFrame::APNGFrame(const std::string& filePath, int delayNum, int delayDen)
        : m_pixels(NULL), m_width(0), m_height(0), m_color_type(0), m_paletteSize(0),
        m_transparencySize(0), m_delayNum(delayNum), m_delayDen(delayDen), m_rows(NULL)
    {
        memset(m_palette, 0, sizeof(m_palette));
        memset(m_transparency, 0, sizeof(m_transparency));
        // TODO save extracted info to self
        FILE* f;
        if ((f = fopen(filePath.c_str(), "rb")) != 0) {
            uchar sig[8];

            if (fread(sig, 1, 8, f) == 8 && png_sig_cmp(sig, 0, 8) == 0) {
                png_structp png_ptr =
                    png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
                png_infop info_ptr = png_create_info_struct(png_ptr);
                if (png_ptr && info_ptr) {
                    png_byte depth;
                    png_uint_32 rowbytes;
                    png_colorp palette;
                    png_color_16p trans_color;
                    png_bytep trans_alpha;

                    if (setjmp(png_jmpbuf(png_ptr))) {
                        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
                        fclose(f);
                        return;
                    }

                    png_init_io(png_ptr, f);
                    png_set_sig_bytes(png_ptr, 8);
                    png_read_info(png_ptr, info_ptr);
                    m_width = png_get_image_width(png_ptr, info_ptr);
                    m_height = png_get_image_height(png_ptr, info_ptr);
                    m_color_type = png_get_color_type(png_ptr, info_ptr);
                    depth = png_get_bit_depth(png_ptr, info_ptr);
                    if (depth < 8) {
                        if (m_color_type == PNG_COLOR_TYPE_PALETTE)
                            png_set_packing(png_ptr);
                        else
                            png_set_expand(png_ptr);
                    }
                    else if (depth > 8) {
                        png_set_expand(png_ptr);
                        png_set_strip_16(png_ptr);
                    }
                    (void)png_set_interlace_handling(png_ptr);
                    png_read_update_info(png_ptr, info_ptr);
                    m_color_type = png_get_color_type(png_ptr, info_ptr);
                    rowbytes = (int)png_get_rowbytes(png_ptr, info_ptr);
                    memset(m_palette, 255, sizeof(m_palette));
                    memset(m_transparency, 255, sizeof(m_transparency));

                    if (png_get_PLTE(png_ptr, info_ptr, &palette, &m_paletteSize))
                        memcpy(m_palette, palette, m_paletteSize * 3);
                    else
                        m_paletteSize = 0;

                    if (png_get_tRNS(png_ptr, info_ptr, &trans_alpha, &m_transparencySize,
                        &trans_color)) {
                        if (m_transparencySize > 0) {
                            if (m_color_type == PNG_COLOR_TYPE_GRAY) {
                                m_transparency[0] = 0;
                                m_transparency[1] = trans_color->gray & 0xFF;
                                m_transparencySize = 2;
                            }
                            else if (m_color_type == PNG_COLOR_TYPE_RGB) {
                                m_transparency[0] = 0;
                                m_transparency[1] = trans_color->red & 0xFF;
                                m_transparency[2] = 0;
                                m_transparency[3] = trans_color->green & 0xFF;
                                m_transparency[4] = 0;
                                m_transparency[5] = trans_color->blue & 0xFF;
                                m_transparencySize = 6;
                            }
                            else if (m_color_type == PNG_COLOR_TYPE_PALETTE)
                                memcpy(m_transparency, trans_alpha, m_transparencySize);
                            else
                                m_transparencySize = 0;
                        }
                    }
                    else
                        m_transparencySize = 0;

                    m_pixels = new uchar[m_height * rowbytes];
                    m_rows = new png_bytep[m_height * sizeof(png_bytep)];

                    for (int i = 0; i < m_height; ++i)
                        m_rows[i] = m_pixels + i * rowbytes;

                    png_read_image(png_ptr, m_rows);
                    png_read_end(png_ptr, NULL);
                }
                png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
            }
            fclose(f);
        }
    }
    /*
    APNGFrame::APNGFrame(Mat src, int delayNum, int delayDen)
    {
        memset(m_palette, 0, sizeof(m_palette));
        memset(m_transparency, 0, sizeof(m_transparency));

        if (!src.empty()
        {
            png_uint_32 rowbytes = width * 4;

                m_width = src.cols;
                m_height = src.rows;
                m_color_type = PNG_COLOR_TYPE_RGB_ALPHA;

                m_pixels = new uchar[m_height * rowbytes];
                m_rows = new png_bytep[m_height * sizeof(png_bytep)];

                memcpy(m_pixels, pixels, m_height * rowbytes);

            for (int i = 0; i < m_height; ++i)
                m_rows[i] = m_pixels + i * rowbytes;
        }

    }
    */


    uchar* APNGFrame::pixels(uchar* setPixels)
    {
        if (setPixels != NULL)
            m_pixels = setPixels;
        return m_pixels;
    }

    rgb* APNGFrame::palette(rgb* setPalette)
    {
        if (setPalette != NULL)
            memcpy(m_palette, setPalette,
                std::min(sizeof(m_palette), sizeof(setPalette)));
        return m_palette;
    }

    uchar* APNGFrame::transparency(uchar* setTransparency)
    {
        if (setTransparency != NULL)
            memcpy(m_transparency, setTransparency,
                std::min(sizeof(m_transparency), sizeof(setTransparency)));
        return m_transparency;
    }

    int APNGFrame::paletteSize(int setPaletteSize)
    {
        if (setPaletteSize != 0)
            m_paletteSize = setPaletteSize;
        return m_paletteSize;
    }

    int APNGFrame::transparencySize(int setTransparencySize)
    {
        if (setTransparencySize != 0)
            m_transparencySize = setTransparencySize;
        return m_transparencySize;
    }

    int APNGFrame::delayNum(int setDelayNum)
    {
        if (setDelayNum != 0)
            m_delayNum = setDelayNum;
        return m_delayNum;
    }

    int APNGFrame::delayDen(int setDelayDen)
    {
        if (setDelayDen != 0)
            m_delayDen = setDelayDen;
        return m_delayDen;
    }

    uchar** APNGFrame::rows(uchar** setRows)
    {
        if (setRows != NULL)
            m_rows = setRows;
        return m_rows;
    }

    // Save frame to a PNG image file.
    // Return true if save succeeded.
    bool APNGFrame::save(const std::string& outPath) const
    {
        FILE* f;
        if ((f = fopen(outPath.c_str(), "wb")) != 0) {
            png_structp png_ptr =
                png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
            png_infop info_ptr = png_create_info_struct(png_ptr);
            if (png_ptr && info_ptr) {
                if (setjmp(png_jmpbuf(png_ptr))) {
                    png_destroy_read_struct(&png_ptr, &info_ptr, 0);
                    fclose(f);
                    return false;
                }
                png_init_io(png_ptr, f);
                png_set_compression_level(png_ptr, 9);
                png_set_IHDR(png_ptr, info_ptr, m_width, m_height, 8, m_color_type, 0, 0, 0);
                if (m_paletteSize > 0) {
                    png_color palette[PNG_MAX_PALETTE_LENGTH];
                    memcpy(palette, m_palette, m_paletteSize * 3);
                    png_set_PLTE(png_ptr, info_ptr, palette, m_paletteSize);
                }
                if (m_transparencySize > 0) {
                    png_color_16 trans_color;
                    if (m_color_type == PNG_COLOR_TYPE_GRAY) {
                        trans_color.gray = m_transparency[1];
                        png_set_tRNS(png_ptr, info_ptr, NULL, 0, &trans_color);
                    }
                    else if (m_color_type == PNG_COLOR_TYPE_RGB) {
                        trans_color.red = m_transparency[1];
                        trans_color.green = m_transparency[3];
                        trans_color.blue = m_transparency[5];
                        png_set_tRNS(png_ptr, info_ptr, NULL, 0, &trans_color);
                    }
                    else if (m_color_type == PNG_COLOR_TYPE_PALETTE)
                        png_set_tRNS(png_ptr, info_ptr,
                            const_cast<uchar*>(m_transparency),
                            m_transparencySize, NULL);
                }
                png_write_info(png_ptr, info_ptr);
                png_set_bgr(png_ptr);
                png_write_image(png_ptr, m_rows);
                png_write_end(png_ptr, info_ptr);
            }
            else
                return false;
            png_destroy_write_struct(&png_ptr, &info_ptr);
            fclose(f);
            return true;
        }

        return false;
    }

}

#endif

/* End of file. */
