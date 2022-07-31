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

#ifdef HAVE_SPNG

/****************************************************************************************\
    This part of the file implements PNG codec on base of libspng library,
    in particular, this code is based on example.c from libspng
    (see 3rdparty/libspng/LICENSE for copyright notice)
\****************************************************************************************/

#ifndef _LFS64_LARGEFILE
#  define _LFS64_LARGEFILE 0
#endif
#ifndef _FILE_OFFSET_BITS
#  define _FILE_OFFSET_BITS 0
#endif

#include <spng.h>
#include <zlib.h>

#include "grfmt_spng.hpp"

namespace cv
{

/////////////////////// PngDecoder ///////////////////

    SPngDecoder::SPngDecoder()
    {
        m_signature = "\x89\x50\x4e\x47\xd\xa\x1a\xa";
        m_color_type = 0;
        m_ctx = 0;
        m_f = 0;
        m_buf_supported = true;
        m_buf_pos = 0;
        m_bit_depth = 0;
    }


    SPngDecoder::~SPngDecoder()
    {
        close();
    }

    ImageDecoder SPngDecoder::newDecoder() const
    {
        return makePtr<SPngDecoder>();
    }

    void  SPngDecoder::close()
    {
        if( m_f )
        {
            fclose( m_f );
            m_f = 0;
        }

        if( m_ctx ) {
            struct spng_ctx *ctx = (struct spng_ctx*)m_ctx;
            spng_ctx_free(ctx);
            m_ctx = 0;
        }
    }


    int  SPngDecoder::readDataFromBuf( void* sp_ctx, void *user, void* dst, size_t size )
    {
        /*
         * typedef int spng_read_fn(spng_ctx *ctx, void *user, void *dest, size_t length)
         *   Type definition for callback passed to spng_set_png_stream() for decoders.
         *   A read callback function should copy length bytes to dest and return 0 or SPNG_IO_EOF/SPNG_IO_ERROR on error.
         */
        SPngDecoder* decoder = (SPngDecoder*)(user);
        CV_Assert( decoder );

        const Mat& buf = decoder->m_buf;
        if( decoder->m_buf_pos + size > buf.cols*buf.rows*buf.elemSize() )
        {
            return SPNG_IO_ERROR;
        }
        memcpy( dst, decoder->m_buf.ptr() + decoder->m_buf_pos, size );
        decoder->m_buf_pos += size;

        return 0;
    }

    bool  SPngDecoder::readHeader()
    {
        volatile bool result = false;
        close();

        spng_ctx *ctx = spng_ctx_new(SPNG_CTX_IGNORE_ADLER32);

        if(!ctx) {
            spng_ctx_free(ctx);
            return false;
        }

        m_ctx = ctx;
        spng_set_crc_action(ctx, SPNG_CRC_USE, SPNG_CRC_USE);

        if(!m_buf.empty())
            spng_set_png_stream((struct spng_ctx *) m_ctx, (spng_rw_fn *) readDataFromBuf, this);
        else {
            m_f = fopen(m_filename.c_str(), "rb");
            if(m_f) {
                spng_set_png_file(ctx, m_f);
            }
        }

        if(!m_buf.empty() || m_f)
        {
            struct spng_ihdr ihdr;
            int ret = spng_get_ihdr(ctx, &ihdr);

            if(ret == SPNG_OK) {
                m_width = static_cast<int>(ihdr.width);
                m_height = static_cast<int>(ihdr.height);
                m_color_type = ihdr.color_type;
                m_bit_depth = ihdr.bit_depth;

                if (ihdr.bit_depth <= 8 || ihdr.bit_depth == 16) {
                    int num_trans;
                    switch (ihdr.color_type) {
                        case SPNG_COLOR_TYPE_TRUECOLOR:
                        case SPNG_COLOR_TYPE_INDEXED:
                            struct spng_trns trns;
                            num_trans = !spng_get_trns(ctx, &trns);
                            if (num_trans > 0)
                                m_type = CV_8UC4;
                            else
                                m_type = CV_8UC3;
                            break;
                        case SPNG_COLOR_TYPE_GRAYSCALE_ALPHA:
                        case SPNG_COLOR_TYPE_TRUECOLOR_ALPHA:
                            m_type = CV_8UC4;
                            break;
                        default:
                            m_type = CV_8UC1;
                    }
                    if (ihdr.bit_depth == 16)
                        m_type = CV_MAKETYPE(CV_16U, CV_MAT_CN(m_type));
                    result = true;
                }
            }
        }

        return result;
    }


    bool  SPngDecoder::readData( Mat& img )
    {
        volatile bool result = false;
        bool color = img.channels() > 1;

        struct spng_ctx *png_ptr = (struct spng_ctx *)m_ctx;

        if( m_ctx && m_width && m_height )
        {
            int fmt = SPNG_FMT_PNG;

            struct spng_trns trns;
            int have_trns = spng_get_trns((struct spng_ctx*) m_ctx, &trns);

            int decode_flags = 0;
            if(have_trns == SPNG_OK && img.channels() == 4)
            {
                if( m_color_type == SPNG_COLOR_TYPE_TRUECOLOR ||
                    m_color_type == SPNG_COLOR_TYPE_INDEXED ||
                    m_color_type == SPNG_COLOR_TYPE_TRUECOLOR_ALPHA)
                    fmt = m_bit_depth == 16 ? SPNG_FMT_RGBA16 : SPNG_FMT_RGBA8;
                else if( m_color_type == SPNG_COLOR_TYPE_GRAYSCALE )
                    fmt = m_bit_depth == 16 ? SPNG_FMT_GA16 : SPNG_FMT_GA8;
                else
                    fmt = SPNG_FMT_RGBA8;

                decode_flags = SPNG_DECODE_TRNS;
            }
            else if(img.channels() < 4 && m_bit_depth != 16)
            {
                if( m_color_type == SPNG_COLOR_TYPE_INDEXED ||
                    m_color_type == SPNG_COLOR_TYPE_TRUECOLOR ||
                    m_color_type == SPNG_COLOR_TYPE_TRUECOLOR_ALPHA ||
                    (m_color_type == SPNG_COLOR_TYPE_GRAYSCALE && img.channels() == 3))
                    fmt = SPNG_FMT_RGB8;
                else if( m_color_type == SPNG_COLOR_TYPE_GRAYSCALE &&
                         m_bit_depth <= 8 )
                    fmt = SPNG_FMT_G8;
            }

            size_t image_width, image_size = 0;
            int ret = spng_decoded_image_size(png_ptr, fmt, &image_size);

            if(ret == SPNG_OK) {
                image_width = image_size / m_height;

                ret = spng_decode_image(png_ptr, nullptr, 0, fmt, SPNG_DECODE_PROGRESSIVE | decode_flags);

                if(ret == SPNG_OK)
                {
                    struct spng_row_info row_info;
                    if(!color && (fmt != SPNG_FMT_G8 && fmt != SPNG_FMT_GA8 && fmt != SPNG_FMT_GA16)) {
                        AutoBuffer<unsigned char> buffer;
                        buffer.allocate(image_width);
                        if(fmt == SPNG_FMT_RGB8) { // Convert RBG8 image to Gray
                            do {
                                ret = spng_get_row_info(png_ptr, &row_info);
                                if (ret) break;

                                ret = spng_decode_row(png_ptr, buffer.data(), image_width);
                                icvCvt_BGR2Gray_8u_C3C1R(
                                        buffer.data(),
                                        0,
                                        img.data + row_info.row_num * (image_width / 3),
                                        0, Size(static_cast<int>(image_width) / 3, 1), 2);
                            } while (ret == SPNG_OK);
                        }
                        else if(fmt == SPNG_FMT_RGBA8) { // Convert RBGA8 image to Gray
                            do {
                                ret = spng_get_row_info(png_ptr, &row_info);
                                if (ret) break;

                                ret = spng_decode_row(png_ptr, buffer.data(), image_width);
                                icvCvt_BGRA2Gray_8u_C4C1R(
                                        buffer.data(),
                                        0,
                                        img.data + row_info.row_num * (image_width/4) ,
                                        0, Size(static_cast<int>(image_width)/4,1), 2);
                            } while (ret == SPNG_OK);
                        }
                        else
                        { // Convert RGBA16 to Gray
                            do {
                                ret = spng_get_row_info(png_ptr, &row_info);
                                if (ret) break;

                                ret = spng_decode_row(png_ptr, buffer.data(), image_width);
                                icvCvt_BGRA2Gray_16u_CnC1R(
                                        reinterpret_cast<const ushort *>(buffer.data()), 0,
                                        reinterpret_cast<ushort *>(img.data + row_info.row_num * (image_width / 4)),
                                        0 , Size(static_cast<int>(image_width) / 4, 1),
                                        3, 2);
                            } while (ret == SPNG_OK);
                        }
                    } else if(color){ // RGB -> BGR
                        AutoBuffer<uchar*> _buffer(m_height);
                        uchar** buffer = _buffer.data();
                        for(int y = 0; y < m_height; y++ )
                            buffer[y] = img.data + y*img.step;

                            if(img.channels() == 3 && m_bit_depth == 8){
                                do {
                                    ret = spng_get_row_info(png_ptr, &row_info);
                                    if (ret) break;

                                    ret = spng_decode_row(png_ptr, buffer[row_info.row_num], image_width);
                                    icvCvt_RGB2BGR_8u_C3R(buffer[row_info.row_num], 0, buffer[row_info.row_num], 0, Size(image_width/3,1));
                                } while (ret == SPNG_OK);
                            }
                            else if(img.channels() == 4 && m_bit_depth == 16) {
                                do {
                                    ret = spng_get_row_info(png_ptr, &row_info);
                                    if (ret) break;

                                    ret = spng_decode_row(png_ptr, buffer[row_info.row_num], image_width);
                                    icvCvt_RGBA2BGRA_16u_C4R(reinterpret_cast<const ushort *>(buffer[row_info.row_num]), 0,
                                                             reinterpret_cast<ushort *>(buffer[row_info.row_num]), 0,
                                                             Size(image_width/4, 1));
                                } while (ret == SPNG_OK);
                            }
                            else if(img.channels() == 4 && m_bit_depth == 8){
                                do {
                                    ret = spng_get_row_info(png_ptr, &row_info);
                                    if (ret) break;

                                    ret = spng_decode_row(png_ptr, buffer[row_info.row_num], image_width);
                                    icvCvt_RGBA2BGRA_8u_C4R(buffer[row_info.row_num], 0, buffer[row_info.row_num], 0, Size(image_width/4,1));
                                } while (ret == SPNG_OK);
                            }
                            else {
                                do {
                                    ret = spng_get_row_info(png_ptr, &row_info);
                                    if (ret) break;

                                    ret = spng_decode_row(png_ptr, buffer[row_info.row_num], image_width);
                                    icvCvt_RGB2BGR_16u_C3R(reinterpret_cast<const ushort *>(buffer[row_info.row_num]), 0,
                                                           reinterpret_cast<ushort *>(buffer[row_info.row_num]), 0,
                                                           Size(image_width/3, 1));
                                } while (ret == SPNG_OK);
                            }
                        }
                    else{
                        do
                        {
                            ret = spng_get_row_info(png_ptr, &row_info);
                            if(ret) break;

                            ret = spng_decode_row(png_ptr, img.data + row_info.row_num * img.step, image_width);
                        }
                        while(ret == SPNG_OK);
                    }
                    }

                    if(ret == SPNG_EOI) {
                        struct spng_exif exif_s;
                        ret = spng_get_exif(png_ptr, &exif_s);
                        if(ret == SPNG_OK) {
                            if (exif_s.data && exif_s.length > 0) {
                                m_exif.parseExif((unsigned char *) exif_s.data, exif_s.length);
                            }
                        }
                        result = true;
                    }


            }
        }

        return result;
    }


/////////////////////// PngEncoder ///////////////////


    SPngEncoder::SPngEncoder()
    {
        m_description = "Portable Network Graphics files (*.png)";
        m_buf_supported = true;
    }


    SPngEncoder::~SPngEncoder()
    {
    }


    bool  SPngEncoder::isFormatSupported( int depth ) const
    {
        return depth == CV_8U || depth == CV_16U;
    }

    ImageEncoder SPngEncoder::newEncoder() const
    {
        return makePtr<SPngEncoder>();
    }


    int SPngEncoder::writeDataToBuf(void *ctx, void *user, void *dst_src, size_t length)
    {
        SPngEncoder* encoder = (SPngEncoder*)(user);
        CV_Assert( encoder && encoder->m_buf );
        size_t cursz = encoder->m_buf->size();
        encoder->m_buf->resize(cursz + length);
        memcpy( &(*encoder->m_buf)[cursz], dst_src, length );
        return 0;
    }

    bool  SPngEncoder::write( const Mat& img, const std::vector<int>& params )
    {
        int fmt;
        spng_ctx *ctx = spng_ctx_new(SPNG_CTX_ENCODER);
        FILE * volatile f = 0;
        int width = img.cols, height = img.rows;
        int depth = img.depth(), channels = img.channels();
        volatile bool result = false;

        if( depth != CV_8U && depth != CV_16U )
            return false;

        if( ctx )
        {
            struct spng_ihdr ihdr = {};
            ihdr.height = height;
            ihdr.width = width;

            int compression_level = -1; // Invalid value to allow setting 0-9 as valid
            int compression_strategy = IMWRITE_PNG_STRATEGY_RLE; // Default strategy
            bool isBilevel = false;

            for( size_t i = 0; i < params.size(); i += 2 )
            {
                if( params[i] == IMWRITE_PNG_COMPRESSION )
                {
                    compression_strategy = IMWRITE_PNG_STRATEGY_DEFAULT; // Default strategy
                    compression_level = params[i+1];
                    compression_level = MIN(MAX(compression_level, 0), Z_BEST_COMPRESSION);
                }
                if( params[i] == IMWRITE_PNG_STRATEGY )
                {
                    compression_strategy = params[i+1];
                    compression_strategy = MIN(MAX(compression_strategy, 0), Z_FIXED);
                }
                if( params[i] == IMWRITE_PNG_BILEVEL )
                {
                    isBilevel = params[i+1] != 0;
                }
            }
            fmt = channels == 1 ? SPNG_COLOR_TYPE_GRAYSCALE :
                  channels == 3 ? SPNG_COLOR_TYPE_TRUECOLOR : SPNG_COLOR_TYPE_TRUECOLOR_ALPHA;

            ihdr.bit_depth = depth == CV_8U ? isBilevel ? 1 : 8 : 16;
            ihdr.color_type = fmt;
            ihdr.interlace_method = SPNG_INTERLACE_NONE;
            ihdr.filter_method = SPNG_FILTER_NONE;
            ihdr.compression_method = 0;
            spng_set_ihdr(ctx, &ihdr);

            if( m_buf )
            {
                spng_set_png_stream(ctx,(spng_rw_fn*)writeDataToBuf, this);
            }
            else
            {
                f = fopen( m_filename.c_str(), "wb" );
                if( f )
                    spng_set_png_file( ctx, f );
            }

            if( m_buf || f )
            {
                if( compression_level >= 0 )
                {
                    spng_set_option(ctx, SPNG_IMG_COMPRESSION_LEVEL, compression_level);
                }
                else
                {
                    spng_set_option(ctx, SPNG_FILTER_CHOICE, SPNG_FILTER_CHOICE_SUB);
                    spng_set_option(ctx, SPNG_IMG_COMPRESSION_LEVEL, Z_BEST_SPEED);
                }
                spng_set_option(ctx, SPNG_IMG_COMPRESSION_STRATEGY, compression_strategy);

                int ret;
                if(channels > 1) {
                    spng_encode_chunks(ctx);
                    ret = spng_encode_image(ctx, nullptr, 0, SPNG_FMT_PNG, SPNG_ENCODE_PROGRESSIVE);
                    int error;
                    if (ret == SPNG_OK) {
                        if(depth == CV_16U) {
                            AutoBuffer<ushort*> buff2;
                            buff2.allocate(height);
                            for(int y = 0; y < height; y++ )
                                buff2[y] = reinterpret_cast<unsigned short *>(img.data + y * img.step);

                            AutoBuffer<ushort> _buffer;
                            _buffer.allocate(width*channels*2);
                            for (int y = 0; y < height; y++) {
                                if (channels == 3) {
                                    icvCvt_BGR2RGB_16u_C3R(buff2[y], 0,
                                                           _buffer.data(), 0, Size(width, 1));
                                } else if (channels == 4) {
                                    icvCvt_BGRA2RGBA_16u_C4R(buff2[y], 0,
                                                             _buffer.data(), 0, Size(width, 1));
                                }
                                error = spng_encode_row(ctx, _buffer.data(), width * channels * 2);
                                if (error) break;
                            }
                        } else {
                            AutoBuffer<uchar*> buff;
                            buff.allocate(height);
                            for(int y = 0; y < height; y++ )
                                buff[y] = img.data + y*img.step;

                            AutoBuffer<uchar> _buffer;
                            _buffer.allocate(width*channels);
                            for (int y = 0; y < height; y++) {
                                if (channels == 3) {
                                    icvCvt_BGR2RGB_8u_C3R(buff[y], 0, _buffer.data(), 0, Size(width, 1));
                                } else if (channels == 4) {
                                    icvCvt_BGRA2RGBA_8u_C4R(buff[y], 0, _buffer.data(), 0, Size(width, 1));
                                }
                                error = spng_encode_row(ctx, _buffer.data(), width * channels);
                                if (error) break;
                            }
                        }

                        if (error == SPNG_EOI) { // success
                            spng_encode_chunks(ctx);
                            ret = SPNG_OK;
                        }
                    }
                }
                else{
                    AutoBuffer<uchar*> buff;
                    buff.allocate(height);
                    for(int y = 0; y < height; y++ )
                        buff[y] = img.data + y*img.step;

                    ret = spng_encode_image(ctx, *buff.data(), img.cols*img.rows*channels, SPNG_FMT_PNG, SPNG_ENCODE_FINALIZE);
                }
                if(ret == SPNG_OK)
                    result = true;
            }
        }

        spng_ctx_free(ctx);
        if(f) fclose( (FILE*)f );

        return result;
    }

}

#endif

/* End of file. */