// This file is part of OpenCV project.
// It is subject to the license terms in the LICENSE file found in the top-level
// directory of this distribution and at http://opencv.org/license.html

#ifndef _GRFMT_APNG_H_
#define _GRFMT_APNG_H_

#ifdef HAVE_PNG

#include "grfmt_base.hpp"
#include "bitstrm.hpp"
#include <png.h>
#include <zlib.h>

namespace cv
{
    struct CHUNK { unsigned char* p; unsigned int size; };
    struct COLORS { unsigned int num; unsigned char r, g, b, a; };
    struct OP { unsigned char* p; unsigned int size; int x, y, w, h, valid, filters; };

    const unsigned DEFAULT_FRAME_NUMERATOR =
        100; //!< @brief The default numerator for the frame delay fraction.
    const unsigned DEFAULT_FRAME_DENOMINATOR =
        1000; //!< @brief The default denominator for the frame delay fraction.

    typedef struct {
        unsigned char r, g, b;
    } rgb;
    typedef struct {
        unsigned char r, g, b, a;
    } rgba;


    class APNGFrame {
    public:
        // Raw pixel data
        unsigned char* pixels(unsigned char* setPixels = NULL);
        unsigned char* _pixels;

        // Width and Height
        unsigned int width(unsigned int setWidth = 0);
        unsigned int height(unsigned int setHeight = 0);
        unsigned int _width;
        unsigned int _height;

        // PNG color type
        unsigned char colorType(unsigned char setColorType = 255);
        unsigned char _colorType;

        // Palette into
        rgb* palette(rgb* setPalette = NULL);
        rgb _palette[256];

        // Transparency info
        unsigned char* transparency(unsigned char* setTransparency = NULL);
        unsigned char _transparency[256];

        // Sizes for palette and transparency records
        int paletteSize(int setPaletteSize = 0);
        int _paletteSize;

        int transparencySize(int setTransparencySize = 0);
        int _transparencySize;

        // Delay is numerator/denominator ratio, in seconds
        unsigned int delayNum(unsigned int setDelayNum = 0);
        unsigned int _delayNum;

        unsigned int delayDen(unsigned int setDelayDen = 0);
        unsigned int _delayDen;

        unsigned char** rows(unsigned char** setRows = NULL);
        unsigned char** _rows;

        /**
         * @brief Creates an empty APNGFrame.
         */
        APNGFrame();

        /**
         * @brief Creates an APNGFrame from a PNG file.
         * @param filePath The relative or absolute path to an image file.
         * @param delayNum The delay numerator for this frame (defaults to
         * DEFAULT_FRAME_NUMERATOR).
         * @param delayDen The delay denominator for this frame (defaults to
         * DEFAULT_FRAME_DENMINATOR).
         */
        APNGFrame(const std::string& filePath,
            unsigned delayNum = DEFAULT_FRAME_NUMERATOR,
            unsigned delayDen = DEFAULT_FRAME_DENOMINATOR);

        /**
         * @brief Creates an APNGFrame from a bitmapped array of RBG pixel data.
         * @param pixels The RGB pixel data.
         * @param width The width of the pixel data.
         * @param height The height of the pixel data.
         * @param delayNum The delay numerator for this frame (defaults to
         * DEFAULT_FRAME_NUMERATOR).
         * @param delayDen The delay denominator for this frame (defaults to
         * DEFAULT_FRAME_DENMINATOR).
         */
        APNGFrame(rgb* pixels, unsigned int width, unsigned int height,
            unsigned delayNum = DEFAULT_FRAME_NUMERATOR,
            unsigned delayDen = DEFAULT_FRAME_DENOMINATOR);

        /**
         * @brief Creates an APNGFrame from a bitmapped array of RBG pixel data.
         * @param pixels The RGB pixel data.
         * @param width The width of the pixel data.
         * @param height The height of the pixel data.
         * @param trns_color An array of transparency data.
         * @param delayNum The delay numerator for this frame (defaults to
         * DEFAULT_FRAME_NUMERATOR).
         * @param delayDen The delay denominator for this frame (defaults to
         * DEFAULT_FRAME_DENMINATOR).
         */
        APNGFrame(rgb* pixels, unsigned int width, unsigned int height,
            rgb* trns_color = NULL, unsigned delayNum = DEFAULT_FRAME_NUMERATOR,
            unsigned delayDen = DEFAULT_FRAME_DENOMINATOR);

        /**
         * @brief Creates an APNGFrame from a bitmapped array of RBGA pixel data.
         * @param pixels The RGBA pixel data.
         * @param width The width of the pixel data.
         * @param height The height of the pixel data.
         * @param delayNum The delay numerator for this frame (defaults to
         * DEFAULT_FRAME_NUMERATOR).
         * @param delayDen The delay denominator for this frame (defaults to
         * DEFAULT_FRAME_DENMINATOR).
         */
        APNGFrame(rgba* pixels, unsigned int width, unsigned int height,
            unsigned delayNum = DEFAULT_FRAME_NUMERATOR,
            unsigned delayDen = DEFAULT_FRAME_DENOMINATOR);

        /**
         * @brief Saves this frame as a single PNG file.
         * @param outPath The relative or absolute path to save the image file to.
         * @return Returns true if save was successful.
         */
        bool save(const std::string& outPath) const;

    private:
    };

class ApngDecoder CV_FINAL : public BaseImageDecoder
{
public:

    ApngDecoder();
    virtual ~ApngDecoder();

    int load_apng(std::string inputFileName, std::vector<APNGFrame>& frames, unsigned int& first, unsigned int& loops);
    bool  readData( Mat& img ) CV_OVERRIDE;
    bool  readHeader() CV_OVERRIDE;
    bool  setSource(const String& filename) CV_OVERRIDE;
    void  close();

    ImageDecoder newDecoder() const CV_OVERRIDE;

protected:

    unsigned int read_chunk(FILE* f, CHUNK* pChunk);
    int processing_start(png_structp& png_ptr, png_infop& info_ptr, void* frame_ptr, bool hasInfo, CHUNK& chunkIHDR, std::vector<CHUNK>& chunksInfo);
    int processing_data(png_structp png_ptr, png_infop info_ptr, unsigned char* p, unsigned int size);
    int processing_finish(png_structp png_ptr, png_infop info_ptr);
    void compose_frame(unsigned char** rows_dst, unsigned char** rows_src, unsigned char bop, unsigned int x, unsigned int y, unsigned int w, unsigned int h);
    static void readDataFromBuf(void* png_ptr, uchar* dst, size_t size);

    int   m_bit_depth;
    void* m_png_ptr;  // pointer to decompression structure
    void* m_info_ptr; // pointer to image information structure
    void* m_end_info; // pointer to one more image information structure
    FILE* m_f;
    int   m_color_type;
    size_t m_buf_pos;
};


class ApngEncoder CV_FINAL : public BaseImageEncoder
{
public:
    ApngEncoder();
    virtual ~ApngEncoder();

    bool isFormatSupported( int depth ) const CV_OVERRIDE;
    bool write( const Mat& img, const std::vector<int>& params ) CV_OVERRIDE;
    bool writemulti(const std::vector<Mat>& img_vec, const std::vector<int>& params) CV_OVERRIDE;
    size_t save_apng(std::string inputFileName, std::vector<APNGFrame>& frames, unsigned int first, unsigned int loops, unsigned int coltype, int deflate_method, int iter);
    void optim_dirty(std::vector<APNGFrame>& frames);
    void optim_duplicates(std::vector<APNGFrame>& frames, unsigned int first);
    void optim_downconvert(std::vector<APNGFrame>& frames, unsigned int& coltype);
    void optim_image(std::vector<APNGFrame>& frames, unsigned int& coltype, int minQuality, int maxQuality);

    ImageEncoder newEncoder() const CV_OVERRIDE;

protected:
    static void writeDataToBuf(void* png_ptr, uchar* src, size_t size);
    static void flushBuf(void* png_ptr);

private:
    void write_chunk(FILE* f, const char* name, unsigned char* data, unsigned int length);
    void write_IDATs(FILE* f, int frame, unsigned char* data, unsigned int length, unsigned int idat_size);
    void process_rect(unsigned char* row, int rowbytes, int bpp, int stride, int h, unsigned char* rows);
    void deflate_rect_fin(int deflate_method, int iter, unsigned char* zbuf, unsigned int* zsize, int bpp, int stride, unsigned char* rows, int zbuf_size, int n);
    void deflate_rect_op(unsigned char* pdata, int x, int y, int w, int h, int bpp, int stride, int zbuf_size, int n);
    void get_rect(unsigned int w, unsigned int h, unsigned char* pimage1, unsigned char* pimage2, unsigned char* ptemp, unsigned int bpp, unsigned int stride, int zbuf_size, unsigned int has_tcolor, unsigned int tcolor, int n);

    void (*process_callback)(float);
    unsigned char* op_zbuf1;
    unsigned char* op_zbuf2;
    z_stream       op_zstream1;
    z_stream       op_zstream2;
    unsigned char* row_buf;
    unsigned char* sub_row;
    unsigned char* up_row;
    unsigned char* avg_row;
    unsigned char* paeth_row;
    OP             op[6];
    rgb            palette[256];
    unsigned char  trns[256];
    unsigned int   palsize, trnssize;
    unsigned int   next_seq_num;
};

}

#endif

#endif/*_GRFMT_PNG_H_*/
