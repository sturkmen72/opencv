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

/*****************************************************\
    this file includes some part of APNG Optimizer 1.4

 *  APNG Optimizer 1.4
 *
 *  Makes APNG files smaller.
 *
 *  http://sourceforge.net/projects/apng/files
 *
 *  Copyright (c) 2011-2015 Max Stepin
 *  maxst at users.sourceforge.net
 *
 *  zlib license
 *****************************************************/

#ifndef _LFS64_LARGEFILE
#  define _LFS64_LARGEFILE 0
#endif
#ifndef _FILE_OFFSET_BITS
#  define _FILE_OFFSET_BITS 0
#endif

#include <png.h>
#include <zlib.h>

#include "grfmt_apng.hpp"

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
    void info_fn(png_structp png_ptr, png_infop info_ptr);
    void row_fn(png_structp png_ptr, png_bytep new_row, png_uint_32 row_num, int pass);
    int cmp_colors(const void* arg1, const void* arg2);

    void info_fn(png_structp png_ptr, png_infop info_ptr)
    {
        png_set_expand(png_ptr);
        png_set_strip_16(png_ptr);
        png_set_gray_to_rgb(png_ptr);
        png_set_add_alpha(png_ptr, 0xff, PNG_FILLER_AFTER);
        (void)png_set_interlace_handling(png_ptr);
        png_read_update_info(png_ptr, info_ptr);
    }

    void row_fn(png_structp png_ptr, png_bytep new_row, png_uint_32 row_num, int pass)
    {
        CV_UNUSED(pass);
        APNGFrame* frame = (APNGFrame*)png_get_progressive_ptr(png_ptr);
        png_progressive_combine_row(png_ptr, frame->rows()[row_num], new_row);
    }

    int cmp_colors(const void* arg1, const void* arg2)
    {
        if (((COLORS*)arg1)->a != ((COLORS*)arg2)->a)
            return (int)(((COLORS*)arg1)->a) - (int)(((COLORS*)arg2)->a);

        if (((COLORS*)arg1)->num != ((COLORS*)arg2)->num)
            return (int)(((COLORS*)arg2)->num) - (int)(((COLORS*)arg1)->num);

        if (((COLORS*)arg1)->r != ((COLORS*)arg2)->r)
            return (int)(((COLORS*)arg1)->r) - (int)(((COLORS*)arg2)->r);

        if (((COLORS*)arg1)->g != ((COLORS*)arg2)->g)
            return (int)(((COLORS*)arg1)->g) - (int)(((COLORS*)arg2)->g);

        return (int)(((COLORS*)arg1)->b) - (int)(((COLORS*)arg2)->b);
    }




    unsigned char* APNGFrame::pixels(unsigned char* setPixels) {
        if (setPixels != NULL)
            _pixels = setPixels;
        return _pixels;
    }

    unsigned int APNGFrame::width(unsigned int setWidth) {
        if (setWidth != 0)
            _width = setWidth;
        return _width;
    }

    unsigned int APNGFrame::height(unsigned int setHeight) {
        if (setHeight != 0)
            _height = setHeight;
        return _height;
    }

    unsigned char APNGFrame::colorType(unsigned char setColorType) {
        if (setColorType != 255)

            _colorType = setColorType;
        return _colorType;
    }

    rgb* APNGFrame::palette(rgb* setPalette) {
        if (setPalette != NULL)
            memcpy(_palette, setPalette,
                std::min(sizeof(_palette), sizeof(setPalette)));
        return _palette;
    }

    unsigned char* APNGFrame::transparency(unsigned char* setTransparency) {
        if (setTransparency != NULL)
            memcpy(_transparency, setTransparency,
                std::min(sizeof(_transparency), sizeof(setTransparency)));
        return _transparency;
    }

    int APNGFrame::paletteSize(int setPaletteSize) {
        if (setPaletteSize != 0)
            _paletteSize = setPaletteSize;
        return _paletteSize;
    }

    int APNGFrame::transparencySize(int setTransparencySize) {
        if (setTransparencySize != 0)
            _transparencySize = setTransparencySize;
        return _transparencySize;
    }

    unsigned int APNGFrame::delayNum(unsigned int setDelayNum) {
        if (setDelayNum != 0)
            _delayNum = setDelayNum;
        return _delayNum;
    }

    unsigned int APNGFrame::delayDen(unsigned int setDelayDen) {
        if (setDelayDen != 0)
            _delayDen = setDelayDen;
        return _delayDen;
    }

    unsigned char** APNGFrame::rows(unsigned char** setRows) {
        if (setRows != NULL)
            _rows = setRows;
        return _rows;
    }

    APNGFrame::APNGFrame()
        : _pixels(NULL), _width(0), _height(0), _colorType(0), _paletteSize(0),
        _transparencySize(0), _delayNum(0), _delayDen(0), _rows(NULL) {
        memset(_palette, 0, sizeof(_palette));
        memset(_transparency, 0, sizeof(_transparency));
    }

    APNGFrame::APNGFrame(const std::string& filePath, unsigned delayNum,
        unsigned delayDen)
        : _pixels(NULL), _width(0), _height(0), _colorType(0), _paletteSize(0),
        _transparencySize(0), _delayNum(delayNum), _delayDen(delayDen),
        _rows(NULL) {
        memset(_palette, 0, sizeof(_palette));
        memset(_transparency, 0, sizeof(_transparency));
        // TODO save extracted info to self
        FILE* f;
        if ((f = fopen(filePath.c_str(), "rb")) != 0) {
            unsigned char sig[8];

            if (fread(sig, 1, 8, f) == 8 && png_sig_cmp(sig, 0, 8) == 0) {
                png_structp png_ptr =
                    png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
                png_infop info_ptr = png_create_info_struct(png_ptr);
                if (png_ptr && info_ptr) {
                    png_byte depth;
                    png_uint_32 rowbytes, i;
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
                    _width = png_get_image_width(png_ptr, info_ptr);
                    _height = png_get_image_height(png_ptr, info_ptr);
                    _colorType = png_get_color_type(png_ptr, info_ptr);
                    depth = png_get_bit_depth(png_ptr, info_ptr);
                    if (depth < 8) {
                        if (_colorType == PNG_COLOR_TYPE_PALETTE)
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
                    _colorType = png_get_color_type(png_ptr, info_ptr);
                    rowbytes = (int)png_get_rowbytes(png_ptr, info_ptr);
                    memset(_palette, 255, sizeof(_palette));
                    memset(_transparency, 255, sizeof(_transparency));

                    if (png_get_PLTE(png_ptr, info_ptr, &palette, &_paletteSize))
                        memcpy(_palette, palette, _paletteSize * 3);
                    else
                        _paletteSize = 0;

                    if (png_get_tRNS(png_ptr, info_ptr, &trans_alpha, &_transparencySize,
                        &trans_color)) {
                        if (_transparencySize > 0) {
                            if (_colorType == PNG_COLOR_TYPE_GRAY) {
                                _transparency[0] = 0;
                                _transparency[1] = trans_color->gray & 0xFF;
                                _transparencySize = 2;
                            }
                            else if (_colorType == PNG_COLOR_TYPE_RGB) {
                                _transparency[0] = 0;
                                _transparency[1] = trans_color->red & 0xFF;
                                _transparency[2] = 0;
                                _transparency[3] = trans_color->green & 0xFF;
                                _transparency[4] = 0;
                                _transparency[5] = trans_color->blue & 0xFF;
                                _transparencySize = 6;
                            }
                            else if (_colorType == PNG_COLOR_TYPE_PALETTE)
                                memcpy(_transparency, trans_alpha, _transparencySize);
                            else
                                _transparencySize = 0;
                        }
                    }
                    else
                        _transparencySize = 0;

                    _pixels = new unsigned char[_height * rowbytes];
                    _rows = new png_bytep[_height * sizeof(png_bytep)];

                    for (i = 0; i < _height; ++i)
                        _rows[i] = _pixels + i * rowbytes;

                    png_read_image(png_ptr, _rows);
                    png_read_end(png_ptr, NULL);
                }
                png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
            }
            fclose(f);
        }
    }

    APNGFrame::APNGFrame(rgb* pixels, unsigned int width, unsigned int height,
        rgb* trns_color, unsigned delayNum, unsigned delayDen)
        : _pixels(NULL), _width(0), _height(0), _colorType(0), _paletteSize(0),
        _transparencySize(0), _delayNum(delayNum), _delayDen(delayDen),
        _rows(NULL) {
        memset(_palette, 0, sizeof(_palette));
        memset(_transparency, 0, sizeof(_transparency));

        if (pixels != NULL) {
            png_uint_32 rowbytes = width * 3;

            _width = width;
            _height = height;
            _colorType = 2;

            _pixels = new unsigned char[_height * rowbytes];
            _rows = new png_bytep[_height * sizeof(png_bytep)];

            memcpy(_pixels, pixels, _height * rowbytes);

            for (unsigned int i = 0; i < _height; ++i)
                _rows[i] = _pixels + i * rowbytes;

            if (trns_color != NULL) {
                _transparency[0] = 0;
                _transparency[1] = trns_color->r;
                _transparency[2] = 0;
                _transparency[3] = trns_color->g;
                _transparency[4] = 0;
                _transparency[5] = trns_color->b;
                _transparencySize = 6;
            }
        }
    }

    APNGFrame::APNGFrame(rgba* pixels, unsigned int width, unsigned int height,
        unsigned delayNum, unsigned delayDen)
        : _pixels(NULL), _width(0), _height(0), _colorType(0), _paletteSize(0),
        _transparencySize(0), _delayNum(delayNum), _delayDen(delayDen),
        _rows(NULL) {
        memset(_palette, 0, sizeof(_palette));
        memset(_transparency, 0, sizeof(_transparency));

        if (pixels != NULL) {
            png_uint_32 rowbytes = width * 4;

            _width = width;
            _height = height;
            _colorType = 6;

            _pixels = new unsigned char[_height * rowbytes];
            _rows = new png_bytep[_height * sizeof(png_bytep)];

            memcpy(_pixels, pixels, _height * rowbytes);

            for (unsigned int i = 0; i < _height; ++i)
                _rows[i] = _pixels + i * rowbytes;
        }
    }

    // Save frame to a PNG image file.
    // Return true if save succeeded.
    bool APNGFrame::save(const std::string& outPath) const {
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
                png_set_IHDR(png_ptr, info_ptr, _width, _height, 8, _colorType, 0, 0, 0);
                if (_paletteSize > 0) {
                    png_color palette[PNG_MAX_PALETTE_LENGTH];
                    memcpy(palette, _palette, _paletteSize * 3);
                    png_set_PLTE(png_ptr, info_ptr, palette, _paletteSize);
                }
                if (_transparencySize > 0) {
                    png_color_16 trans_color;
                    if (_colorType == PNG_COLOR_TYPE_GRAY) {
                        trans_color.gray = _transparency[1];
                        png_set_tRNS(png_ptr, info_ptr, NULL, 0, &trans_color);
                    }
                    else if (_colorType == PNG_COLOR_TYPE_RGB) {
                        trans_color.red = _transparency[1];
                        trans_color.green = _transparency[3];
                        trans_color.blue = _transparency[5];
                        png_set_tRNS(png_ptr, info_ptr, NULL, 0, &trans_color);
                    }
                    else if (_colorType == PNG_COLOR_TYPE_PALETTE)
                        png_set_tRNS(png_ptr, info_ptr,
                            const_cast<unsigned char*>(_transparency),
                            _transparencySize, NULL);
                }
                png_write_info(png_ptr, info_ptr);
                png_write_image(png_ptr, _rows);
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

/////////////////////// PngDecoder ///////////////////

ApngDecoder::ApngDecoder()
{
    m_signature = "\x89\x50\x4e\x47\xd\xa\x1a\xa";
    m_color_type = 0;
    m_png_ptr = 0;
    m_info_ptr = m_end_info = 0;
    m_f = 0;
    m_buf_supported = true;
    m_buf_pos = 0;
    m_bit_depth = 0;

}


ApngDecoder::~ApngDecoder()
{
    close();
}

ImageDecoder ApngDecoder::newDecoder() const
{
    return makePtr<ApngDecoder>();
}

void  ApngDecoder::close()
{
    if( m_f )
    {
        fclose( m_f );
        m_f = 0;
    }

    if( m_png_ptr )
    {
        png_structp png_ptr = (png_structp)m_png_ptr;
        png_infop info_ptr = (png_infop)m_info_ptr;
        png_infop end_info = (png_infop)m_end_info;
        png_destroy_read_struct( &png_ptr, &info_ptr, &end_info );
        m_png_ptr = m_info_ptr = m_end_info = 0;
    }
}


void  ApngDecoder::readDataFromBuf( void* _png_ptr, uchar* dst, size_t size )
{
    png_structp png_ptr = (png_structp)_png_ptr;
    ApngDecoder* decoder = (ApngDecoder*)(png_get_io_ptr(png_ptr));
    CV_Assert( decoder );
    const Mat& buf = decoder->m_buf;
    if( decoder->m_buf_pos + size > buf.cols*buf.rows*buf.elemSize() )
    {
        png_error(png_ptr, "PNG input buffer is incomplete");
        return;
    }
    memcpy( dst, decoder->m_buf.ptr() + decoder->m_buf_pos, size );
    decoder->m_buf_pos += size;
}

bool ApngDecoder::setSource(const String& filename)
{
    std::vector<APNGFrame> frames;
    unsigned int first, loops;

    load_apng(filename, frames, first, loops);

    for (int i = 0; i < frames.size(); i++)
    {
        String fname = format("frame%d.png", i);
        //frames[i].save(fname);
    }
    return true;
}

bool  ApngDecoder::readHeader()
{
    volatile bool result = false;
    close();

    png_structp png_ptr = png_create_read_struct( PNG_LIBPNG_VER_STRING, 0, 0, 0 );

    if( png_ptr )
    {
        png_infop info_ptr = png_create_info_struct( png_ptr );
        png_infop end_info = png_create_info_struct( png_ptr );

        m_png_ptr = png_ptr;
        m_info_ptr = info_ptr;
        m_end_info = end_info;
        m_buf_pos = 0;

        if( info_ptr && end_info )
        {
            if( setjmp( png_jmpbuf( png_ptr ) ) == 0 )
            {
                if( !m_buf.empty() )
                    png_set_read_fn(png_ptr, this, (png_rw_ptr)readDataFromBuf );
                else
                {
                    m_f = fopen( m_filename.c_str(), "rb" );
                    if( m_f )
                        png_init_io( png_ptr, m_f );
                }

                if( !m_buf.empty() || m_f )
                {
                    png_uint_32 wdth, hght;
                    int bit_depth, color_type, num_trans=0;
                    png_bytep trans;
                    png_color_16p trans_values;

                    png_read_info( png_ptr, info_ptr );

                    png_get_IHDR( png_ptr, info_ptr, &wdth, &hght,
                                  &bit_depth, &color_type, 0, 0, 0 );

                    m_width = (int)wdth;
                    m_height = (int)hght;
                    m_color_type = color_type;
                    m_bit_depth = bit_depth;

                    if( bit_depth <= 8 || bit_depth == 16 )
                    {
                        switch(color_type)
                        {
                            case PNG_COLOR_TYPE_RGB:
                            case PNG_COLOR_TYPE_PALETTE:
                                png_get_tRNS(png_ptr, info_ptr, &trans, &num_trans, &trans_values);
                                if( num_trans > 0 )
                                    m_type = CV_8UC4;
                                else
                                    m_type = CV_8UC3;
                                break;
                            case PNG_COLOR_TYPE_GRAY_ALPHA:
                            case PNG_COLOR_TYPE_RGB_ALPHA:
                                m_type = CV_8UC4;
                                break;
                            default:
                                m_type = CV_8UC1;
                        }
                        if( bit_depth == 16 )
                            m_type = CV_MAKETYPE(CV_16U, CV_MAT_CN(m_type));
                        result = true;
                    }
                }
            }
        }
    }

    return result;
}


bool  ApngDecoder::readData( Mat& img )
{
    volatile bool result = false;
    AutoBuffer<uchar*> _buffer(m_height);
    uchar** buffer = _buffer.data();
    bool color = img.channels() > 1;

    png_structp png_ptr = (png_structp)m_png_ptr;
    png_infop info_ptr = (png_infop)m_info_ptr;
    png_infop end_info = (png_infop)m_end_info;

    if( m_png_ptr && m_info_ptr && m_end_info && m_width && m_height )
    {
        if( setjmp( png_jmpbuf ( png_ptr ) ) == 0 )
        {
            int y;

            if( img.depth() == CV_8U && m_bit_depth == 16 )
                png_set_strip_16( png_ptr );
            else if( !isBigEndian() )
                png_set_swap( png_ptr );

            if(img.channels() < 4)
            {
                /* observation: png_read_image() writes 400 bytes beyond
                 * end of data when reading a 400x118 color png
                 * "mpplus_sand.png".  OpenCV crashes even with demo
                 * programs.  Looking at the loaded image I'd say we get 4
                 * bytes per pixel instead of 3 bytes per pixel.  Test
                 * indicate that it is a good idea to always ask for
                 * stripping alpha..  18.11.2004 Axel Walthelm
                 */
                 png_set_strip_alpha( png_ptr );
            } else
                png_set_tRNS_to_alpha( png_ptr );

            if( m_color_type == PNG_COLOR_TYPE_PALETTE )
                png_set_palette_to_rgb( png_ptr );

            if( (m_color_type & PNG_COLOR_MASK_COLOR) == 0 && m_bit_depth < 8 )
#if (PNG_LIBPNG_VER_MAJOR*10000 + PNG_LIBPNG_VER_MINOR*100 + PNG_LIBPNG_VER_RELEASE >= 10209) || \
    (PNG_LIBPNG_VER_MAJOR == 1 && PNG_LIBPNG_VER_MINOR == 0 && PNG_LIBPNG_VER_RELEASE >= 18)
                png_set_expand_gray_1_2_4_to_8( png_ptr );
#else
                png_set_gray_1_2_4_to_8( png_ptr );
#endif

            if( (m_color_type & PNG_COLOR_MASK_COLOR) && color )
                png_set_bgr( png_ptr ); // convert RGB to BGR
            else if( color )
                png_set_gray_to_rgb( png_ptr ); // Gray->RGB
            else
                png_set_rgb_to_gray( png_ptr, 1, 0.299, 0.587 ); // RGB->Gray

            png_set_interlace_handling( png_ptr );
            png_read_update_info( png_ptr, info_ptr );

            for( y = 0; y < m_height; y++ )
                buffer[y] = img.data + y*img.step;

            png_read_image( png_ptr, buffer );
            png_read_end( png_ptr, end_info );

#ifdef PNG_eXIf_SUPPORTED
            png_uint_32 num_exif = 0;
            png_bytep exif = 0;

            // Exif info could be in info_ptr (intro_info) or end_info per specification
            if( png_get_valid(png_ptr, info_ptr, PNG_INFO_eXIf) )
                png_get_eXIf_1(png_ptr, info_ptr, &num_exif, &exif);
            else if( png_get_valid(png_ptr, end_info, PNG_INFO_eXIf) )
                png_get_eXIf_1(png_ptr, end_info, &num_exif, &exif);

            if( exif && num_exif > 0 )
            {
                m_exif.parseExif(exif, num_exif);
            }
#endif

            result = true;
        }
    }

    return result;
}

void ApngDecoder::compose_frame(unsigned char** rows_dst, unsigned char** rows_src, unsigned char bop, unsigned int x, unsigned int y, unsigned int w, unsigned int h)
{
    unsigned int  i, j;
    int u, v, al;

    for (j = 0; j < h; j++)
    {
        unsigned char* sp = rows_src[j];
        unsigned char* dp = rows_dst[j + y] + x * 4;

        if (bop == 0)
            memcpy(dp, sp, w * 4);
        else
            for (i = 0; i < w; i++, sp += 4, dp += 4)
            {
                if (sp[3] == 255)
                    memcpy(dp, sp, 4);
                else
                    if (sp[3] != 0)
                    {
                        if (dp[3] != 0)
                        {
                            u = sp[3] * 255;
                            v = (255 - sp[3]) * dp[3];
                            al = u + v;
                            dp[0] = (sp[0] * u + dp[0] * v) / al;
                            dp[1] = (sp[1] * u + dp[1] * v) / al;
                            dp[2] = (sp[2] * u + dp[2] * v) / al;
                            dp[3] = al / 255;
                        }
                        else
                            memcpy(dp, sp, 4);
                    }
            }
    }
}

unsigned int ApngDecoder::read_chunk(FILE* f, CHUNK* pChunk)
{
    unsigned char len[4];
    pChunk->size = 0;
    pChunk->p = 0;
    if (fread(&len, 4, 1, f) == 1)
    {
        pChunk->size = png_get_uint_32(len);
        if (pChunk->size > PNG_USER_CHUNK_MALLOC_MAX)
            return 0;
        pChunk->size += 12;
        pChunk->p = new unsigned char[pChunk->size];
        memcpy(pChunk->p, len, 4);
        if (fread(pChunk->p + 4, pChunk->size - 4, 1, f) == 1)
            return *(unsigned int*)(pChunk->p + 4);
    }
    return 0;
}

int ApngDecoder::processing_start(png_structp& png_ptr, png_infop& info_ptr, void* frame_ptr, bool hasInfo, CHUNK& chunkIHDR, std::vector<CHUNK>& chunksInfo)
{
    unsigned char header[8] = { 137, 80, 78, 71, 13, 10, 26, 10 };

    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    info_ptr = png_create_info_struct(png_ptr);
    if (!png_ptr || !info_ptr)
        return 1;

    if (setjmp(png_jmpbuf(png_ptr)))
    {
        png_destroy_read_struct(&png_ptr, &info_ptr, 0);
        return 1;
    }

    png_set_crc_action(png_ptr, PNG_CRC_QUIET_USE, PNG_CRC_QUIET_USE);
    png_set_progressive_read_fn(png_ptr, frame_ptr, info_fn, row_fn, NULL);

    png_process_data(png_ptr, info_ptr, header, 8);
    png_process_data(png_ptr, info_ptr, chunkIHDR.p, chunkIHDR.size);

    if (hasInfo)
        for (unsigned int i = 0; i < chunksInfo.size(); i++)
            png_process_data(png_ptr, info_ptr, chunksInfo[i].p, chunksInfo[i].size);

    return 0;
}

int ApngDecoder::processing_data(png_structp png_ptr, png_infop info_ptr, unsigned char* p, unsigned int size)
{
    if (!png_ptr || !info_ptr)
        return 1;

    if (setjmp(png_jmpbuf(png_ptr)))
    {
        png_destroy_read_struct(&png_ptr, &info_ptr, 0);
        return 1;
    }

    png_process_data(png_ptr, info_ptr, p, size);
    return 0;
}

int ApngDecoder::processing_finish(png_structp png_ptr, png_infop info_ptr)
{
    unsigned char footer[12] = { 0, 0, 0, 0, 73, 69, 78, 68, 174, 66, 96, 130 };

    if (!png_ptr || !info_ptr)
        return 1;

    if (setjmp(png_jmpbuf(png_ptr)))
    {
        png_destroy_read_struct(&png_ptr, &info_ptr, 0);
        return 1;
    }

    png_process_data(png_ptr, info_ptr, footer, 12);
    png_destroy_read_struct(&png_ptr, &info_ptr, 0);

    return 0;
}

int ApngDecoder::load_apng(std::string inputFileName, std::vector<APNGFrame>& frames, unsigned int& first, unsigned int& loops)
{
    FILE* f;
    unsigned int id, i, j, w, h, w0, h0, x0, y0;
    unsigned int delay_num, delay_den, dop, bop, rowbytes, imagesize;
    unsigned char sig[8];
    png_structp png_ptr;
    png_infop info_ptr;
    CHUNK chunk;
    CHUNK chunkIHDR;
    std::vector<CHUNK> chunksInfo;
    bool isAnimated = false;
    bool hasInfo = false;
    APNGFrame frameRaw(inputFileName); //initialized temporarily by reading from file.
    APNGFrame frameCur(inputFileName); //initialized temporarily by reading from file.
    APNGFrame frameNext(inputFileName); //initialized temporarily by reading from file.
    int result = -1;
    first = 0;
    const unsigned long cMaxPNGSize = 1000000UL;

    if ((f = fopen(inputFileName.c_str(), "rb")) != 0)
    {
        if (fread(sig, 1, 8, f) == 8 && png_sig_cmp(sig, 0, 8) == 0)
        {
            id = read_chunk(f, &chunkIHDR);

            if (id == id_IHDR && chunkIHDR.size == 25)
            {
                w0 = w = png_get_uint_32(chunkIHDR.p + 8);
                h0 = h = png_get_uint_32(chunkIHDR.p + 12);

                if (w > cMaxPNGSize || h > cMaxPNGSize)
                {
                    fclose(f);
                    return result;
                }

                x0 = 0;
                y0 = 0;
                delay_num = 1;
                delay_den = 10;
                dop = 0;
                bop = 0;
                rowbytes = w * 4;
                imagesize = h * rowbytes;

                frameRaw.pixels(new unsigned char[imagesize]);
                frameRaw.rows(new png_bytep[h * sizeof(png_bytep)]);
                for (j = 0; j < h; j++)
                    frameRaw.rows()[j] = frameRaw.pixels() + j * rowbytes;

                if (!processing_start(png_ptr, info_ptr, (void*)&frameRaw, hasInfo, chunkIHDR, chunksInfo))
                {
                    frameCur.width(w);
                    frameCur.height(h);
                    frameCur.pixels(new unsigned char[imagesize]);
                    frameCur.rows(new png_bytep[h * sizeof(png_bytep)]);
                    for (j = 0; j < h; j++)
                        frameCur.rows()[j] = frameCur.pixels() + j * rowbytes;

                    while (!feof(f))
                    {
                        id = read_chunk(f, &chunk);
                        if (!id)
                            break;

                        if (id == id_acTL && !hasInfo && !isAnimated)
                        {
                            isAnimated = true;
                            first = 1;
                            loops = png_get_uint_32(chunk.p + 12);
                        }
                        else if (id == id_fcTL && (!hasInfo || isAnimated))
                        {
                            if (hasInfo)
                            {
                                if (!processing_finish(png_ptr, info_ptr))
                                {
                                    frameNext.pixels(new unsigned char[imagesize]);
                                    frameNext.rows(new png_bytep[h * sizeof(png_bytep)]);
                                    for (j = 0; j < h; j++)
                                        frameNext.rows()[j] = frameNext.pixels() + j * rowbytes;

                                    if (dop == 2)
                                        memcpy(frameNext.pixels(), frameCur.pixels(), imagesize);

                                    compose_frame(frameCur.rows(), frameRaw.rows(), bop, x0, y0, w0, h0);
                                    frameCur.delayNum(delay_num);
                                    frameCur.delayDen(delay_den);
                                    frames.push_back(frameCur);

                                    String fname = format("frameRaw%.2d.png", frames.size() - 1);
                                    //frameRaw.save(fname);
                                    fname = format("frameCur%.2d.png", frames.size() - 1);
                                    //frameCur.save(fname);
                                    fname = format("frameNext%.2d.png", frames.size()-1);
                                    //frameNext.save(fname);

                                    if (dop != 2)
                                    {
                                        memcpy(frameNext.pixels(), frameCur.pixels(), imagesize);
                                        if (dop == 1)
                                            for (j = 0; j < h0; j++)
                                                memset(frameNext.rows()[y0 + j] + x0 * 4, 0, w0 * 4);
                                    }
                                    frameCur.pixels(frameNext.pixels());
                                    frameCur.rows(frameNext.rows());
                                }
                                else
                                {
                                    delete[] frameCur.rows();
                                    delete[] frameCur.pixels();
                                    delete[] chunk.p;
                                    break;
                                }
                            }

                            // At this point the old frame is done. Let's start a new one.
                            w0 = png_get_uint_32(chunk.p + 12);
                            h0 = png_get_uint_32(chunk.p + 16);
                            x0 = png_get_uint_32(chunk.p + 20);
                            y0 = png_get_uint_32(chunk.p + 24);
                            delay_num = png_get_uint_16(chunk.p + 28);
                            delay_den = png_get_uint_16(chunk.p + 30);
                            dop = chunk.p[32];
                            bop = chunk.p[33];

                            if (w0 > cMaxPNGSize || h0 > cMaxPNGSize || x0 > cMaxPNGSize || y0 > cMaxPNGSize || x0 + w0 > w || y0 + h0 > h || dop > 2 || bop > 1)
                            {
                                delete[] frameCur.rows();
                                delete[] frameCur.pixels();
                                delete[] chunk.p;
                                break;
                            }

                            if (hasInfo)
                            {
                                memcpy(chunkIHDR.p + 8, chunk.p + 12, 8);
                                if (processing_start(png_ptr, info_ptr, (void*)&frameRaw, hasInfo, chunkIHDR, chunksInfo))
                                {
                                    delete[] frameCur.rows();
                                    delete[] frameCur.pixels();
                                    delete[] chunk.p;
                                    break;
                                }
                            }
                            else
                                first = 0;

                            if (frames.size() == first)
                            {
                                bop = 0;
                                if (dop == 2)
                                    dop = 1;
                            }
                        }
                        else if (id == id_IDAT)
                        {
                            hasInfo = true;
                            if (processing_data(png_ptr, info_ptr, chunk.p, chunk.size))
                            {
                                delete[] frameCur.rows();
                                delete[] frameCur.pixels();
                                delete[] chunk.p;
                                break;
                            }
                        }
                        else if (id == id_fdAT && isAnimated)
                        {
                            png_save_uint_32(chunk.p + 4, chunk.size - 16);
                            memcpy(chunk.p + 8, "IDAT", 4);
                            if (processing_data(png_ptr, info_ptr, chunk.p + 4, chunk.size - 4))
                            {
                                delete[] frameCur.rows();
                                delete[] frameCur.pixels();
                                delete[] chunk.p;
                                break;
                            }
                        }
                        else if (id == id_IEND)
                        {
                            if (hasInfo && !processing_finish(png_ptr, info_ptr))
                            {
                                compose_frame(frameCur.rows(), frameRaw.rows(), bop, x0, y0, w0, h0);
                                frameCur.delayNum(delay_num);
                                frameCur.delayDen(delay_den);
                                frames.push_back(frameCur);
                            }
                            else
                            {
                                delete[] frameCur.rows();
                                delete[] frameCur.pixels();
                            }
                            delete[] chunk.p;
                            break;
                        }
                        else if (notabc(chunk.p[4]) || notabc(chunk.p[5]) || notabc(chunk.p[6]) || notabc(chunk.p[7]))
                        {
                            delete[] chunk.p;
                            break;
                        }
                        else if (!hasInfo)
                        {
                            if (processing_data(png_ptr, info_ptr, chunk.p, chunk.size))
                            {
                                delete[] frameCur.rows();
                                delete[] frameCur.pixels();
                                delete[] chunk.p;
                                break;
                            }
                            chunksInfo.push_back(chunk);
                            continue;
                        }
                        delete[] chunk.p;
                    }
                }
                delete[] frameRaw.rows();
                delete[] frameRaw.pixels();

                if (!frames.empty())
                {
                    printf("***********************************\nfile name : %s .. frames in the file : %zd\n", inputFileName.c_str(), frames.size());
                    result = 0;
                }
            }

            for (i = 0; i < chunksInfo.size(); i++)
                delete[] chunksInfo[i].p;

            chunksInfo.clear();
            delete[] chunkIHDR.p;
        }
        fclose(f);
    }

    return result;
}

/////////////////////// PngEncoder ///////////////////


ApngEncoder::ApngEncoder()
{
    m_description = "Portable Network Graphics files (*.png)";
    m_buf_supported = true;
    op_zbuf1 = NULL;
    op_zbuf2 = NULL;
    op_zstream1.zalloc = NULL;
    op_zstream2.zalloc = NULL;
    row_buf = NULL;
    sub_row = NULL;
    up_row = NULL;
    avg_row = NULL;
    paeth_row = NULL;
    next_seq_num = 0;
    trnssize = 0;
    palsize = 0;
    memset(palette, 0, sizeof(palette));
    memset(trns, 0, sizeof(trns));
    memset(op, 0, sizeof(op));
    process_callback = { 0 };
}


ApngEncoder::~ApngEncoder()
{
}


bool  ApngEncoder::isFormatSupported( int depth ) const
{
    return depth == CV_8U || depth == CV_16U;
}

ImageEncoder ApngEncoder::newEncoder() const
{
    return makePtr<ApngEncoder>();
}


void ApngEncoder::writeDataToBuf(void* _png_ptr, uchar* src, size_t size)
{
    if( size == 0 )
        return;
    png_structp png_ptr = (png_structp)_png_ptr;
    ApngEncoder* encoder = (ApngEncoder*)(png_get_io_ptr(png_ptr));
    CV_Assert( encoder && encoder->m_buf );
    size_t cursz = encoder->m_buf->size();
    encoder->m_buf->resize(cursz + size);
    memcpy( &(*encoder->m_buf)[cursz], src, size );
}


void ApngEncoder::flushBuf(void*)
{
}

bool  ApngEncoder::write( const Mat& img, const std::vector<int>& params )
{
    png_structp png_ptr = png_create_write_struct( PNG_LIBPNG_VER_STRING, 0, 0, 0 );
    png_infop info_ptr = 0;
    FILE * volatile f = 0;
    int y, width = img.cols, height = img.rows;
    int depth = img.depth(), channels = img.channels();
    volatile bool result = false;
    AutoBuffer<uchar*> buffer;

    if( depth != CV_8U && depth != CV_16U )
        return false;

    if( png_ptr )
    {
        info_ptr = png_create_info_struct( png_ptr );

        if( info_ptr )
        {
            if( setjmp( png_jmpbuf ( png_ptr ) ) == 0 )
            {
                if( m_buf )
                {
                    png_set_write_fn(png_ptr, this,
                        (png_rw_ptr)writeDataToBuf, (png_flush_ptr)flushBuf);
                }
                else
                {
                    f = fopen( m_filename.c_str(), "wb" );
                    if( f )
                        png_init_io( png_ptr, (png_FILE_p)f );
                }

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

                if( m_buf || f )
                {
                    if( compression_level >= 0 )
                    {
                        png_set_compression_level( png_ptr, compression_level );
                    }
                    else
                    {
                        // tune parameters for speed
                        // (see http://wiki.linuxquestions.org/wiki/Libpng)
                        png_set_filter(png_ptr, PNG_FILTER_TYPE_BASE, PNG_FILTER_SUB);
                        png_set_compression_level(png_ptr, Z_BEST_SPEED);
                    }
                    png_set_compression_strategy(png_ptr, compression_strategy);

                    png_set_IHDR( png_ptr, info_ptr, width, height, depth == CV_8U ? isBilevel?1:8 : 16,
                        channels == 1 ? PNG_COLOR_TYPE_GRAY :
                        channels == 3 ? PNG_COLOR_TYPE_RGB : PNG_COLOR_TYPE_RGBA,
                        PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
                        PNG_FILTER_TYPE_DEFAULT );

                    png_write_info( png_ptr, info_ptr );

                    if (isBilevel)
                        png_set_packing(png_ptr);

                    png_set_bgr( png_ptr );
                    if( !isBigEndian() )
                        png_set_swap( png_ptr );

                    buffer.allocate(height);
                    for( y = 0; y < height; y++ )
                        buffer[y] = img.data + y*img.step;

                    png_write_image( png_ptr, buffer.data() );
                    png_write_end( png_ptr, info_ptr );

                    result = true;
                }
            }
        }
    }

    png_destroy_write_struct( &png_ptr, &info_ptr );
    if(f) fclose( (FILE*)f );

    return result;
}

void ApngEncoder::optim_dirty(std::vector<APNGFrame>& frames)
{
    unsigned int i, j;
    unsigned char* sp;
    unsigned int size = frames[0].width() * frames[0].height();

    for (i = 0; i < frames.size(); i++)
    {
        sp = frames[i].pixels();
        for (j = 0; j < size; j++, sp += 4)
            if (sp[3] == 0)
                sp[0] = sp[1] = sp[2] = 0;
        process_callback(0.1 + i / float(frames.size()) * 0.1);
    }
}

void ApngEncoder::optim_duplicates(std::vector<APNGFrame>& frames, unsigned int first)
{
    unsigned int imagesize = frames[0].width() * frames[0].height() * 4;
    unsigned int i = first;

    while (++i < frames.size())
    {
        if (memcmp(frames[i - 1].pixels(), frames[i].pixels(), imagesize) != 0)
            continue;

        i--;
        delete[] frames[i].pixels();
        delete[] frames[i].rows();
        unsigned int num = frames[i].delayNum();
        unsigned int den = frames[i].delayDen();
        frames.erase(frames.begin() + i);

        if (frames[i].delayDen() == den)
            frames[i].delayNum(frames[i].delayNum()+num);
        else
        {
            frames[i].delayNum(num * frames[i].delayDen() + den * frames[i].delayNum());
            frames[i].delayDen(den * frames[i].delayDen());
            while (num && den)
            {
                if (num > den)
                    num = num % den;
                else
                    den = den % num;
            }
            num += den;
            frames[i].delayNum(frames[i].delayNum() / num);
            frames[i].delayDen(frames[i].delayDen() / num);
        }

        process_callback(0.2 + i / float(frames.size()) * 0.1);
    }
}

void ApngEncoder::optim_downconvert(std::vector<APNGFrame>& frames, unsigned int& coltype)
{
    unsigned int i, j, k, r, g, b, a;
    unsigned char* sp, * dp;
    unsigned char cube[4096];
    unsigned char gray[256];
    COLORS col[256];
    unsigned int colors = 0;
    unsigned int size = frames[0].width() * frames[0].height();
    unsigned int has_tcolor = 0;
    unsigned int num_frames = (int)frames.size();

    memset(&cube, 0, sizeof(cube));
    memset(&gray, 0, sizeof(gray));

    for (i = 0; i < 256; i++)
    {
        col[i].num = 0;
        col[i].r = col[i].g = col[i].b = i;
        col[i].a = trns[i] = 255;
    }
    palsize = trnssize = 0;
    coltype = 6;

    int transparent = 255;
    int simple_trans = 1;
    int grayscale = 1;

    for (i = 0; i < num_frames; i++)
    {
        sp = frames[i].pixels();
        for (j = 0; j < size; j++)
        {
            r = *sp++;
            g = *sp++;
            b = *sp++;
            a = *sp++;
            transparent &= a;

            if (a != 0)
            {
                if (a != 255)
                    simple_trans = 0;
                else if (((r | g | b) & 15) == 0)
                    cube[(r << 4) + g + (b >> 4)] = 1;

                if (r != g || g != b)
                    grayscale = 0;
                else
                    gray[r] = 1;
            }

            if (colors <= 256)
            {
                int found = 0;
                for (k = 0; k < colors; k++)
                    if (col[k].r == r && col[k].g == g && col[k].b == b && col[k].a == a)
                    {
                        found = 1;
                        col[k].num++;
                        break;
                    }
                if (found == 0)
                {
                    if (colors < 256)
                    {
                        col[colors].num++;
                        col[colors].r = r;
                        col[colors].g = g;
                        col[colors].b = b;
                        col[colors].a = a;
                        if (a == 0)
                            has_tcolor = 1;
                    }
                    colors++;
                }
            }
        }
        process_callback(0.3 + i / float(num_frames) * 0.1);
    }

    if (grayscale && simple_trans && colors <= 256) /* 6 -> 0 */
    {
        coltype = 0;

        for (i = 0; i < 256; i++)
            if (gray[i] == 0)
            {
                trns[0] = 0;
                trns[1] = i;
                trnssize = 2;
                break;
            }

        for (i = 0; i < num_frames; i++)
        {
            sp = dp = frames[i].pixels();
            for (j = 0; j < size; j++, sp += 4)
            {
                if (sp[3] == 0)
                    *dp++ = trns[1];
                else
                    *dp++ = sp[0];
            }
        }
    }
    else if (colors <= 256) /* 6 -> 3 */
    {
        coltype = 3;

        if (has_tcolor == 0 && colors < 256)
            col[colors++].a = 0;

        qsort(&col[0], colors, sizeof(COLORS), cmp_colors);

        palsize = colors;
        for (i = 0; i < colors; i++)
        {
            palette[i].r = col[i].r;
            palette[i].g = col[i].g;
            palette[i].b = col[i].b;
            trns[i] = col[i].a;
            if (trns[i] != 255)
                trnssize = i + 1;
        }

        for (i = 0; i < num_frames; i++)
        {
            sp = dp = frames[i].pixels();
            for (j = 0; j < size; j++)
            {
                r = *sp++;
                g = *sp++;
                b = *sp++;
                a = *sp++;
                for (k = 0; k < colors; k++)
                    if (col[k].r == r && col[k].g == g && col[k].b == b && col[k].a == a)
                        break;
                *dp++ = k;
            }
        }
    }
    else if (grayscale) /* 6 -> 4 */
    {
        coltype = 4;
        for (i = 0; i < num_frames; i++)
        {
            sp = dp = frames[i].pixels();
            for (j = 0; j < size; j++, sp += 4)
            {
                *dp++ = sp[2];
                *dp++ = sp[3];
            }
        }
    }
    else if (simple_trans) /* 6 -> 2 */
    {
        for (i = 0; i < 4096; i++)
            if (cube[i] == 0)
            {
                trns[0] = 0;
                trns[1] = (i >> 4) & 0xF0;
                trns[2] = 0;
                trns[3] = i & 0xF0;
                trns[4] = 0;
                trns[5] = (i << 4) & 0xF0;
                trnssize = 6;
                break;
            }
        if (transparent == 255)
        {
            coltype = 2;
            for (i = 0; i < num_frames; i++)
            {
                sp = dp = frames[i].pixels();
                for (j = 0; j < size; j++)
                {
                    *dp++ = *sp++;
                    *dp++ = *sp++;
                    *dp++ = *sp++;
                    sp++;
                }
            }
        }
        else if (trnssize != 0)
        {
            coltype = 2;
            for (i = 0; i < num_frames; i++)
            {
                sp = dp = frames[i].pixels();
                for (j = 0; j < size; j++)
                {
                    r = *sp++;
                    g = *sp++;
                    b = *sp++;
                    a = *sp++;
                    if (a == 0)
                    {
                        *dp++ = trns[1];
                        *dp++ = trns[3];
                        *dp++ = trns[5];
                    }
                    else
                    {
                        *dp++ = r;
                        *dp++ = g;
                        *dp++ = b;
                    }
                }
            }
        }
    }
}

void ApngEncoder::optim_image(std::vector<APNGFrame>& frames, unsigned int& coltype, int minQuality, int maxQuality)
{
    CV_UNUSED(frames);
    CV_UNUSED(coltype);
    CV_UNUSED(minQuality);
    CV_UNUSED(maxQuality);

    /*
    unsigned int size = frames.size();
    unsigned int width = frames[0].w;
    unsigned int height = frames[0].h;
    unsigned int imageSize = width * height * 4;
    liq_attr* attr = liq_attr_create();
    liq_set_quality(attr, minQuality, maxQuality);
    liq_histogram* hist = liq_histogram_create(attr);
    unsigned char* pixels[size];
    liq_image* images[size];

    for (int i = 0; i < size; i++)
    {
        unsigned char* raw_rgba_pixels = (unsigned char*)malloc(imageSize);
        memcpy(raw_rgba_pixels, frames[i].p, imageSize);

        liq_image* image = liq_image_create_rgba(attr, raw_rgba_pixels, width, height, 0);
        liq_histogram_add_image(hist, attr, image);

        pixels[i] = raw_rgba_pixels;
        images[i] = image;
        process_callback(0.3 + i / float(size) * 0.1);
    }

    liq_result* res;
    coltype = 3;
    int errorCode = liq_histogram_quantize(hist, attr, &res);
    if (errorCode == LIQ_OK)
    {
        for (int i = 0; i < size; i++)
        {
            liq_write_remapped_image(res, images[i], frames[i].p, imageSize);
        }
        const liq_palette* liqPalette = liq_get_palette(res);
        palsize = liqPalette->count;
        for (size_t i = 0; i < liqPalette->count; i++)
        {
            palette[i].r = liqPalette->entries[i].r;
            palette[i].g = liqPalette->entries[i].g;
            palette[i].b = liqPalette->entries[i].b;
            trns[i] = liqPalette->entries[i].a;
            if (trns[i] != 255)
                trnssize = i + 1;

            process_callback(0.4 + i / float(palsize) * 0.1);
        }
    }

    for (int i = 0; i < size; i++)
    {
        delete[] pixels[i];
        liq_image_destroy(images[i]);
    }
    liq_result_destroy(res);
    liq_attr_destroy(attr);
    liq_histogram_destroy(hist);
    */
}

void ApngEncoder::write_chunk(FILE* f, const char* name, unsigned char* data, unsigned int length)
{
    unsigned char buf[4];
    unsigned int crc = crc32(0, Z_NULL, 0);

    png_save_uint_32(buf, length);
    fwrite(buf, 1, 4, f);
    fwrite(name, 1, 4, f);
    crc = crc32(crc, (const Bytef*)name, 4);

    if (memcmp(name, "fdAT", 4) == 0)
    {
        png_save_uint_32(buf, next_seq_num++);
        fwrite(buf, 1, 4, f);
        crc = crc32(crc, buf, 4);
        length -= 4;
    }

    if (data != NULL && length > 0)
    {
        fwrite(data, 1, length, f);
        crc = crc32(crc, data, length);
    }

    png_save_uint_32(buf, crc);
    fwrite(buf, 1, 4, f);
}

void ApngEncoder::write_IDATs(FILE* f, int frame, unsigned char* data, unsigned int length, unsigned int idat_size)
{
    unsigned int z_cmf = data[0];
    if ((z_cmf & 0x0f) == 8 && (z_cmf & 0xf0) <= 0x70)
    {
        if (length >= 2)
        {
            unsigned int z_cinfo = z_cmf >> 4;
            unsigned int half_z_window_size = 1 << (z_cinfo + 7);
            while (idat_size <= half_z_window_size && half_z_window_size >= 256)
            {
                z_cinfo--;
                half_z_window_size >>= 1;
            }
            z_cmf = (z_cmf & 0x0f) | (z_cinfo << 4);
            if (data[0] != (unsigned char)z_cmf)
            {
                data[0] = (unsigned char)z_cmf;
                data[1] &= 0xe0;
                data[1] += (unsigned char)(0x1f - ((z_cmf << 8) + data[1]) % 0x1f);
            }
        }
    }

    while (length > 0)
    {
        unsigned int ds = length;
        if (ds > 32768)
            ds = 32768;

        if (frame == 0)
            write_chunk(f, "IDAT", data, ds);
        else
            write_chunk(f, "fdAT", data, ds + 4);

        data += ds;
        length -= ds;
    }
}

void ApngEncoder::process_rect(unsigned char* row, int rowbytes, int bpp, int stride, int h, unsigned char* rows)
{
    int i, j, v;
    int a, b, c, pa, pb, pc, p;
    unsigned char* prev = NULL;
    unsigned char* dp = rows;
    unsigned char* out;

    for (j = 0; j < h; j++)
    {
        unsigned int sum = 0;
        unsigned char* best_row = row_buf;
        unsigned int mins = ((unsigned int)(-1)) >> 1;

        out = row_buf + 1;
        for (i = 0; i < rowbytes; i++)
        {
            v = out[i] = row[i];
            sum += (v < 128) ? v : 256 - v;
        }
        mins = sum;

        sum = 0;
        out = sub_row + 1;
        for (i = 0; i < bpp; i++)
        {
            v = out[i] = row[i];
            sum += (v < 128) ? v : 256 - v;
        }
        for (i = bpp; i < rowbytes; i++)
        {
            v = out[i] = row[i] - row[i - bpp];
            sum += (v < 128) ? v : 256 - v;
            if (sum > mins)
                break;
        }
        if (sum < mins)
        {
            mins = sum;
            best_row = sub_row;
        }

        if (prev)
        {
            sum = 0;
            out = up_row + 1;
            for (i = 0; i < rowbytes; i++)
            {
                v = out[i] = row[i] - prev[i];
                sum += (v < 128) ? v : 256 - v;
                if (sum > mins)
                    break;
            }
            if (sum < mins)
            {
                mins = sum;
                best_row = up_row;
            }

            sum = 0;
            out = avg_row + 1;
            for (i = 0; i < bpp; i++)
            {
                v = out[i] = row[i] - prev[i] / 2;
                sum += (v < 128) ? v : 256 - v;
            }
            for (i = bpp; i < rowbytes; i++)
            {
                v = out[i] = row[i] - (prev[i] + row[i - bpp]) / 2;
                sum += (v < 128) ? v : 256 - v;
                if (sum > mins)
                    break;
            }
            if (sum < mins)
            {
                mins = sum;
                best_row = avg_row;
            }

            sum = 0;
            out = paeth_row + 1;
            for (i = 0; i < bpp; i++)
            {
                v = out[i] = row[i] - prev[i];
                sum += (v < 128) ? v : 256 - v;
            }
            for (i = bpp; i < rowbytes; i++)
            {
                a = row[i - bpp];
                b = prev[i];
                c = prev[i - bpp];
                p = b - c;
                pc = a - c;
                pa = abs(p);
                pb = abs(pc);
                pc = abs(p + pc);
                p = (pa <= pb && pa <= pc) ? a : (pb <= pc) ? b
                    : c;
                v = out[i] = row[i] - p;
                sum += (v < 128) ? v : 256 - v;
                if (sum > mins)
                    break;
            }
            if (sum < mins)
            {
                best_row = paeth_row;
            }
        }

        if (rows == NULL)
        {
            // deflate_rect_op()
            op_zstream1.next_in = row_buf;
            op_zstream1.avail_in = rowbytes + 1;
            deflate(&op_zstream1, Z_NO_FLUSH);

            op_zstream2.next_in = best_row;
            op_zstream2.avail_in = rowbytes + 1;
            deflate(&op_zstream2, Z_NO_FLUSH);
        }
        else
        {
            // deflate_rect_fin()
            memcpy(dp, best_row, rowbytes + 1);
            dp += rowbytes + 1;
        }

        prev = row;
        row += stride;
    }
}

void ApngEncoder::deflate_rect_op(unsigned char* pdata, int x, int y, int w, int h, int bpp, int stride, int zbuf_size, int n)
{
    unsigned char* row = pdata + y * stride + x * bpp;
    int rowbytes = w * bpp;

    op_zstream1.data_type = Z_BINARY;
    op_zstream1.next_out = op_zbuf1;
    op_zstream1.avail_out = zbuf_size;

    op_zstream2.data_type = Z_BINARY;
    op_zstream2.next_out = op_zbuf2;
    op_zstream2.avail_out = zbuf_size;

    process_rect(row, rowbytes, bpp, stride, h, NULL);

    deflate(&op_zstream1, Z_FINISH);
    deflate(&op_zstream2, Z_FINISH);
    op[n].p = pdata;

    if (op_zstream1.total_out < op_zstream2.total_out)
    {
        op[n].size = op_zstream1.total_out;
        op[n].filters = 0;
    }
    else
    {
        op[n].size = op_zstream2.total_out;
        op[n].filters = 1;
    }
    op[n].x = x;
    op[n].y = y;
    op[n].w = w;
    op[n].h = h;
    op[n].valid = 1;
    deflateReset(&op_zstream1);
    deflateReset(&op_zstream2);
}

void ApngEncoder::get_rect(unsigned int w, unsigned int h, unsigned char* pimage1, unsigned char* pimage2, unsigned char* ptemp, unsigned int bpp, unsigned int stride, int zbuf_size, unsigned int has_tcolor, unsigned int tcolor, int n)
{
    unsigned int i, j, x0, y0, w0, h0;
    unsigned int x_min = w - 1;
    unsigned int y_min = h - 1;
    unsigned int x_max = 0;
    unsigned int y_max = 0;
    unsigned int diffnum = 0;
    unsigned int over_is_possible = 1;

    if (!has_tcolor)
        over_is_possible = 0;

    if (bpp == 1)
    {
        unsigned char* pa = pimage1;
        unsigned char* pb = pimage2;
        unsigned char* pc = ptemp;

        for (j = 0; j < h; j++)
            for (i = 0; i < w; i++)
            {
                unsigned char c = *pb++;
                if (*pa++ != c)
                {
                    diffnum++;
                    if (has_tcolor && c == tcolor)
                        over_is_possible = 0;
                    if (i < x_min)
                        x_min = i;
                    if (i > x_max)
                        x_max = i;
                    if (j < y_min)
                        y_min = j;
                    if (j > y_max)
                        y_max = j;
                }
                else
                    c = tcolor;

                *pc++ = c;
            }
    }
    else if (bpp == 2)
    {
        unsigned short* pa = (unsigned short*)pimage1;
        unsigned short* pb = (unsigned short*)pimage2;
        unsigned short* pc = (unsigned short*)ptemp;

        for (j = 0; j < h; j++)
            for (i = 0; i < w; i++)
            {
                unsigned int c1 = *pa++;
                unsigned int c2 = *pb++;
                if ((c1 != c2) && ((c1 >> 8) || (c2 >> 8)))
                {
                    diffnum++;
                    if ((c2 >> 8) != 0xFF)
                        over_is_possible = 0;
                    if (i < x_min)
                        x_min = i;
                    if (i > x_max)
                        x_max = i;
                    if (j < y_min)
                        y_min = j;
                    if (j > y_max)
                        y_max = j;
                }
                else
                    c2 = 0;

                *pc++ = c2;
            }
    }
    else if (bpp == 3)
    {
        unsigned char* pa = pimage1;
        unsigned char* pb = pimage2;
        unsigned char* pc = ptemp;

        for (j = 0; j < h; j++)
            for (i = 0; i < w; i++)
            {
                unsigned int c1 = (pa[2] << 16) + (pa[1] << 8) + pa[0];
                unsigned int c2 = (pb[2] << 16) + (pb[1] << 8) + pb[0];
                if (c1 != c2)
                {
                    diffnum++;
                    if (has_tcolor && c2 == tcolor)
                        over_is_possible = 0;
                    if (i < x_min)
                        x_min = i;
                    if (i > x_max)
                        x_max = i;
                    if (j < y_min)
                        y_min = j;
                    if (j > y_max)
                        y_max = j;
                }
                else
                    c2 = tcolor;

                memcpy(pc, &c2, 3);
                pa += 3;
                pb += 3;
                pc += 3;
            }
    }
    else if (bpp == 4)
    {
        unsigned int* pa = (unsigned int*)pimage1;
        unsigned int* pb = (unsigned int*)pimage2;
        unsigned int* pc = (unsigned int*)ptemp;

        for (j = 0; j < h; j++)
            for (i = 0; i < w; i++)
            {
                unsigned int c1 = *pa++;
                unsigned int c2 = *pb++;
                if ((c1 != c2) && ((c1 >> 24) || (c2 >> 24)))
                {
                    diffnum++;
                    if ((c2 >> 24) != 0xFF)
                        over_is_possible = 0;
                    if (i < x_min)
                        x_min = i;
                    if (i > x_max)
                        x_max = i;
                    if (j < y_min)
                        y_min = j;
                    if (j > y_max)
                        y_max = j;
                }
                else
                    c2 = 0;

                *pc++ = c2;
            }
    }

    if (diffnum == 0)
    {
        x0 = y0 = 0;
        w0 = h0 = 1;
    }
    else
    {
        x0 = x_min;
        y0 = y_min;
        w0 = x_max - x_min + 1;
        h0 = y_max - y_min + 1;
    }

    deflate_rect_op(pimage2, x0, y0, w0, h0, bpp, stride, zbuf_size, n * 2);

    if (over_is_possible)
        deflate_rect_op(ptemp, x0, y0, w0, h0, bpp, stride, zbuf_size, n * 2 + 1);
}

void ApngEncoder::deflate_rect_fin(int deflate_method, int iter, unsigned char* zbuf, unsigned int* zsize, int bpp, int stride, unsigned char* rows, int zbuf_size, int n)
{
    unsigned char* row = op[n].p + op[n].y * stride + op[n].x * bpp;
    int rowbytes = op[n].w * bpp;

    if (op[n].filters == 0)
    {
        unsigned char* dp = rows;
        for (int j = 0; j < op[n].h; j++)
        {
            *dp++ = 0;
            memcpy(dp, row, rowbytes);
            dp += rowbytes;
            row += stride;
        }
    }
    else
        process_rect(row, rowbytes, bpp, stride, op[n].h, rows);

    if (deflate_method == 2)
    {
        CV_UNUSED(iter);
        /*ZopfliOptions opt_zopfli;
        unsigned char* data = 0;
        size_t size = 0;
        ZopfliInitOptions(&opt_zopfli);
        opt_zopfli.numiterations = iter;
        ZopfliCompress(&opt_zopfli, ZOPFLI_FORMAT_ZLIB, rows, op[n].h * (rowbytes + 1), &data, &size);
        if (size < (size_t)zbuf_size)
        {
            memcpy(zbuf, data, size);
            *zsize = size;
        }
        free(data);*/
    }
    else if (deflate_method == 1)
    {
        unsigned size = zbuf_size;
        //compress_rfc1950_7z(rows, op[n].h * (rowbytes + 1), zbuf, size, iter < 100 ? iter : 100, 255);
        *zsize = size;
    }
    else
    {
        z_stream fin_zstream;

        fin_zstream.data_type = Z_BINARY;
        fin_zstream.zalloc = Z_NULL;
        fin_zstream.zfree = Z_NULL;
        fin_zstream.opaque = Z_NULL;
        deflateInit2(&fin_zstream, Z_BEST_COMPRESSION, 8, 15, 8, op[n].filters ? Z_FILTERED : Z_DEFAULT_STRATEGY);

        fin_zstream.next_out = zbuf;
        fin_zstream.avail_out = zbuf_size;
        fin_zstream.next_in = rows;
        fin_zstream.avail_in = op[n].h * (rowbytes + 1);
        deflate(&fin_zstream, Z_FINISH);
        *zsize = fin_zstream.total_out;
        deflateEnd(&fin_zstream);
    }
}

size_t ApngEncoder::save_apng(std::string outputFileName, std::vector<APNGFrame>& frames, unsigned int first, unsigned int loops, unsigned int coltype, int deflate_method, int iter)
{
    FILE* f;
    unsigned int i, j, k;
    unsigned int x0, y0, w0, h0, dop, bop;
    unsigned int idat_size, zbuf_size, zsize;
    unsigned char* zbuf;
    unsigned char header[8] = { 137, 80, 78, 71, 13, 10, 26, 10 };
    unsigned int num_frames = (int)frames.size();
    unsigned int width = frames[0].width();
    unsigned int height = frames[0].height();
    unsigned int bpp = (coltype == 6) ? 4 : (coltype == 2) ? 3
        : (coltype == 4) ? 2
        : 1;
    unsigned int has_tcolor = (coltype >= 4 || (coltype <= 2 && trnssize)) ? 1 : 0;
    unsigned int tcolor = 0;
    unsigned int rowbytes = width * bpp;
    unsigned int imagesize = rowbytes * height;
    size_t fileSize = 0;

    unsigned char* temp = new unsigned char[imagesize];
    unsigned char* over1 = new unsigned char[imagesize];
    unsigned char* over2 = new unsigned char[imagesize];
    unsigned char* over3 = new unsigned char[imagesize];
    unsigned char* rest = new unsigned char[imagesize];
    unsigned char* rows = new unsigned char[(rowbytes + 1) * height];

    if (trnssize)
    {
        if (coltype == 0)
            tcolor = trns[1];
        else if (coltype == 2)
            tcolor = (((trns[5] << 8) + trns[3]) << 8) + trns[1];
        else if (coltype == 3)
        {
            for (i = 0; i < trnssize; i++)
                if (trns[i] == 0)
                {
                    has_tcolor = 1;
                    tcolor = i;
                    break;
                }
        }
    }

    if ((f = fopen(outputFileName.c_str(), "wb")) != 0)
    {
        unsigned char buf_IHDR[13];
        unsigned char buf_acTL[8];
        unsigned char buf_fcTL[26];

        png_save_uint_32(buf_IHDR, width);
        png_save_uint_32(buf_IHDR + 4, height);
        buf_IHDR[8] = 8;
        buf_IHDR[9] = coltype;
        buf_IHDR[10] = 0;
        buf_IHDR[11] = 0;
        buf_IHDR[12] = 0;

        png_save_uint_32(buf_acTL, num_frames - first);
        png_save_uint_32(buf_acTL + 4, loops);

        fwrite(header, 1, 8, f);

        write_chunk(f, "IHDR", buf_IHDR, 13);

        if (num_frames > 1)
            write_chunk(f, "acTL", buf_acTL, 8);
        else
            first = 0;

        if (palsize > 0)
            write_chunk(f, "PLTE", (unsigned char*)(&palette), palsize * 3);

        if (trnssize > 0)
            write_chunk(f, "tRNS", trns, trnssize);

        op_zstream1.data_type = Z_BINARY;
        op_zstream1.zalloc = Z_NULL;
        op_zstream1.zfree = Z_NULL;
        op_zstream1.opaque = Z_NULL;
        deflateInit2(&op_zstream1, Z_BEST_SPEED + 1, 8, 15, 8, Z_DEFAULT_STRATEGY);

        op_zstream2.data_type = Z_BINARY;
        op_zstream2.zalloc = Z_NULL;
        op_zstream2.zfree = Z_NULL;
        op_zstream2.opaque = Z_NULL;
        deflateInit2(&op_zstream2, Z_BEST_SPEED + 1, 8, 15, 8, Z_FILTERED);

        idat_size = (rowbytes + 1) * height;
        zbuf_size = idat_size + ((idat_size + 7) >> 3) + ((idat_size + 63) >> 6) + 11;

        zbuf = new unsigned char[zbuf_size];
        op_zbuf1 = new unsigned char[zbuf_size];
        op_zbuf2 = new unsigned char[zbuf_size];
        row_buf = new unsigned char[rowbytes + 1];
        sub_row = new unsigned char[rowbytes + 1];
        up_row = new unsigned char[rowbytes + 1];
        avg_row = new unsigned char[rowbytes + 1];
        paeth_row = new unsigned char[rowbytes + 1];

        row_buf[0] = 0;
        sub_row[0] = 1;
        up_row[0] = 2;
        avg_row[0] = 3;
        paeth_row[0] = 4;

        x0 = 0;
        y0 = 0;
        w0 = width;
        h0 = height;
        bop = 0;
        next_seq_num = 0;

        // printf("saving %s (frame %d of %d)\n", szOut, 1-first, num_frames-first);
        for (j = 0; j < 6; j++)
            op[j].valid = 0;
        deflate_rect_op(frames[0].pixels(), x0, y0, w0, h0, bpp, rowbytes, zbuf_size, 0);
        deflate_rect_fin(deflate_method, iter, zbuf, &zsize, bpp, rowbytes, rows, zbuf_size, 0);

        process_callback(0.5 + ((1 - first) / float(num_frames - first)) * 0.5);
        if (first)
        {
            write_IDATs(f, 0, zbuf, zsize, idat_size);
            process_callback(0.5 + ((1) / float(num_frames - first)) * 0.5);
            for (j = 0; j < 6; j++)
                op[j].valid = 0;
            deflate_rect_op(frames[1].pixels(), x0, y0, w0, h0, bpp, rowbytes, zbuf_size, 0);
            deflate_rect_fin(deflate_method, iter, zbuf, &zsize, bpp, rowbytes, rows, zbuf_size, 0);
        }

        for (i = first; i < num_frames - 1; i++)
        {
            unsigned int op_min;
            int op_best;
            process_callback(0.5 + ((i - first + 2) / float(num_frames - first)) * 0.5);
            for (j = 0; j < 6; j++)
                op[j].valid = 0;

            /* dispose = none */
            get_rect(width, height, frames[i].pixels(), frames[i + 1].pixels(), over1, bpp, rowbytes, zbuf_size, has_tcolor, tcolor, 0);

            /* dispose = background */
            if (has_tcolor)
            {
                memcpy(temp, frames[i].pixels(), imagesize);
                if (coltype == 2)
                    for (j = 0; j < h0; j++)
                        for (k = 0; k < w0; k++)
                            memcpy(temp + ((j + y0) * width + (k + x0)) * 3, &tcolor, 3);
                else
                    for (j = 0; j < h0; j++)
                        memset(temp + ((j + y0) * width + x0) * bpp, tcolor, w0 * bpp);

                get_rect(width, height, temp, frames[i + 1].pixels(), over2, bpp, rowbytes, zbuf_size, has_tcolor, tcolor, 1);
            }

            /* dispose = previous */
            if (i > first)
                get_rect(width, height, rest, frames[i + 1].pixels(), over3, bpp, rowbytes, zbuf_size, has_tcolor, tcolor, 2);

            op_min = op[0].size;
            op_best = 0;
            for (j = 1; j < 6; j++)
                if (op[j].valid)
                {
                    if (op[j].size < op_min)
                    {
                        op_min = op[j].size;
                        op_best = j;
                    }
                }

            dop = op_best >> 1;

            png_save_uint_32(buf_fcTL, next_seq_num++);
            png_save_uint_32(buf_fcTL + 4, w0);
            png_save_uint_32(buf_fcTL + 8, h0);
            png_save_uint_32(buf_fcTL + 12, x0);
            png_save_uint_32(buf_fcTL + 16, y0);
            png_save_uint_16(buf_fcTL + 20, frames[i].delayNum());
            png_save_uint_16(buf_fcTL + 22, frames[i].delayDen());
            buf_fcTL[24] = dop;
            buf_fcTL[25] = bop;
            write_chunk(f, "fcTL", buf_fcTL, 26);

            write_IDATs(f, i, zbuf, zsize, idat_size);

            /* process apng dispose - begin */
            if (dop != 2)
                memcpy(rest, frames[i].pixels(), imagesize);

            if (dop == 1)
            {
                if (coltype == 2)
                    for (j = 0; j < h0; j++)
                        for (k = 0; k < w0; k++)
                            memcpy(rest + ((j + y0) * width + (k + x0)) * 3, &tcolor, 3);
                else
                    for (j = 0; j < h0; j++)
                        memset(rest + ((j + y0) * width + x0) * bpp, tcolor, w0 * bpp);
            }
            /* process apng dispose - end */

            x0 = op[op_best].x;
            y0 = op[op_best].y;
            w0 = op[op_best].w;
            h0 = op[op_best].h;
            bop = op_best & 1;

            deflate_rect_fin(deflate_method, iter, zbuf, &zsize, bpp, rowbytes, rows, zbuf_size, op_best);
        }

        if (num_frames > 1)
        {
            png_save_uint_32(buf_fcTL, next_seq_num++);
            png_save_uint_32(buf_fcTL + 4, w0);
            png_save_uint_32(buf_fcTL + 8, h0);
            png_save_uint_32(buf_fcTL + 12, x0);
            png_save_uint_32(buf_fcTL + 16, y0);
            png_save_uint_16(buf_fcTL + 20, frames[num_frames - 1].delayNum());
            png_save_uint_16(buf_fcTL + 22, frames[num_frames - 1].delayDen());
            buf_fcTL[24] = 0;
            buf_fcTL[25] = bop;
            write_chunk(f, "fcTL", buf_fcTL, 26);
        }

        write_IDATs(f, num_frames - 1, zbuf, zsize, idat_size);

        write_chunk(f, "IEND", 0, 0);

        fclose(f);

        delete[] zbuf;
        delete[] op_zbuf1;
        delete[] op_zbuf2;
        delete[] row_buf;
        delete[] sub_row;
        delete[] up_row;
        delete[] avg_row;
        delete[] paeth_row;

        deflateEnd(&op_zstream1);
        deflateEnd(&op_zstream2);
    }

    delete[] temp;
    delete[] over1;
    delete[] over2;
    delete[] over3;
    delete[] rest;
    delete[] rows;

    return fileSize;
}

bool ApngEncoder::writemulti(const std::vector<Mat>& img_vec, const std::vector<int>& params)
{
    // ******  WIP *****
    // ****** test *****
    CV_UNUSED(img_vec);
    CV_UNUSED(params);
 /*
    std::vector<Image> imgs;

    int r1 = load_apng(m_filename, imgs);
    save_apng("test_new"+m_filename, imgs, 0, 0, 6, 0, 0);
    printf("writemulti %d",r1);

 */
    return true;
}

}

#endif

/* End of file. */
