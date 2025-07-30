/*M///////////////////////////////////////////////////////////////////////////////////////
//
//  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
//
//  By downloading, copying, installing or using the software you agree to this license.
//  If you do not agree to this license, do not download, install,
//  copy or use the software.
//
//
//                        Intel License Agreement
//                For Open Source Computer Vision Library
//
// Copyright (C) 2000, Intel Corporation, all rights reserved.
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
//   * The name of Intel Corporation may not be used to endorse or promote products
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

//
//  Loading and saving images.
//

#include "precomp.hpp"
#include "grfmts.hpp"
#include "utils.hpp"
#include "exif.hpp"
#undef min
#undef max
#include <iostream>
#include <fstream>
#include <cerrno>
#include <opencv2/core/utils/logger.hpp>
#include <opencv2/core/utils/configuration.private.hpp>
#include <opencv2/imgcodecs.hpp>



/****************************************************************************************\
*                                      Image Codecs                                      *
\****************************************************************************************/

namespace cv {

size_t tagTypeSize(cv::ExifTagType);
std::string exifTagIdToString(cv::ExifTagId);
size_t tagValueSize(cv::ExifTagType, size_t);

static const size_t CV_IO_MAX_IMAGE_PARAMS = cv::utils::getConfigurationParameterSizeT("OPENCV_IO_MAX_IMAGE_PARAMS", 50);
static const size_t CV_IO_MAX_IMAGE_WIDTH = utils::getConfigurationParameterSizeT("OPENCV_IO_MAX_IMAGE_WIDTH", 1 << 20);
static const size_t CV_IO_MAX_IMAGE_HEIGHT = utils::getConfigurationParameterSizeT("OPENCV_IO_MAX_IMAGE_HEIGHT", 1 << 20);
static const size_t CV_IO_MAX_IMAGE_PIXELS = utils::getConfigurationParameterSizeT("OPENCV_IO_MAX_IMAGE_PIXELS", 1 << 30);

static Size validateInputImageSize(const Size& size)
{
    CV_Assert(size.width > 0);
    CV_Assert(static_cast<size_t>(size.width) <= CV_IO_MAX_IMAGE_WIDTH);
    CV_Assert(size.height > 0);
    CV_Assert(static_cast<size_t>(size.height) <= CV_IO_MAX_IMAGE_HEIGHT);
    uint64 pixels = (uint64)size.width * (uint64)size.height;
    CV_Assert(pixels <= CV_IO_MAX_IMAGE_PIXELS);
    return size;
}


static inline int calcType(int type, int flags)
{
    if(flags != IMREAD_UNCHANGED)
    {
        CV_CheckNE(flags & (IMREAD_COLOR_BGR | IMREAD_COLOR_RGB),
                   IMREAD_COLOR_BGR | IMREAD_COLOR_RGB,
                   "IMREAD_COLOR_BGR (IMREAD_COLOR) and IMREAD_COLOR_RGB can not be set at the same time.");
    }

    if ( (flags & (IMREAD_COLOR | IMREAD_ANYCOLOR | IMREAD_ANYDEPTH)) == (IMREAD_COLOR | IMREAD_ANYCOLOR | IMREAD_ANYDEPTH))
        return type;

    if( (flags & IMREAD_LOAD_GDAL) != IMREAD_LOAD_GDAL && flags != IMREAD_UNCHANGED )
    {
        if( (flags & IMREAD_ANYDEPTH) == 0 )
            type = CV_MAKETYPE(CV_8U, CV_MAT_CN(type));

        if( (flags & IMREAD_COLOR) != 0 || (flags & IMREAD_COLOR_RGB) != 0 ||
           ((flags & IMREAD_ANYCOLOR) != 0 && CV_MAT_CN(type) > 1) )
            type = CV_MAKETYPE(CV_MAT_DEPTH(type), 3);
        else
            type = CV_MAKETYPE(CV_MAT_DEPTH(type), 1);
    }
    return type;
}

namespace {

class ByteStreamBuffer: public std::streambuf
{
public:
    ByteStreamBuffer(char* base, size_t length)
    {
        setg(base, base, base + length);
    }

protected:
    virtual pos_type seekoff( off_type offset,
                              std::ios_base::seekdir dir,
                              std::ios_base::openmode ) CV_OVERRIDE
    {
        char* whence = eback();
        if (dir == std::ios_base::cur)
        {
            whence = gptr();
        }
        else if (dir == std::ios_base::end)
        {
            whence = egptr();
        }
        char* to = whence + offset;

        // check limits
        if (to >= eback() && to <= egptr())
        {
            setg(eback(), to, egptr());
            return gptr() - eback();
        }

        return -1;
    }
};

}

/**
 * @struct ImageCodecInitializer
 *
 * Container which stores the registered codecs to be used by OpenCV
*/
struct ImageCodecInitializer
{
    /**
     * Default Constructor for the ImageCodeInitializer
    */
    ImageCodecInitializer()
    {
        /// BMP Support
        decoders.push_back( makePtr<BmpDecoder>() );
        encoders.push_back( makePtr<BmpEncoder>() );

    #ifdef HAVE_IMGCODEC_GIF
        decoders.push_back( makePtr<GifDecoder>() );
        encoders.push_back( makePtr<GifEncoder>() );
    #endif
    #ifdef HAVE_AVIF
        decoders.push_back(makePtr<AvifDecoder>());
        encoders.push_back(makePtr<AvifEncoder>());
    #endif
    #ifdef HAVE_IMGCODEC_HDR
        decoders.push_back( makePtr<HdrDecoder>() );
        encoders.push_back( makePtr<HdrEncoder>() );
    #endif
    #ifdef HAVE_JPEG
        decoders.push_back( makePtr<JpegDecoder>() );
        encoders.push_back( makePtr<JpegEncoder>() );
    #endif
    #ifdef HAVE_WEBP
        decoders.push_back( makePtr<WebPDecoder>() );
        encoders.push_back( makePtr<WebPEncoder>() );
    #endif
    #ifdef HAVE_IMGCODEC_SUNRASTER
        decoders.push_back( makePtr<SunRasterDecoder>() );
        encoders.push_back( makePtr<SunRasterEncoder>() );
    #endif
    #ifdef HAVE_IMGCODEC_PXM
        decoders.push_back( makePtr<PxMDecoder>() );
        encoders.push_back( makePtr<PxMEncoder>(PXM_TYPE_AUTO) );
        encoders.push_back( makePtr<PxMEncoder>(PXM_TYPE_PBM) );
        encoders.push_back( makePtr<PxMEncoder>(PXM_TYPE_PGM) );
        encoders.push_back( makePtr<PxMEncoder>(PXM_TYPE_PPM) );
        decoders.push_back( makePtr<PAMDecoder>() );
        encoders.push_back( makePtr<PAMEncoder>() );
    #endif
    #ifdef HAVE_IMGCODEC_PFM
        decoders.push_back( makePtr<PFMDecoder>() );
        encoders.push_back( makePtr<PFMEncoder>() );
    #endif
    #ifdef HAVE_TIFF
        decoders.push_back( makePtr<TiffDecoder>() );
        encoders.push_back( makePtr<TiffEncoder>() );
    #endif
    #ifdef HAVE_SPNG
        decoders.push_back( makePtr<SPngDecoder>() );
        encoders.push_back( makePtr<SPngEncoder>() );
    #elif defined(HAVE_PNG)
        decoders.push_back( makePtr<PngDecoder>() );
        encoders.push_back( makePtr<PngEncoder>() );
    #endif
    #ifdef HAVE_GDCM
        decoders.push_back( makePtr<DICOMDecoder>() );
    #endif
    #ifdef HAVE_JASPER
        decoders.push_back( makePtr<Jpeg2KDecoder>() );
        encoders.push_back( makePtr<Jpeg2KEncoder>() );
    #endif
    #ifdef HAVE_JPEGXL
        decoders.push_back( makePtr<JpegXLDecoder>() );
        encoders.push_back( makePtr<JpegXLEncoder>() );
    #endif
    #ifdef HAVE_OPENJPEG
        decoders.push_back( makePtr<Jpeg2KJP2OpjDecoder>() );
        decoders.push_back( makePtr<Jpeg2KJ2KOpjDecoder>() );
        encoders.push_back( makePtr<Jpeg2KOpjEncoder>() );
    #endif
    #ifdef HAVE_OPENEXR
        decoders.push_back( makePtr<ExrDecoder>() );
        encoders.push_back( makePtr<ExrEncoder>() );
    #endif

    #ifdef HAVE_GDAL
        /// Attach the GDAL Decoder
        decoders.push_back( makePtr<GdalDecoder>() );
    #endif/*HAVE_GDAL*/
    }

    std::vector<ImageDecoder> decoders;
    std::vector<ImageEncoder> encoders;
};

static
ImageCodecInitializer& getCodecs()
{
    static ImageCodecInitializer g_codecs;
    return g_codecs;
}

/**
 * Find the decoders
 *
 * @param[in] filename File to search
 *
 * @return Image decoder to parse image file.
*/
static ImageDecoder findDecoder( const String& filename ) {

    size_t i, maxlen = 0;

    /// iterate through list of registered codecs
    ImageCodecInitializer& codecs = getCodecs();
    for( i = 0; i < codecs.decoders.size(); i++ )
    {
        size_t len = codecs.decoders[i]->signatureLength();
        maxlen = std::max(maxlen, len);
    }

    /// Open the file
    FILE* f= fopen( filename.c_str(), "rb" );

    /// in the event of a failure, return an empty image decoder
    if( !f ) {
        CV_LOG_WARNING(NULL, "imread_('" << filename << "'): can't open/read file: check file path/integrity");
        return ImageDecoder();
    }

    // read the file signature
    String signature(maxlen, ' ');
    maxlen = fread( (void*)signature.c_str(), 1, maxlen, f );
    fclose(f);
    signature = signature.substr(0, maxlen);

    /// compare signature against all decoders
    for( i = 0; i < codecs.decoders.size(); i++ )
    {
        if( codecs.decoders[i]->checkSignature(signature) )
            return codecs.decoders[i]->newDecoder();
    }

    /// If no decoder was found, return base type
    return ImageDecoder();
}

static ImageDecoder findDecoder( const Mat& buf )
{
    size_t i, maxlen = 0;

    if( buf.rows*buf.cols < 1 || !buf.isContinuous() )
        return ImageDecoder();

    ImageCodecInitializer& codecs = getCodecs();
    for( i = 0; i < codecs.decoders.size(); i++ )
    {
        size_t len = codecs.decoders[i]->signatureLength();
        maxlen = std::max(maxlen, len);
    }

    String signature(maxlen, ' ');
    size_t bufSize = buf.rows*buf.cols*buf.elemSize();
    maxlen = std::min(maxlen, bufSize);
    memcpy( (void*)signature.c_str(), buf.data, maxlen );

    for( i = 0; i < codecs.decoders.size(); i++ )
    {
        if( codecs.decoders[i]->checkSignature(signature) )
            return codecs.decoders[i]->newDecoder();
    }

    return ImageDecoder();
}

static ImageEncoder findEncoder( const String& _ext )
{
    if( _ext.size() <= 1 )
        return ImageEncoder();

    const char* ext = strrchr( _ext.c_str(), '.' );
    if( !ext )
        return ImageEncoder();
    int len = 0;
    for( ext++; len < 128 && isalnum(ext[len]); len++ )
        ;

    ImageCodecInitializer& codecs = getCodecs();
    for( size_t i = 0; i < codecs.encoders.size(); i++ )
    {
        String description = codecs.encoders[i]->getDescription();
        const char* descr = strchr( description.c_str(), '(' );

        while( descr )
        {
            descr = strchr( descr + 1, '.' );
            if( !descr )
                break;
            int j = 0;
            for( descr++; j < len && isalnum(descr[j]) ; j++ )
            {
                int c1 = tolower(ext[j]);
                int c2 = tolower(descr[j]);
                if( c1 != c2 )
                    break;
            }
            if( j == len && !isalnum(descr[j]))
                return codecs.encoders[i]->newEncoder();
            descr += j;
        }
    }

    return ImageEncoder();
}


static void ExifTransform(int orientation, OutputArray img)
{
    switch( orientation )
    {
        case    IMAGE_ORIENTATION_TL: //0th row == visual top, 0th column == visual left-hand side
            //do nothing, the image already has proper orientation
            break;
        case    IMAGE_ORIENTATION_TR: //0th row == visual top, 0th column == visual right-hand side
            flip(img, img, 1); //flip horizontally
            break;
        case    IMAGE_ORIENTATION_BR: //0th row == visual bottom, 0th column == visual right-hand side
            flip(img, img, -1);//flip both horizontally and vertically
            break;
        case    IMAGE_ORIENTATION_BL: //0th row == visual bottom, 0th column == visual left-hand side
            flip(img, img, 0); //flip vertically
            break;
        case    IMAGE_ORIENTATION_LT: //0th row == visual left-hand side, 0th column == visual top
            transpose(img, img);
            break;
        case    IMAGE_ORIENTATION_RT: //0th row == visual right-hand side, 0th column == visual top
            transpose(img, img);
            flip(img, img, 1); //flip horizontally
            break;
        case    IMAGE_ORIENTATION_RB: //0th row == visual right-hand side, 0th column == visual bottom
            transpose(img, img);
            flip(img, img, -1); //flip both horizontally and vertically
            break;
        case    IMAGE_ORIENTATION_LB: //0th row == visual left-hand side, 0th column == visual bottom
            transpose(img, img);
            flip(img, img, 0); //flip vertically
            break;
        default:
            //by default the image read has normal (JPEG_ORIENTATION_TL) orientation
            break;
    }
}

static void ApplyExifOrientation(ExifEntry exifEntry, OutputArray img)
{
    int orientation = IMAGE_ORIENTATION_TL;

    if (exifEntry.tagId != ExifTagId::TAG_INVALID_TAG)
    {
        orientation = exifEntry.value.field_u16; //orientation is unsigned short, so check field_u16
        ExifTransform(orientation, img);
    }
}

static void readMetadata(ImageDecoder& decoder,
                         std::vector<int>* metadata_types,
                         OutputArrayOfArrays metadata)
{
    if (!metadata_types)
        return;
    int kind = metadata.kind();
    void* obj = metadata.getObj();
    std::vector<Mat>* matvector = nullptr;
    std::vector<std::vector<uchar> >* vecvector = nullptr;
    if (kind == _InputArray::STD_VECTOR_MAT) {
        matvector = (std::vector<Mat>*)obj;
    } else if (kind == _InputArray::STD_VECTOR_VECTOR) {
        int elemtype = metadata.type(0);
        CV_Assert(elemtype == CV_8UC1 || elemtype == CV_8SC1);
        vecvector = (std::vector<std::vector<uint8_t> >*)obj;
    } else {
        CV_Error(Error::StsBadArg,
                 "unsupported metadata type, should be a vector of matrices or vector of byte vectors");
    }
    std::vector<Mat> src_metadata;
    for (int m = (int)IMAGE_METADATA_EXIF; m <= (int)IMAGE_METADATA_MAX; m++) {
        Mat mm = decoder->getMetadata((ImageMetadataType)m);
        if (!mm.empty()) {
            CV_Assert(mm.isContinuous());
            CV_Assert(mm.elemSize() == 1u);
            metadata_types->push_back(m);
            src_metadata.push_back(mm);
        }
    }
    size_t nmetadata = metadata_types->size();
    if (matvector) {
        matvector->resize(nmetadata);
        for (size_t m = 0; m < nmetadata; m++)
            src_metadata[m].copyTo(matvector->at(m));
    } else {
        vecvector->resize(nmetadata);
        for (size_t m = 0; m < nmetadata; m++) {
            const Mat& mm = src_metadata[m];
            const uchar* data = (uchar*)mm.data;
            vecvector->at(m).assign(data, data + mm.total());
        }
    }
}

static const char* metadataTypeToString(ImageMetadataType type)
{
    return type == IMAGE_METADATA_EXIF ? "Exif" :
           type == IMAGE_METADATA_XMP ? "XMP" :
           type == IMAGE_METADATA_ICCP ? "ICC Profile" :
           type == IMAGE_METADATA_TEXT ? "Text" : "???";
}

static void addMetadata(ImageEncoder& encoder,
                        const std::vector<int>& metadata_types,
                        InputArrayOfArrays metadata)
{
    size_t nmetadata_chunks = metadata_types.size();
    for (size_t i = 0; i < nmetadata_chunks; i++) {
        ImageMetadataType metadata_type = (ImageMetadataType)metadata_types[i];
        bool ok = encoder->addMetadata(metadata_type, metadata.getMat((int)i));
        if (!ok) {
            std::string desc = encoder->getDescription();
            CV_LOG_WARNING(NULL, "Imgcodecs: metadata of type '"
                           << metadataTypeToString(metadata_type)
                           << "' is not supported when encoding '"
                           << desc << "'");
        }
    }
}

/**
 * Read an image into memory and return the information
 *
 * @param[in] filename File to load
 * @param[in] flags Flags
 * @param[in] mat Reference to C++ Mat object (If LOAD_MAT)
 *
*/
static bool
imread_( const String& filename, int flags, OutputArray mat,
         std::vector<int>* metadata_types, OutputArrayOfArrays metadata)
{
    /// Search for the relevant decoder to handle the imagery
    ImageDecoder decoder;

#ifdef HAVE_GDAL
    if(flags != IMREAD_UNCHANGED && (flags & IMREAD_LOAD_GDAL) == IMREAD_LOAD_GDAL ){
        decoder = GdalDecoder().newDecoder();
    }else{
#endif
        decoder = findDecoder( filename );
#ifdef HAVE_GDAL
    }
#endif

    /// if no decoder was found, return nothing.
    if( !decoder ){
        return 0;
    }

    int scale_denom = 1;
    if( flags > IMREAD_LOAD_GDAL )
    {
        if( flags & IMREAD_REDUCED_GRAYSCALE_2 )
            scale_denom = 2;
        else if( flags & IMREAD_REDUCED_GRAYSCALE_4 )
            scale_denom = 4;
        else if( flags & IMREAD_REDUCED_GRAYSCALE_8 )
            scale_denom = 8;
    }

    // Try to decode image by RGB instead of BGR.
    if (flags & IMREAD_COLOR_RGB && flags != IMREAD_UNCHANGED)
    {
        decoder->setRGB(true);
    }

    /// set the scale_denom in the driver
    decoder->setScale( scale_denom );

    /// set the filename in the driver
    decoder->setSource( filename );

    if (metadata_types)
    {
        metadata_types->clear();
        decoder->setMetadataReadingFlag(1);
    }

    try
    {
        // read the header to make sure it succeeds
        if( !decoder->readHeader() )
            return 0;
    }
    catch (const cv::Exception& e)
    {
        CV_LOG_ERROR(NULL, "imread_('" << filename << "'): can't read header: " << e.what());
        return 0;
    }
    catch (...)
    {
        CV_LOG_ERROR(NULL, "imread_('" << filename << "'): can't read header: unknown exception");
        return 0;
    }


    // established the required input image size
    Size size = validateInputImageSize(Size(decoder->width(), decoder->height()));

    // grab the decoded type
    const int type = calcType(decoder->type(), flags);

    if (mat.empty())
    {
        mat.create( size.height, size.width, type );
    }
    else
    {
        CV_CheckEQ(size, mat.size(), "");
        CV_CheckTypeEQ(type, mat.type(), "");
        CV_Assert(mat.isContinuous());
    }

    // read the image data
    Mat real_mat = mat.getMat();
    const void * original_ptr = real_mat.data;
    bool success = false;
    decoder->resetFrameCount(); // this is needed for PngDecoder. it should be called before decoder->readData()
    try
    {
        if (decoder->readData(real_mat))
        {
            CV_CheckTrue(original_ptr == real_mat.data, "Internal imread issue");
            success = true;
        }

        readMetadata(decoder, metadata_types, metadata);
    }
    catch (const cv::Exception& e)
    {
        CV_LOG_ERROR(NULL, "imread_('" << filename << "'): can't read data: " << e.what());
    }
    catch (...)
    {
        CV_LOG_ERROR(NULL, "imread_('" << filename << "'): can't read data: unknown exception");
    }
    if (!success)
    {
        mat.release();
        return false;
    }

    if( decoder->setScale( scale_denom ) > 1 ) // if decoder is JpegDecoder then decoder->setScale always returns 1
    {
        resize( mat, mat, Size( size.width / scale_denom, size.height / scale_denom ), 0, 0, INTER_LINEAR_EXACT);
    }

    /// optionally rotate the data if EXIF orientation flag says so
    if (!mat.empty() && (flags & IMREAD_IGNORE_ORIENTATION) == 0 && flags != IMREAD_UNCHANGED )
    {
        ApplyExifOrientation(decoder->getExifEntry(ExifTagId::TAG_ORIENTATION), mat);
    }

    return true;
}


static bool
imreadmulti_(const String& filename, int flags, std::vector<Mat>& mats, int start, int count)
{
    /// Search for the relevant decoder to handle the imagery
    ImageDecoder decoder;

    CV_CheckGE(start, 0, "Start index cannont be < 0");

#ifdef HAVE_GDAL
    if (flags != IMREAD_UNCHANGED && (flags & IMREAD_LOAD_GDAL) == IMREAD_LOAD_GDAL) {
        decoder = GdalDecoder().newDecoder();
    }
    else {
#endif
        decoder = findDecoder(filename);
#ifdef HAVE_GDAL
    }
#endif

    /// if no decoder was found, return nothing.
    if (!decoder) {
        return 0;
    }

    if (count < 0) {
        count = std::numeric_limits<int>::max();
    }

    if (flags & IMREAD_COLOR_RGB && flags != IMREAD_UNCHANGED)
        decoder->setRGB(true);

    /// set the filename in the driver
    decoder->setSource(filename);

    // read the header to make sure it succeeds
    try
    {
        // read the header to make sure it succeeds
        if (!decoder->readHeader())
            return 0;
    }
    catch (const cv::Exception& e)
    {
        CV_LOG_ERROR(NULL, "imreadmulti_('" << filename << "'): can't read header: " << e.what());
        return 0;
    }
    catch (...)
    {
        CV_LOG_ERROR(NULL, "imreadmulti_('" << filename << "'): can't read header: unknown exception");
        return 0;
    }

    int current = start;

    while (current > 0)
    {
        if (!decoder->nextPage())
        {
            return false;
        }
        --current;
    }

    while (current < count)
    {
        // grab the decoded type
        const int type = calcType(decoder->type(), flags);

        // established the required input image size
        Size size = validateInputImageSize(Size(decoder->width(), decoder->height()));

        // read the image data
        Mat mat(size.height, size.width, type);
        bool success = false;
        try
        {
            if (decoder->readData(mat))
                success = true;
        }
        catch (const cv::Exception& e)
        {
            CV_LOG_ERROR(NULL, "imreadmulti_('" << filename << "'): can't read data: " << e.what());
        }
        catch (...)
        {
            CV_LOG_ERROR(NULL, "imreadmulti_('" << filename << "'): can't read data: unknown exception");
        }
        if (!success)
            break;

        // optionally rotate the data if EXIF' orientation flag says so
        if ((flags & IMREAD_IGNORE_ORIENTATION) == 0 && flags != IMREAD_UNCHANGED)
        {
            ApplyExifOrientation(decoder->getExifEntry(ExifTagId::TAG_ORIENTATION), mat);
        }

        mats.push_back(mat);
        if (!decoder->nextPage())
        {
            break;
        }
        ++current;
    }

    return !mats.empty();
}

/**
 * Read an image
 *
 *  This function merely calls the actual implementation above and returns itself.
 *
 * @param[in] filename File to load
 * @param[in] flags Flags you wish to set.
*/
Mat imread( const String& filename, int flags )
{
    CV_TRACE_FUNCTION();

    /// create the basic container
    Mat img;

    /// load the data
    imread_( filename, flags, img, nullptr, noArray() );

    /// return a reference to the data
    return img;
}

Mat imreadWithMetadata( const String& filename,
                        std::vector<int>& metadata_types,
                        OutputArrayOfArrays metadata,
                        int flags )
{
    CV_TRACE_FUNCTION();

    /// create the basic container
    Mat img;

    /// load the data
    imread_( filename, flags, img, &metadata_types, metadata );

    /// return a reference to the data
    return img;
}

void imread( const String& filename, OutputArray dst, int flags )
{
    CV_TRACE_FUNCTION();

    /// load the data
    imread_(filename, flags, dst, nullptr, noArray());
}

/**
* Read a multi-page image
*
*  This function merely calls the actual implementation above and returns itself.
*
* @param[in] filename File to load
* @param[in] mats Reference to C++ vector<Mat> object to hold the images
* @param[in] flags Flags you wish to set.
*
*/
bool imreadmulti(const String& filename, std::vector<Mat>& mats, int flags)
{
    CV_TRACE_FUNCTION();

    return imreadmulti_(filename, flags, mats, 0, -1);
}


bool imreadmulti(const String& filename, std::vector<Mat>& mats, int start, int count, int flags)
{
    CV_TRACE_FUNCTION();

    return imreadmulti_(filename, flags, mats, start, count);
}

static bool
imreadanimation_(const String& filename, int flags, int start, int count, Animation& animation)
{
    bool success = false;
    if (start < 0) {
        start = 0;
    }
    if (count < 0) {
        count = INT16_MAX;
    }

    /// Search for the relevant decoder to handle the imagery
    ImageDecoder decoder;
    decoder = findDecoder(filename);

    /// if no decoder was found, return false.
    if (!decoder) {
        CV_LOG_WARNING(NULL, "Decoder for " << filename << " not found!\n");
        return false;
    }

    /// set the filename in the driver
    decoder->setSource(filename);
    // read the header to make sure it succeeds
    try
    {
        // read the header to make sure it succeeds
        if (!decoder->readHeader())
            return false;
    }
    catch (const cv::Exception& e)
    {
        CV_LOG_ERROR(NULL, "imreadanimation_('" << filename << "'): can't read header: " << e.what());
        return false;
    }
    catch (...)
    {
        CV_LOG_ERROR(NULL, "imreadanimation_('" << filename << "'): can't read header: unknown exception");
        return false;
    }

    int current = 0;
    int frame_count = (int)decoder->getFrameCount();
    count = count + start > frame_count ? frame_count - start : count;

    uint64 pixels = (uint64)decoder->width() * (uint64)decoder->height() * (uint64)(count + 4);
    if (pixels > CV_IO_MAX_IMAGE_PIXELS) {
        CV_LOG_WARNING(NULL, "\nyou are trying to read " << pixels <<
            " bytes that exceed CV_IO_MAX_IMAGE_PIXELS.\n");
        return false;
    }

    while (current < start + count)
    {
        // grab the decoded type
        const int type = calcType(decoder->type(), flags);

        // established the required input image size
        Size size = validateInputImageSize(Size(decoder->width(), decoder->height()));

        // read the image data
        Mat mat(size.height, size.width, type);
        success = false;
        try
        {
            if (decoder->readData(mat))
                success = true;
        }
        catch (const cv::Exception& e)
        {
            CV_LOG_ERROR(NULL, "imreadanimation_('" << filename << "'): can't read data: " << e.what());
        }
        catch (...)
        {
            CV_LOG_ERROR(NULL, "imreadanimation_('" << filename << "'): can't read data: unknown exception");
        }
        if (!success)
            break;

        // optionally rotate the data if EXIF' orientation flag says so
        if ((flags & IMREAD_IGNORE_ORIENTATION) == 0 && flags != IMREAD_UNCHANGED)
        {
            ApplyExifOrientation(decoder->getExifEntry(ExifTagId::TAG_ORIENTATION), mat);
        }

        if (current >= start)
        {
            int duration = decoder->animation().durations.size() > 0 ? decoder->animation().durations.back() : 1000;
            animation.durations.push_back(duration);
            animation.frames.push_back(mat);
        }

        if (!decoder->nextPage())
        {
            break;
        }
        ++current;
    }
    animation.bgcolor = decoder->animation().bgcolor;
    animation.loop_count = decoder->animation().loop_count;
    animation.still_image = decoder->animation().still_image;

    return success;
}

bool imreadanimation(const String& filename, CV_OUT Animation& animation, int start, int count)
{
    CV_TRACE_FUNCTION();

    return imreadanimation_(filename, IMREAD_UNCHANGED, start, count, animation);
}

static bool imdecodeanimation_(InputArray buf, int flags, int start, int count, Animation& animation)
{
    bool success = false;
    if (start < 0) {
        start = 0;
    }
    if (count < 0) {
        count = INT16_MAX;
    }

    /// Search for the relevant decoder to handle the imagery
    ImageDecoder decoder;
    decoder = findDecoder(buf.getMat());

    /// if no decoder was found, return false.
    if (!decoder) {
        CV_LOG_WARNING(NULL, "Decoder for buffer not found!\n");
        return false;
    }

    /// set the filename in the driver
    decoder->setSource(buf.getMat());
    // read the header to make sure it succeeds
    try
    {
        // read the header to make sure it succeeds
        if (!decoder->readHeader())
            return false;
    }
    catch (const cv::Exception& e)
    {
        CV_LOG_ERROR(NULL, "imdecodeanimation_(): can't read header: " << e.what());
        return false;
    }
    catch (...)
    {
        CV_LOG_ERROR(NULL, "imdecodeanimation_(): can't read header: unknown exception");
        return false;
    }

    int current = 0;
    int frame_count = (int)decoder->getFrameCount();
    count = count + start > frame_count ? frame_count - start : count;

    uint64 pixels = (uint64)decoder->width() * (uint64)decoder->height() * (uint64)(count + 4);
    if (pixels > CV_IO_MAX_IMAGE_PIXELS) {
        CV_LOG_WARNING(NULL, "\nyou are trying to read " << pixels <<
            " bytes that exceed CV_IO_MAX_IMAGE_PIXELS.\n");
        return false;
    }

    while (current < start + count)
    {
        // grab the decoded type
        const int type = calcType(decoder->type(), flags);

        // established the required input image size
        Size size = validateInputImageSize(Size(decoder->width(), decoder->height()));

        // read the image data
        Mat mat(size.height, size.width, type);
        success = false;
        try
        {
            if (decoder->readData(mat))
                success = true;
        }
        catch (const cv::Exception& e)
        {
            CV_LOG_ERROR(NULL, "imreadanimation_: can't read data: " << e.what());
        }
        catch (...)
        {
            CV_LOG_ERROR(NULL, "imreadanimation_: can't read data: unknown exception");
        }
        if (!success)
            break;

        // optionally rotate the data if EXIF' orientation flag says so
        if ((flags & IMREAD_IGNORE_ORIENTATION) == 0 && flags != IMREAD_UNCHANGED)
        {
            ApplyExifOrientation(decoder->getExifEntry(ExifTagId::TAG_ORIENTATION), mat);
        }

        if (current >= start)
        {
            int duration = decoder->animation().durations.size() > 0 ? decoder->animation().durations.back() : 1000;
            animation.durations.push_back(duration);
            animation.frames.push_back(mat);
        }

        if (!decoder->nextPage())
        {
            break;
        }
        ++current;
    }
    animation.bgcolor = decoder->animation().bgcolor;
    animation.loop_count = decoder->animation().loop_count;
    animation.still_image = decoder->animation().still_image;

    return success;
}

bool imdecodeanimation(InputArray buf, Animation& animation, int start, int count)
{
    CV_TRACE_FUNCTION();

    return imdecodeanimation_(buf, IMREAD_UNCHANGED, start, count, animation);
}

static
size_t imcount_(const String& filename, int flags)
{
    try{
        ImageCollection collection(filename, flags);
        return collection.size();
    } catch(cv::Exception const& e) {
        // Reading header or finding decoder for the filename is failed
        CV_LOG_ERROR(NULL, "imcount_('" << filename << "'): can't read header or can't find decoder: " << e.what());
    }
    return 0;
}

size_t imcount(const String& filename, int flags)
{
    CV_TRACE_FUNCTION();

    return imcount_(filename, flags);
}


static bool imwrite_( const String& filename, const std::vector<Mat>& img_vec,
                      const std::vector<int>& metadata_types,
                      InputArrayOfArrays metadata,
                      const std::vector<int>& params_, bool flipv )
{
    bool isMultiImg = img_vec.size() > 1;
    std::vector<Mat> write_vec;

    ImageEncoder encoder = findEncoder( filename );
    if( !encoder )
        CV_Error( Error::StsError, "could not find a writer for the specified extension" );

    for (size_t page = 0; page < img_vec.size(); page++)
    {
        Mat image = img_vec[page];
        CV_Assert(!image.empty());

        CV_Assert( image.channels() == 1 || image.channels() == 3 || image.channels() == 4 );

        Mat temp;
        if( !encoder->isFormatSupported(image.depth()) )
        {
            CV_LOG_ONCE_WARNING(NULL, "Unsupported depth image for selected encoder is fallbacked to CV_8U.");
            CV_Assert( encoder->isFormatSupported(CV_8U) );
            image.convertTo( temp, CV_8U );
            image = temp;
        }

        if( flipv )
        {
            flip(image, temp, 0);
            image = temp;
        }

        write_vec.push_back(image);
    }

    encoder->setDestination( filename );
    addMetadata(encoder, metadata_types, metadata);

#if CV_VERSION_MAJOR < 5 && defined(HAVE_IMGCODEC_HDR)
    bool fixed = false;
    std::vector<int> params_pair(2);
    if (dynamic_cast<HdrEncoder*>(encoder.get()))
    {
        if (params_.size() == 1)
        {
            CV_LOG_WARNING(NULL, "imwrite() accepts key-value pair of parameters, but single value is passed. "
                                 "HDR encoder behavior has been changed, please use IMWRITE_HDR_COMPRESSION key.");
            params_pair[0] = IMWRITE_HDR_COMPRESSION;
            params_pair[1] = params_[0];
            fixed = true;
        }
    }
    const std::vector<int>& params = fixed ? params_pair : params_;
#else
    const std::vector<int>& params = params_;
#endif

    CV_Check(params.size(), (params.size() & 1) == 0, "Encoding 'params' must be key-value pairs");
    CV_CheckLE(params.size(), (size_t)(CV_IO_MAX_IMAGE_PARAMS*2), "");
    bool code = false;
    try
    {
        if (!isMultiImg)
            code = encoder->write( write_vec[0], params );
        else
            code = encoder->writemulti( write_vec, params ); //to be implemented

        if (!code)
        {
            FILE* f = fopen( filename.c_str(), "wb" );
            if ( !f )
            {
                if (errno == EACCES)
                {
                    CV_LOG_WARNING(NULL, "imwrite_('" << filename << "'): can't open file for writing: permission denied");
                }
            }
            else
            {
                fclose(f);
                remove(filename.c_str());
            }
        }
    }
    catch (const cv::Exception& e)
    {
        CV_LOG_ERROR(NULL, "imwrite_('" << filename << "'): can't write data: " << e.what());
        code = false;
    }
    catch (...)
    {
        CV_LOG_ERROR(NULL, "imwrite_('" << filename << "'): can't write data: unknown exception");
        code = false;
    }

    return code;
}

bool imwrite( const String& filename, InputArray _img,
              const std::vector<int>& params )
{
    CV_TRACE_FUNCTION();

    CV_Assert(!_img.empty());

    std::vector<Mat> img_vec;
    if (_img.isMatVector() || _img.isUMatVector())
        _img.getMatVector(img_vec);
    else
        img_vec.push_back(_img.getMat());

    CV_Assert(!img_vec.empty());
    return imwrite_(filename, img_vec, {}, noArray(), params, false);
}

bool imwriteWithMetadata( const String& filename, InputArray _img,
                          const std::vector<int>& metadata_types,
                          InputArrayOfArrays metadata,
                          const std::vector<int>& params )
{
    CV_TRACE_FUNCTION();

    CV_Assert(!_img.empty());

    std::vector<Mat> img_vec;
    if (_img.isMatVector() || _img.isUMatVector())
        _img.getMatVector(img_vec);
    else
        img_vec.push_back(_img.getMat());

    CV_Assert(!img_vec.empty());
    return imwrite_(filename, img_vec, metadata_types, metadata, params, false);
}

static bool imwriteanimation_(const String& filename, const Animation& animation, const std::vector<int>& params)
{
    ImageEncoder encoder = findEncoder(filename);
    if (!encoder)
        CV_Error(Error::StsError, "could not find a writer for the specified extension");

    encoder->setDestination(filename);

    bool code = false;
    try
    {
        code = encoder->writeanimation(animation, params);

        if (!code)
        {
            FILE* f = fopen(filename.c_str(), "wb");
            if (!f)
            {
                if (errno == EACCES)
                {
                    CV_LOG_ERROR(NULL, "imwriteanimation_('" << filename << "'): can't open file for writing: permission denied");
                }
            }
            else
            {
                fclose(f);
                remove(filename.c_str());
            }
        }
    }
    catch (const cv::Exception& e)
    {
        CV_LOG_ERROR(NULL, "imwriteanimation_('" << filename << "'): can't write data: " << e.what());
    }
    catch (...)
    {
        CV_LOG_ERROR(NULL, "imwriteanimation_('" << filename << "'): can't write data: unknown exception");
    }

    return code;
}

bool imwriteanimation(const String& filename, const Animation& animation, const std::vector<int>& params)
{
    CV_Assert(!animation.frames.empty());
    CV_Assert(animation.frames.size() == animation.durations.size());
    return imwriteanimation_(filename, animation, params);
}

static bool imencodeanimation_(const String& ext, const Animation& animation, std::vector<uchar>& buf, const std::vector<int>& params)
{
    ImageEncoder encoder = findEncoder(ext);
    if (!encoder)
        CV_Error(Error::StsError, "could not find a writer for the specified extension");

    encoder->setDestination(buf);

    bool code = false;
    try
    {
        code = encoder->writeanimation(animation, params);
    }
    catch (const cv::Exception& e)
    {
        CV_LOG_ERROR(NULL, "imencodeanimation_('" << ext << "'): can't write data: " << e.what());
    }
    catch (...)
    {
        CV_LOG_ERROR(NULL, "imencodeanimation_('" << ext << "'): can't write data: unknown exception");
    }

    return code;
}

bool imencodeanimation(const String& ext, const Animation& animation, std::vector<uchar>& buf, const std::vector<int>& params)
{
    CV_Assert(!animation.frames.empty());
    CV_Assert(animation.frames.size() == animation.durations.size());
    return imencodeanimation_(ext, animation, buf, params);
}

static bool
imdecode_( const Mat& buf, int flags, Mat& mat,
           std::vector<int>* metadata_types,
           OutputArrayOfArrays metadata )
{
    CV_Assert(!buf.empty());
    CV_Assert(buf.isContinuous());
    CV_Assert(buf.checkVector(1, CV_8U) > 0);
    Mat buf_row = buf.reshape(1, 1);  // decoders expects single row, avoid issues with vector columns

    String filename;

    ImageDecoder decoder = findDecoder(buf_row);
    if( !decoder )
        return false;

    int scale_denom = 1;
    if( flags > IMREAD_LOAD_GDAL )
    {
        if( flags & IMREAD_REDUCED_GRAYSCALE_2 )
            scale_denom = 2;
        else if( flags & IMREAD_REDUCED_GRAYSCALE_4 )
            scale_denom = 4;
        else if( flags & IMREAD_REDUCED_GRAYSCALE_8 )
            scale_denom = 8;
    }

    // Try to decode image by RGB instead of BGR.
    if (flags & IMREAD_COLOR_RGB && flags != IMREAD_UNCHANGED)
    {
        decoder->setRGB(true);
    }

    /// set the scale_denom in the driver
    decoder->setScale( scale_denom );

    if( !decoder->setSource(buf_row) )
    {
        filename = tempfile();
        FILE* f = fopen( filename.c_str(), "wb" );
        if( !f )
            return false;
        size_t bufSize = buf_row.total()*buf.elemSize();
        if (fwrite(buf_row.ptr(), 1, bufSize, f) != bufSize)
        {
            fclose( f );
            CV_Error( Error::StsError, "failed to write image data to temporary file" );
        }
        if( fclose(f) != 0 )
        {
            CV_Error( Error::StsError, "failed to write image data to temporary file" );
        }
        decoder->setSource(filename);
    }

    if (metadata_types)
    {
        metadata_types->clear();
        decoder->setMetadataReadingFlag(1);
    }

    bool success = false;
    try
    {
        if (decoder->readHeader())
            success = true;
    }
    catch (const cv::Exception& e)
    {
        CV_LOG_ERROR(NULL, "imdecode_('" << filename << "'): can't read header: " << e.what());
    }
    catch (...)
    {
        CV_LOG_ERROR(NULL, "imdecode_('" << filename << "'): can't read header: unknown exception");
    }
    if (!success)
    {
        decoder.release();
        if (!filename.empty())
        {
            if (0 != remove(filename.c_str()))
            {
                CV_LOG_WARNING(NULL, "unable to remove temporary file:" << filename);
            }
        }
        return false;
    }

    // established the required input image size
    Size size = validateInputImageSize(Size(decoder->width(), decoder->height()));

    const int type = calcType(decoder->type(), flags);

    mat.create( size.height, size.width, type );

    success = false;
    try
    {
        if (decoder->readData(mat))
            success = true;
        readMetadata(decoder, metadata_types, metadata);
    }
    catch (const cv::Exception& e)
    {
        CV_LOG_ERROR(NULL, "imdecode_('" << filename << "'): can't read data: " << e.what());
    }
    catch (...)
    {
        CV_LOG_ERROR(NULL, "imdecode_('" << filename << "'): can't read data: unknown exception");
    }

    if (!filename.empty())
    {
        if (0 != remove(filename.c_str()))
        {
            CV_LOG_WARNING(NULL, "unable to remove temporary file: " << filename);
        }
    }

    if (!success)
    {
        return false;
    }

    if( decoder->setScale( scale_denom ) > 1 ) // if decoder is JpegDecoder then decoder->setScale always returns 1
    {
        resize(mat, mat, Size( size.width / scale_denom, size.height / scale_denom ), 0, 0, INTER_LINEAR_EXACT);
    }

    /// optionally rotate the data if EXIF' orientation flag says so
    if (!mat.empty() && (flags & IMREAD_IGNORE_ORIENTATION) == 0 && flags != IMREAD_UNCHANGED)
    {
        ApplyExifOrientation(decoder->getExifEntry(ExifTagId::TAG_ORIENTATION), mat);
    }

    return true;
}


Mat imdecode( InputArray _buf, int flags )
{
    CV_TRACE_FUNCTION();

    Mat buf = _buf.getMat(), img;
    if (!imdecode_(buf, flags, img, nullptr, noArray()))
        img.release();

    return img;
}

Mat imdecode( InputArray _buf, int flags, Mat* dst )
{
    CV_TRACE_FUNCTION();

    Mat buf = _buf.getMat(), img;
    dst = dst ? dst : &img;
    if (imdecode_(buf, flags, *dst, nullptr, noArray()))
        return *dst;
    else
        return cv::Mat();
}

Mat imdecodeWithMetadata( InputArray _buf, std::vector<int>& metadata_types,
                          OutputArrayOfArrays metadata, int flags )
{
    CV_TRACE_FUNCTION();

    Mat buf = _buf.getMat(), img;
    if (!imdecode_(buf, flags, img, &metadata_types, metadata))
        img.release();

    return img;
}

static bool
imdecodemulti_(const Mat& buf, int flags, std::vector<Mat>& mats, int start, int count)
{
    CV_Assert(!buf.empty());
    CV_Assert(buf.isContinuous());
    CV_Assert(buf.checkVector(1, CV_8U) > 0);
    Mat buf_row = buf.reshape(1, 1);  // decoders expects single row, avoid issues with vector columns

    String filename;

    ImageDecoder decoder = findDecoder(buf_row);
    if (!decoder)
        return false;

    // Try to decode image by RGB instead of BGR.
    if (flags & IMREAD_COLOR_RGB && flags != IMREAD_UNCHANGED)
    {
        decoder->setRGB(true);
    }

    if (count < 0) {
        count = std::numeric_limits<int>::max();
    }

    if (!decoder->setSource(buf_row))
    {
        filename = tempfile();
        FILE* f = fopen(filename.c_str(), "wb");
        if (!f)
            return false;
        size_t bufSize = buf_row.total() * buf.elemSize();
        if (fwrite(buf_row.ptr(), 1, bufSize, f) != bufSize)
        {
            fclose(f);
            CV_Error(Error::StsError, "failed to write image data to temporary file");
        }
        if (fclose(f) != 0)
        {
            CV_Error(Error::StsError, "failed to write image data to temporary file");
        }
        decoder->setSource(filename);
    }

    // read the header to make sure it succeeds
    bool success = false;
    try
    {
        // read the header to make sure it succeeds
        if (decoder->readHeader())
            success = true;
    }
    catch (const cv::Exception& e)
    {
        CV_LOG_ERROR(NULL, "imreadmulti_('" << filename << "'): can't read header: " << e.what());
    }
    catch (...)
    {
        CV_LOG_ERROR(NULL, "imreadmulti_('" << filename << "'): can't read header: unknown exception");
    }

    int current = start;
    while (success && current > 0)
    {
        if (!decoder->nextPage())
        {
            success = false;
            break;
        }
        --current;
    }

    if (!success)
    {
        decoder.release();
        if (!filename.empty())
        {
            if (0 != remove(filename.c_str()))
            {
                CV_LOG_WARNING(NULL, "unable to remove temporary file: " << filename);
            }
        }
        return 0;
    }

    while (current < count)
    {
        // grab the decoded type
        const int type = calcType(decoder->type(), flags);

        // established the required input image size
        Size size = validateInputImageSize(Size(decoder->width(), decoder->height()));

        // read the image data
        Mat mat(size.height, size.width, type);
        success = false;
        try
        {
            if (decoder->readData(mat))
                success = true;
        }
        catch (const cv::Exception& e)
        {
            CV_LOG_ERROR(NULL, "imreadmulti_('" << filename << "'): can't read data: " << e.what());
        }
        catch (...)
        {
            CV_LOG_ERROR(NULL, "imreadmulti_('" << filename << "'): can't read data: unknown exception");
        }
        if (!success)
            break;

        // optionally rotate the data if EXIF' orientation flag says so
        if ((flags & IMREAD_IGNORE_ORIENTATION) == 0 && flags != IMREAD_UNCHANGED)
        {
            ApplyExifOrientation(decoder->getExifEntry(ExifTagId::TAG_ORIENTATION), mat);
        }

        mats.push_back(mat);
        if (!decoder->nextPage())
        {
            break;
        }
        ++current;
    }

    if (!filename.empty())
    {
        if (0 != remove(filename.c_str()))
        {
            CV_LOG_WARNING(NULL, "unable to remove temporary file: " << filename);
        }
    }

    if (!success)
        mats.clear();
    return !mats.empty();
}

bool imdecodemulti(InputArray _buf, int flags, CV_OUT std::vector<Mat>& mats, const Range& range)
{
    CV_TRACE_FUNCTION();

    Mat buf = _buf.getMat();
    if (range == Range::all())
    {
        return imdecodemulti_(buf, flags, mats, 0, -1);
    }
    else
    {
        CV_CheckGE(range.start, 0, "Range start cannot be negative.");
        CV_CheckGT(range.size(), 0, "Range cannot be empty.");
        return imdecodemulti_(buf, flags, mats, range.start, range.size());
    }
}

bool imencodeWithMetadata( const String& ext, InputArray _img,
                           const std::vector<int>& metadata_types,
                           InputArrayOfArrays metadata,
                           std::vector<uchar>& buf, const std::vector<int>& params_ )
{
    CV_TRACE_FUNCTION();

    ImageEncoder encoder = findEncoder( ext );
    if( !encoder )
        CV_Error( Error::StsError, "could not find encoder for the specified extension" );

    std::vector<Mat> img_vec;
    CV_Assert(!_img.empty());
    if (_img.isMatVector() || _img.isUMatVector())
        _img.getMatVector(img_vec);
    else
        img_vec.push_back(_img.getMat());

    CV_Assert(!img_vec.empty());
    const bool isMultiImg = img_vec.size() > 1;

    std::vector<Mat> write_vec;
    for (size_t page = 0; page < img_vec.size(); page++)
    {
        Mat image = img_vec[page];
        CV_Assert(!image.empty());

        const int channels = image.channels();
        CV_Assert( channels == 1 || channels == 3 || channels == 4 );

        Mat temp;
        if( !encoder->isFormatSupported(image.depth()) )
        {
            CV_LOG_ONCE_WARNING(NULL, "Unsupported depth image for selected encoder is fallbacked to CV_8U.");
            CV_Assert( encoder->isFormatSupported(CV_8U) );
            image.convertTo( temp, CV_8U );
            image = temp;
        }

        write_vec.push_back(image);
    }

#if CV_VERSION_MAJOR < 5 && defined(HAVE_IMGCODEC_HDR)
    bool fixed = false;
    std::vector<int> params_pair(2);
    if (dynamic_cast<HdrEncoder*>(encoder.get()))
    {
        if (params_.size() == 1)
        {
            CV_LOG_WARNING(NULL, "imwrite() accepts key-value pair of parameters, but single value is passed. "
                                 "HDR encoder behavior has been changed, please use IMWRITE_HDR_COMPRESSION key.");
            params_pair[0] = IMWRITE_HDR_COMPRESSION;
            params_pair[1] = params_[0];
            fixed = true;
        }
    }
    const std::vector<int>& params = fixed ? params_pair : params_;
#else
    const std::vector<int>& params = params_;
#endif

    CV_Check(params.size(), (params.size() & 1) == 0, "Encoding 'params' must be key-value pairs");
    CV_CheckLE(params.size(), (size_t)(CV_IO_MAX_IMAGE_PARAMS*2), "");

    bool code = false;
    String filename;
    if( !encoder->setDestination(buf) )
    {
        filename = tempfile();
        code = encoder->setDestination(filename);
        CV_Assert( code );
    }
    addMetadata(encoder, metadata_types, metadata);

    try {
        if (!isMultiImg)
            code = encoder->write(write_vec[0], params);
        else
            code = encoder->writemulti(write_vec, params);

        encoder->throwOnError();
        CV_Assert( code );
    }
    catch (const cv::Exception& e)
    {
        CV_LOG_ERROR(NULL, "imencode(): can't encode data: " << e.what());
        code = false;
    }
    catch (...)
    {
        CV_LOG_ERROR(NULL, "imencode(): can't encode data: unknown exception");
        code = false;
    }

    if( !filename.empty() && code )
    {
        FILE* f = fopen( filename.c_str(), "rb" );
        CV_Assert(f != 0);
        fseek( f, 0, SEEK_END );
        long pos = ftell(f);
        buf.resize((size_t)pos);
        fseek( f, 0, SEEK_SET );
        buf.resize(fread( &buf[0], 1, buf.size(), f ));
        fclose(f);
        remove(filename.c_str());
    }
    return code;
}

bool imencode( const String& ext, InputArray img,
               std::vector<uchar>& buf, const std::vector<int>& params_ )
{
    return imencodeWithMetadata(ext, img, {}, noArray(), buf, params_);
}

bool imencodemulti( const String& ext, InputArrayOfArrays imgs,
                    std::vector<uchar>& buf, const std::vector<int>& params)
{
    return imencode(ext, imgs, buf, params);
}

bool haveImageReader( const String& filename )
{
    ImageDecoder decoder = cv::findDecoder(filename);
    return !decoder.empty();
}

bool haveImageWriter( const String& filename )
{
    cv::ImageEncoder encoder = cv::findEncoder(filename);
    return !encoder.empty();
}

class ImageCollection::Impl {
public:
    Impl() = default;
    Impl(const std::string&  filename, int flags);
    void init(String const& filename, int flags);
    size_t size() const;
    Mat& at(int index);
    Mat& operator[](int index);
    void releaseCache(int index);
    ImageCollection::iterator begin(ImageCollection* ptr);
    ImageCollection::iterator end(ImageCollection* ptr);
    Mat read();
    int width() const;
    int height() const;
    bool readHeader();
    Mat readData();
    bool advance();
    int currentIndex() const;
    void reset();

private:
    String m_filename;
    int m_flags{};
    std::size_t m_size{};
    int m_width{};
    int m_height{};
    int m_current{};
    std::vector<cv::Mat> m_pages;
    ImageDecoder m_decoder;
};

ImageCollection::Impl::Impl(std::string const& filename, int flags) {
    this->init(filename, flags);
}

void ImageCollection::Impl::init(String const& filename, int flags) {
    m_filename = filename;
    m_flags = flags;

#ifdef HAVE_GDAL
    if (m_flags != IMREAD_UNCHANGED && (m_flags & IMREAD_LOAD_GDAL) == IMREAD_LOAD_GDAL) {
        m_decoder = GdalDecoder().newDecoder();
    }
    else {
#endif
    m_decoder = findDecoder(filename);
#ifdef HAVE_GDAL
    }
#endif


    CV_Assert(m_decoder);
    m_decoder->setSource(filename);
    CV_Assert(m_decoder->readHeader());

    m_size = m_decoder->getFrameCount();
    m_pages.resize(m_size);
}

size_t ImageCollection::Impl::size() const { return m_size; }

Mat ImageCollection::Impl::read() {
    auto result = this->readHeader();
    if(!result) {
        return {};
    }
    return this->readData();
}

int ImageCollection::Impl::width() const {
    return m_width;
}

int ImageCollection::Impl::height() const {
    return m_height;
}

bool ImageCollection::Impl::readHeader() {
    bool status = m_decoder->readHeader();
    m_width = m_decoder->width();
    m_height = m_decoder->height();
    return status;
}

// readHeader must be called before calling this method
Mat ImageCollection::Impl::readData() {
    const int type = calcType(m_decoder->type(), m_flags);

    // established the required input image size
    Size size = validateInputImageSize(Size(m_width, m_height));

    Mat mat(size.height, size.width, type);
    bool success = false;
    try {
        if (m_decoder->readData(mat))
            success = true;
    }
    catch (const cv::Exception &e) {
        CV_LOG_ERROR(NULL, "ImageCollection class: can't read data: " << e.what());
    }
    catch (...) {
        CV_LOG_ERROR(NULL, "ImageCollection class:: can't read data: unknown exception");
    }
    if (!success)
        return cv::Mat();

    if ((m_flags & IMREAD_IGNORE_ORIENTATION) == 0 && m_flags != IMREAD_UNCHANGED) {
        ApplyExifOrientation(m_decoder->getExifEntry(ExifTagId::TAG_ORIENTATION), mat);
    }

    return mat;
}

bool ImageCollection::Impl::advance() {  ++m_current; return m_decoder->nextPage(); }

int ImageCollection::Impl::currentIndex() const { return m_current; }

ImageCollection::iterator ImageCollection::Impl::begin(ImageCollection* ptr) { return ImageCollection::iterator(ptr); }

ImageCollection::iterator ImageCollection::Impl::end(ImageCollection* ptr) { return ImageCollection::iterator(ptr, static_cast<int>(this->size())); }

void ImageCollection::Impl::reset() {
    m_current = 0;
#ifdef HAVE_GDAL
    if (m_flags != IMREAD_UNCHANGED && (m_flags & IMREAD_LOAD_GDAL) == IMREAD_LOAD_GDAL) {
        m_decoder = GdalDecoder().newDecoder();
    }
    else {
#endif
    m_decoder = findDecoder(m_filename);
#ifdef HAVE_GDAL
    }
#endif

    m_decoder->setSource(m_filename);
    m_decoder->readHeader();
}

Mat& ImageCollection::Impl::at(int index) {
    CV_Assert(index >= 0 && size_t(index) < m_size);
    return operator[](index);
}

Mat& ImageCollection::Impl::operator[](int index) {
    if(m_pages.at(index).empty()) {
        // We can't go backward in multi images. If the page is not in vector yet,
        // go back to first page and advance until the desired page and read it into memory
        if(m_current != index) {
            reset();
            for(int i = 0; i != index && advance(); ++i) {}
        }
        m_pages[index] = read();
    }
    return m_pages[index];
}

void ImageCollection::Impl::releaseCache(int index) {
    CV_Assert(index >= 0 && size_t(index) < m_size);
    m_pages[index].release();
}

/* ImageCollection API*/

ImageCollection::ImageCollection() : pImpl(new Impl()) {}

ImageCollection::ImageCollection(const std::string& filename, int flags) : pImpl(new Impl(filename, flags)) {}

void ImageCollection::init(const String& img, int flags) { pImpl->init(img, flags); }

size_t ImageCollection::size() const { return pImpl->size(); }

const Mat& ImageCollection::at(int index) { return pImpl->at(index); }

const Mat& ImageCollection::operator[](int index) { return pImpl->operator[](index); }

void ImageCollection::releaseCache(int index) { pImpl->releaseCache(index); }

Ptr<ImageCollection::Impl> ImageCollection::getImpl() { return pImpl; }

/* Iterator API */

ImageCollection::iterator ImageCollection::begin() { return pImpl->begin(this); }

ImageCollection::iterator ImageCollection::end() { return pImpl->end(this); }

ImageCollection::iterator::iterator(ImageCollection* col) : m_pCollection(col), m_curr(0) {}

ImageCollection::iterator::iterator(ImageCollection* col, int end) : m_pCollection(col), m_curr(end) {}

Mat& ImageCollection::iterator::operator*() {
    CV_Assert(m_pCollection);
    return m_pCollection->getImpl()->operator[](m_curr);
}

Mat* ImageCollection::iterator::operator->() {
    CV_Assert(m_pCollection);
    return &m_pCollection->getImpl()->operator[](m_curr);
}

ImageCollection::iterator& ImageCollection::iterator::operator++() {
    if(m_pCollection->pImpl->currentIndex() == m_curr) {
        m_pCollection->pImpl->advance();
    }
    m_curr++;
    return *this;
}

ImageCollection::iterator ImageCollection::iterator::operator++(int) {
    iterator tmp = *this;
    ++(*this);
    return tmp;
}

Animation::Animation(int loopCount, Scalar bgColor)
    : loop_count(loopCount), bgcolor(bgColor)
{
    if (loopCount < 0 || loopCount > 0xffff)
        this->loop_count = 0; // loop_count should be non-negative
}

typedef std::unordered_map<int, int> intmap_t;

std::string tagTypeToString(ExifTagType type)
{
    const char* typestr =
        type == TAG_TYPE_NOTYPE ? "NoType" :
        type == TAG_TYPE_BYTE ? "Byte" :
        type == TAG_TYPE_ASCII ? "ASCII" :
        type == TAG_TYPE_SHORT ? "Short" :
        type == TAG_TYPE_LONG ? "Long" :
        type == TAG_TYPE_RATIONAL ? "Rational" :
        type == TAG_TYPE_SBYTE ? "SByte" :
        type == TAG_TYPE_UNDEFINED ? "Undefined" :
        type == TAG_TYPE_SSHORT ? "SShort" :
        type == TAG_TYPE_SLONG ? "SLong" :
        type == TAG_TYPE_SRATIONAL ? "SRational" :
        type == TAG_TYPE_FLOAT ? "Float" :
        type == TAG_TYPE_DOUBLE ? "Double" :
        type == TAG_TYPE_IFD ? "IFD" :
        type == TAG_TYPE_LONG8 ? "Long8" :
        type == TAG_TYPE_SLONG8 ? "SLong8" :
        type == TAG_TYPE_IFD8 ? "IFD8" : nullptr;
    return typestr ? std::string(typestr) : cv::format("Unkhown type <%d>", (int)type);
}

size_t tagTypeSize(ExifTagType type)
{
    return
        type == TAG_TYPE_NOTYPE ? 0 :
        type == TAG_TYPE_BYTE ? 1 :
        type == TAG_TYPE_ASCII ? 1 :
        type == TAG_TYPE_SHORT ? 2 :
        type == TAG_TYPE_LONG ? 4 :
        type == TAG_TYPE_RATIONAL ? 8 :
        type == TAG_TYPE_SBYTE ? 1 :
        type == TAG_TYPE_UNDEFINED ? 1 :
        type == TAG_TYPE_SSHORT ? 2 :
        type == TAG_TYPE_SLONG ? 4 :
        type == TAG_TYPE_SRATIONAL ? 8 :
        type == TAG_TYPE_FLOAT ? 4 :
        type == TAG_TYPE_DOUBLE ? 8 :
        type == TAG_TYPE_IFD ? 0 :
        type == TAG_TYPE_LONG8 ? 8 :
        type == TAG_TYPE_SLONG8 ? 8 :
        type == TAG_TYPE_IFD8 ? 0 : 0;
}

std::string exifTagIdToString(ExifTagId tag)
{
    const char* tagstr =
        tag == TAG_EMPTY ? "<empty>" :
        tag == TAG_SUB_FILETYPE ? "SubFileType" :
        tag == TAG_IMAGE_WIDTH ? "ImageWidth" :
        tag == TAG_IMAGE_LENGTH ? "ImageLength" :
        tag == TAG_BITS_PER_SAMPLE ? "BitsPerSample" :
        tag == TAG_COMPRESSION ? "Compression" :
        tag == TAG_PHOTOMETRIC ? "Photometric" :
        tag == TAG_IMAGEDESCRIPTION ? "ImageDescription" :
        tag == TAG_MAKE ? "Make" :
        tag == TAG_MODEL ? "Model" :
        tag == TAG_STRIP_OFFSET ? "StripOffset" :
        tag == TAG_SAMPLES_PER_PIXEL ? "SamplesPerPixel" :
        tag == TAG_ROWS_PER_STRIP ? "RowsPerStrip" :
        tag == TAG_STRIP_BYTE_COUNTS ? "StripByteCounts" :
        tag == TAG_PLANAR_CONFIG ? "PlanarConfig" :
        tag == TAG_ORIENTATION ? "Orientation" :
        tag == TAG_XRESOLUTION ? "XResolution" :
        tag == TAG_YRESOLUTION ? "YResolution" :
        tag == TAG_RESOLUTION_UNIT ? "ResolutionUnit" :
        tag == TAG_SOFTWARE ? "Software" :
        tag == TAG_MODIFYDATE ? "ModifyDate" :
        tag == TAG_SAMPLEFORMAT ? "SampleFormat" :
        tag == TAG_CFA_REPEAT_PATTERN_DIM ? "CFARepeatPatternDim" :
        tag == TAG_CFA_PATTERN ? "CFAPattern" :

        tag == TAG_COPYRIGHT ? "Copyright" :
        tag == TAG_EXPOSURE_TIME ? "ExposureTime" :
        tag == TAG_FNUMBER ? "FNumber" :

        tag == TAG_EXIF_TAGS ? "ExifTags" :
        tag == TAG_ISOSPEED ? "ISOSpeed" :
        tag == TAG_DATETIME_CREATE ? "CreateDate" :
        tag == TAG_DATETIME_ORIGINAL ? "DateTimeOriginal" :

        tag == TAG_FLASH ? "Flash" :
        tag == TAG_FOCALLENGTH ? "FocalLength" :
        tag == TAG_EP_STANDARD_ID ? "TIFF/EPStandardID" :

        tag == TAG_SHUTTER_SPEED ? "Shutter Speed" :
        tag == TAG_APERTURE_VALUE ? "Aperture Value" :

        tag == TAG_SUBSECTIME ? "SubSec Time" :
        tag == TAG_SUBSECTIME_ORIGINAL ? "SubSec Original Time" :
        tag == TAG_SUBSECTIME_DIGITIZED ? "SubSec Digitized Time" :

        tag == TAG_EXIF_IMAGE_WIDTH ? "Exif Image Width" :
        tag == TAG_EXIF_IMAGE_HEIGHT ? "Exif Image Height" :
        tag == TAG_WHITE_BALANCE ? "White Balance" :

        tag == TAG_EXIF_VERSION ? "Exif Version" :

        tag == TAG_DNG_VERSION ? "DNGVersion" :
        tag == TAG_DNG_BACKWARD_VERSION ? "DNGBackwardVersion" :
        tag == TAG_UNIQUE_CAMERA_MODEL ? "UniqueCameraModel" :
        tag == TAG_CHROMA_BLUR_RADIUS ? "ChromaBlurRadius" :
        tag == TAG_CFA_PLANECOLOR ? "CFAPlaneColor" :
        tag == TAG_CFA_LAYOUT ? "CFALayout" :
        tag == TAG_BLACK_LEVEL_REPEAT_DIM ? "BlackLevelRepeatDim" :
        tag == TAG_BLACK_LEVEL ? "BlackLevel" :
        tag == TAG_WHITE_LEVEL ? "WhiteLevel" :
        tag == TAG_DEFAULT_SCALE ? "DefaultScale" :
        tag == TAG_DEFAULT_CROP_ORIGIN ? "DefaultCropOrigin" :
        tag == TAG_DEFAULT_CROP_SIZE ? "DefaultCropSize" :
        tag == TAG_COLOR_MATRIX1 ? "ColorMatrix1" :
        tag == TAG_COLOR_MATRIX2 ? "ColorMatrix2" :
        tag == TAG_CAMERA_CALIBRATION1 ? "CameraCalibration1" :
        tag == TAG_CAMERA_CALIBRATION2 ? "CameraCalibration2" :
        tag == TAG_ANALOG_BALANCE ? "AnalogBalance" :
        tag == TAG_AS_SHOT_NEUTRAL ? "AsShotNeutral" :
        tag == TAG_AS_SHOT_WHITE_XY ? "AsShotWhiteXY" :
        tag == TAG_BASELINE_EXPOSURE ? "BaselineExposure" :
        tag == TAG_CALIBRATION_ILLUMINANT1 ? "CalibrationIlluminant1" :
        tag == TAG_CALIBRATION_ILLUMINANT2 ? "CalibrationIlluminant2" :
        tag == TAG_EXTRA_CAMERA_PROFILES ? "ExtraCameraProfiles" :
        tag == TAG_PROFILE_NAME ? "ProfileName" :
        tag == TAG_AS_SHOT_PROFILE_NAME ? "AsShotProfileName" :
        tag == TAG_PREVIEW_COLORSPACE ? "PreviewColorspace" :
        tag == TAG_OPCODE_LIST2 ? "OpCodeList2" :
        tag == TAG_NOISE_PROFILE ? "NoiseProfile" :
        tag == TAG_DEFAULT_BLACK_RENDER ? "BlackRender" :
        tag == TAG_ACTIVE_AREA ? "ActiveArea" :
        tag == TAG_FORWARD_MATRIX1 ? "ForwardMatrix1" :
        tag == TAG_FORWARD_MATRIX2 ? "ForwardMatrix2" : nullptr;
    return tagstr ? std::string(tagstr) : cv::format("<unknown tag>(%d)", (int)tag);
};

template<typename _Tp> void dumpScalar(std::ostream& strm, _Tp v)
{
    strm << v;
}

template<> void dumpScalar(std::ostream& strm, int64_t v)
{
    strm << v;
}

template<> void dumpScalar(std::ostream& strm, double v)
{
    strm << cv::format("%.8g", v);
}

template<> void dumpScalar(std::ostream& strm, srational64_t v)
{
    strm << cv::format("%.4f", (double)v.num / v.denom);
}

template <typename _Tp> void dumpVector(std::ostream& strm, const std::vector<_Tp>& v)
{
    size_t i, nvalues = v.size();
    strm << '[';
    for (i = 0; i < nvalues; i++) {
        if (i > 0)
            strm << ", ";
        if (i >= 3 && i + 6 < nvalues) {
            strm << "... ";
            i = nvalues - 3;
        }
        dumpScalar(strm, v[i]);
    }
    strm << ']';
}

std::ostream& ExifEntry::dump(std::ostream& strm) const
{
    if (empty()) {
        strm << "<empty>";
        return strm;
    }
    strm << tagId << ": ";
    return strm;
}

static srational64_t doubleToRational(double v, int maxbits = 31)
{
    srational64_t r = { 1, 0 };
    if (std::isfinite(v)) {
        int e = 0;
        //double m = frexp(v, &e);
        if (e >= maxbits)
            return r;

        double iv = round(v);
        if (iv == v) {
            r.denom = 1;
            r.num = (int64_t)iv;
        }
        else {
            r.denom = (int64_t)1 << (maxbits - std::max(e, 0));
            r.num = (int64_t)round(v * r.denom);
            while ((r.denom & 1) == 0 && (r.num & 1) == 0) {
                r.num >>= 1;
                r.denom >>= 1;
            }
        }
    }
    return r;
}

static srational64_t doubleToSRational(double v)
{
    srational64_t r = doubleToRational(fabs(v), 30);
    r.num *= (v < 0 ? -1 : 1);
    return r;
}

constexpr size_t EXIF_HDR_SIZE = 8; // ('II' or 'MM'), (0x2A 0x00), (IFD0 offset: 4 bytes)
constexpr size_t IFD_ENTRY_SIZE = 12;
constexpr size_t IFD_MAX_INLINE_SIZE = 4;
constexpr size_t IFD_HDR_SIZE = 6;

size_t tagValueSize(ExifTagType type, size_t nvalues)
{
    size_t size = tagTypeSize(type) * nvalues;
    return (size + 1u) & ~1u;
}

static size_t computeOpcodeListSize(ExifTagId tagid, const std::vector<double>& v)
{
    constexpr size_t GAINMAP_HDR_SIZE = 18;
    constexpr size_t GAINMAP_HDR_BYTES = 92;

    CV_Assert(tagid == TAG_OPCODE_LIST2);

    size_t idx = 1, v_size = v.size();
    uint32_t i, ngainmaps = v_size > 0 ? (uint32_t)v[0] : 0u;
    size_t size = sizeof(uint32_t);
    for (i = 0; i < ngainmaps; i++) {
        if (idx + GAINMAP_HDR_SIZE > v_size)
            break;
        size_t gainmap_size = (size_t)v[idx + 2];
        size_t gainmap_nitems = gainmap_size - GAINMAP_HDR_SIZE;
        size += gainmap_nitems * sizeof(float) + GAINMAP_HDR_BYTES;
        idx += gainmap_size;
    }
    return size;
}

static size_t computeIFDSize(const std::vector<ExifEntry>* ifds,
    size_t nifds, size_t idx, size_t& values_size)
{
    CV_Assert(idx < nifds);
    const std::vector<ExifEntry>& ifd = ifds[idx];
    size_t i, ntags = ifd.size(), size = IFD_HDR_SIZE + IFD_ENTRY_SIZE * ntags;
    return size;
}

static size_t nextIFD(const std::vector<ExifEntry>& ifd)
{
    for (const ExifEntry& tag : ifd) {
        if (tag.tagId == TAG_NEXT_IFD) {
            return tag.value.field_u32;
        }
    }
    return 0u;
}

static void pack1(std::vector<uchar>& data, size_t& offset, uint8_t value)
{
    data.resize(std::max(data.size(), offset + 1));
    data[offset++] = (char)value;
}

static void pack2(std::vector<uchar>& data, size_t& offset,
    uint16_t value, bool bigendian_)
{
    size_t ofs = offset, bigendian = (size_t)bigendian_;
    data.resize(std::max(data.size(), ofs + sizeof(uint16_t)));
    uchar* ptr = data.data();
    ptr[ofs + bigendian] = (uchar)value;
    ptr[ofs + 1 - bigendian] = (uchar)(value >> 8);
    offset = ofs + sizeof(uint16_t);
}

static void pack4(std::vector<uchar>& data, size_t& offset,
    uint32_t value, bool bigendian_)
{
    size_t ofs = offset, bigendian = (size_t)bigendian_;
    data.resize(std::max(data.size(), ofs + sizeof(uint32_t)));
    uchar* ptr = data.data();
    ptr[ofs + bigendian * 3] = (uchar)value;
    ptr[ofs + 1 + bigendian] = (uchar)(value >> 8);
    ptr[ofs + 2 - bigendian] = (uchar)(value >> 16);
    ptr[ofs + 3 - bigendian * 3] = (uchar)(value >> 24);
    offset = ofs + sizeof(uint32_t);
}

static void pack8(std::vector<uchar>& data, size_t& offset,
    uint64_t value, bool bigendian_)
{
    size_t ofs = offset, bigendian = (size_t)bigendian_;
    data.resize(std::max(data.size(), ofs + sizeof(uint64_t)));
    uchar* ptr = data.data();
    uint32_t lo = (uint32_t)value, hi = (uint32_t)(value >> 32);
    ptr[ofs + bigendian * 7] = (uchar)lo;
    ptr[ofs + 1 + bigendian * 5] = (uchar)(lo >> 8);
    ptr[ofs + 2 + bigendian * 3] = (uchar)(lo >> 16);
    ptr[ofs + 3 + bigendian] = (uchar)(lo >> 24);
    ptr[ofs + 4 - bigendian] = (uchar)hi;
    ptr[ofs + 5 - bigendian * 3] = (uchar)(hi >> 8);
    ptr[ofs + 6 - bigendian * 5] = (uchar)(hi >> 16);
    ptr[ofs + 7 - bigendian * 7] = (uchar)(hi >> 24);
    offset = ofs + sizeof(uint64_t);
}

static void packFloat(std::vector<uchar>& data, size_t& offset,
    float value, bool bigendian_)
{
    Cv32suf u;
    u.f = value;
    pack4(data, offset, u.u, bigendian_);
}

static void packDouble(std::vector<uchar>& data, size_t& offset,
    double value, bool bigendian_)
{
    Cv64suf u;
    u.f = value;
    pack8(data, offset, u.u, bigendian_);
}

static bool packGainMaps(std::vector<uchar>& data, size_t& offset,
    const std::vector<double>& gainmaps,
    bool bigendian)
{
    constexpr uint32_t OPCODE_LIST_GAIN_MAP = 9;

    constexpr size_t GAINMAP_HDR_SIZE = 18;
    constexpr size_t MAP_POINTS_V = 11;
    constexpr size_t MAP_POINTS_H = 12;
    constexpr size_t MAP_NPLANES = 17;

    size_t idx = 1, total = gainmaps.size();
    uint32_t nopcodes = (uint32_t)gainmaps[0];
    pack4(data, offset, nopcodes, bigendian);

    for (uint32_t i = 0; i < nopcodes; i++) {
        if (idx + GAINMAP_HDR_SIZE > total)
            return false;

        //size_t start = idx;
        uint32_t gainmap_size = (uint32_t)gainmaps[idx + 2];

        uint32_t map_points_v = (uint32_t)gainmaps[idx + MAP_POINTS_V];
        uint32_t map_points_h = (uint32_t)gainmaps[idx + MAP_POINTS_H];
        uint32_t map_nplanes = (uint32_t)gainmaps[idx + MAP_NPLANES];
        size_t nitems = map_points_v * map_points_h * map_nplanes;
        size_t gainmap_end = idx + gainmap_size;

        if (gainmap_size != nitems + GAINMAP_HDR_SIZE ||
            gainmap_end > total)
            return false;

        pack4(data, offset, OPCODE_LIST_GAIN_MAP, bigendian);
        pack4(data, offset, (uint32_t)gainmaps[idx++], bigendian); // dng_version
        pack4(data, offset, (uint32_t)gainmaps[idx++], bigendian); // flags
        size_t nbytes_offset = offset;
        pack4(data, offset, (uint32_t)gainmaps[idx++], bigendian); // nbytes: to be updated later

        pack4(data, offset, (uint32_t)gainmaps[idx++], bigendian); // top
        pack4(data, offset, (uint32_t)gainmaps[idx++], bigendian); // left
        pack4(data, offset, (uint32_t)gainmaps[idx++], bigendian); // bottom
        pack4(data, offset, (uint32_t)gainmaps[idx++], bigendian); // right

        pack4(data, offset, (uint32_t)gainmaps[idx++], bigendian); // plane
        pack4(data, offset, (uint32_t)gainmaps[idx++], bigendian); // planes
        pack4(data, offset, (uint32_t)gainmaps[idx++], bigendian); // row_pitch
        pack4(data, offset, (uint32_t)gainmaps[idx++], bigendian); // col_pitch

        pack4(data, offset, (uint32_t)gainmaps[idx++], bigendian); // map_points_v
        pack4(data, offset, (uint32_t)gainmaps[idx++], bigendian); // map_points_h

        packDouble(data, offset, gainmaps[idx++], bigendian); // map_spacing_v
        packDouble(data, offset, gainmaps[idx++], bigendian); // map_spacing_h
        packDouble(data, offset, gainmaps[idx++], bigendian); // map_offset_v
        packDouble(data, offset, gainmaps[idx++], bigendian); // map_offset_h

        pack4(data, offset, (uint32_t)gainmaps[idx++], bigendian); // map_nplanes

        for (uint32_t p = 0; p < map_nplanes; p++) {
            for (uint32_t y = 0; y < map_points_v; y++) {
                for (uint32_t x = 0; x < map_points_h; x++)
                    packFloat(data, offset, (float)gainmaps[idx++], bigendian);
            }
        }

        uint32_t nbytes = (uint32_t)(offset - nbytes_offset - 4);
        //printf("wrote: opcode_id=%u, dng_version=%u, flags=%u, nbytes=%u\n",
        //       OPCODE_LIST_GAIN_MAP, (uint64_t)gainmaps[start], (uint64_t)gainmaps[start+1], nbytes);
        pack4(data, nbytes_offset, nbytes, bigendian); // store the actual size of gainmap in bytes
        idx = gainmap_end;
    }
    CV_Assert(idx == gainmaps.size());
    return true;
}

static void packIFD(const std::vector<ExifEntry>* ifds, size_t nifds, size_t idx,
    std::vector<uchar>& data, size_t& offset,
    size_t& values_offset, size_t& image_data_offset,
    bool bigendian, bool sorttags, bool adjust_stripe_offsets)
{
};

static uint8_t unpack1(const std::vector<uchar>& data, size_t& offset)
{
    CV_Assert(offset + 1 <= data.size());
    return (uint8_t)data[offset++];
}

static uint16_t unpack2(const std::vector<uchar>& data, size_t& offset, bool bigendian_)
{
    size_t ofs = offset, bigendian = (size_t)bigendian_;
    CV_Assert(offset + sizeof(uint16_t) <= data.size());
    const uint8_t* ptr = (const uint8_t*)data.data();
    unsigned value = ptr[ofs + bigendian] | (ptr[ofs + 1 - bigendian] << 8);
    offset = ofs + sizeof(uint16_t);
    return (uint16_t)value;
}

static uint32_t unpack4(const std::vector<uchar>& data, size_t& offset, bool bigendian_)
{
    size_t ofs = offset, bigendian = (size_t)bigendian_;
    CV_Assert(offset + sizeof(uint32_t) <= data.size());
    const uint8_t* ptr = (const uint8_t*)data.data();
    unsigned value = ptr[ofs + bigendian * 3] |
        (ptr[ofs + 1 + bigendian] << 8) |
        (ptr[ofs + 2 - bigendian] << 16) |
        (ptr[ofs + 3 - bigendian * 3] << 24);
    offset = ofs + sizeof(uint32_t);
    return (uint32_t)value;
}

static uint64_t unpack8(const std::vector<uchar>& data, size_t& offset, bool bigendian_)
{
    size_t ofs = offset, bigendian = (size_t)bigendian_;
    CV_Assert(offset + sizeof(uint64_t) <= data.size());
    const uint8_t* ptr = (const uint8_t*)data.data();
    unsigned lo = ptr[ofs + bigendian * 7] |
        (ptr[ofs + 1 + bigendian * 5] << 8) |
        (ptr[ofs + 2 + bigendian * 3] << 16) |
        (ptr[ofs + 3 + bigendian] << 24);
    unsigned hi = ptr[ofs + 4 - bigendian] |
        (ptr[ofs + 5 - bigendian * 3] << 8) |
        (ptr[ofs + 6 - bigendian * 5] << 16) |
        (ptr[ofs + 7 - bigendian * 7] << 24);
    offset = ofs + sizeof(uint64_t);
    return ((uint64_t)hi << 32) | lo;
}

static float unpackFloat(const std::vector<uchar>& data, size_t& offset, bool bigendian_)
{
    Cv32suf u;
    u.u = unpack4(data, offset, bigendian_);
    return u.f;
}

static double unpackDouble(const std::vector<uchar>& data, size_t& offset, bool bigendian_)
{
    Cv64suf u;
    u.u = unpack8(data, offset, bigendian_);
    return u.f;
}

static bool unpackGainMap(const std::vector<uchar>& data, size_t& offset,
    size_t nbytes, std::vector<double>& gainmaps,
    bool bigendian)
{
    size_t limit = offset + nbytes;
    if (nbytes < 16 * sizeof(uint32_t))
        return false;

    gainmaps.push_back(unpack4(data, offset, bigendian)); // top
    gainmaps.push_back(unpack4(data, offset, bigendian)); // left
    gainmaps.push_back(unpack4(data, offset, bigendian)); // bottom
    gainmaps.push_back(unpack4(data, offset, bigendian)); // right

    gainmaps.push_back(unpack4(data, offset, bigendian)); // plane
    gainmaps.push_back(unpack4(data, offset, bigendian)); // planes
    gainmaps.push_back(unpack4(data, offset, bigendian)); // row_pitch
    gainmaps.push_back(unpack4(data, offset, bigendian)); // col_pitch

    uint32_t map_points_v = unpack4(data, offset, bigendian);
    uint32_t map_points_h = unpack4(data, offset, bigendian);
    gainmaps.push_back(map_points_v); // map_points_v
    gainmaps.push_back(map_points_h); // map_points_h

    gainmaps.push_back(unpackDouble(data, offset, bigendian)); // map_spacing_v
    gainmaps.push_back(unpackDouble(data, offset, bigendian)); // map_spacing_h
    gainmaps.push_back(unpackDouble(data, offset, bigendian)); // map_origin_v
    gainmaps.push_back(unpackDouble(data, offset, bigendian)); // map_origin_h

    uint32_t map_nplanes = unpack4(data, offset, bigendian);
    gainmaps.push_back(map_nplanes);

    size_t nitems = map_points_v * map_points_h * map_nplanes;

    if (offset + nitems * sizeof(float) > limit)
        return false;

    for (uint32_t p = 0; p < map_nplanes; p++) {
        for (uint32_t y = 0; y < map_points_v; y++) {
            for (uint32_t x = 0; x < map_points_h; x++)
                gainmaps.push_back(unpackFloat(data, offset, bigendian));
        }
    }
    return true;
}

static bool unpackOpcodeList(ExifTagId tagid, const std::vector<uchar>& data, size_t& offset,
    std::vector<double>& gainmaps, bool bigendian)
{
    CV_UNUSED(tagid);
    constexpr uint32_t OPCODE_LIST_GAIN_MAP = 9;
    uint32_t nopcodes = unpack4(data, offset, bigendian);
    uint32_t ngainmaps = 0;
    //bool ok = true;
    gainmaps.clear();
    gainmaps.push_back(nopcodes);

    for (uint32_t i = 0; i < nopcodes; i++) {
        uint32_t opcode_id = unpack4(data, offset, bigendian);
        uint32_t dng_version = unpack4(data, offset, bigendian);
        uint32_t flags = unpack4(data, offset, bigendian);
        uint32_t nbytes = unpack4(data, offset, bigendian);
        //printf("read: opcode_id=%u, dng_version=%u, flags=%u, nbytes=%u\n", opcode_id, dng_version, flags, nbytes);

        if (opcode_id == OPCODE_LIST_GAIN_MAP) {
            size_t gainmapOffset = offset;
            size_t gainmapStart = gainmaps.size();
            gainmaps.push_back(dng_version);
            gainmaps.push_back(flags);
            gainmaps.push_back(0);

            if (!unpackGainMap(data, gainmapOffset, nbytes, gainmaps, bigendian)) {
                gainmaps.clear();
                return false;
            }

            ngainmaps++;
            gainmaps[gainmapStart + 2] = (double)gainmaps.size() - gainmapStart;
            CV_Assert(gainmapOffset <= offset + nbytes);
        }
        offset += nbytes;
    }
    gainmaps[0] = ngainmaps;
    return ngainmaps > 0;
}

static void dumpIFD(std::ostream& strm, int indent,
    const std::vector<std::vector<ExifEntry> >& exif, size_t idx)
{
    CV_Assert(idx < exif.size());
    const std::vector<ExifEntry>& ifd = exif[idx];
    size_t i, ntags = ifd.size();
    std::string subindent = std::string(indent + 3, ' ');
    strm << "{\n";
    for (i = 0; i < ntags; i++) {
        const ExifEntry& tag = ifd[i];
        strm << subindent;
        if (tag.type == TAG_TYPE_IFD) {
            int32_t sub_idx = tag.value.field_s32;
            strm << exifTagIdToString(tag.tagId) << ": ";
            dumpIFD(strm, indent + 3, exif, (size_t)sub_idx);
        }
        else {
            tag.dump(strm);
        }
        if (i + 1 < ntags)
            strm << ",";
        strm << "\n";
    }
    strm << std::string(indent, ' ') << "}";
}

void dumpExif(std::ostream& strm, const std::vector<std::vector<ExifEntry> >& exif)
{
    if (exif.empty()) {
        strm << "{}";
    }
    else {
        dumpIFD(strm, 0, exif, 0);
    }
}

}

/* End of file. */
