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

#include "old_ml_precomp.hpp"


CvStatModel::CvStatModel()
{
    default_model_name = "my_stat_model";
}


CvStatModel::~CvStatModel()
{
    clear();
}


void CvStatModel::clear()
{
}


void CvStatModel::save( const char* filename, const char* name ) const
{
    cv::FileStorage fs(filename, cv::FileStorage::WRITE);
    write( fs, name ? name : default_model_name );
}


void CvStatModel::load( const char* filename, const char* name )
{
    CV_UNUSED(filename);
    CV_UNUSED(name);
}


void CvStatModel::write( cv::FileStorage&, const char* ) const
{
    OPENCV_ERROR( CV_StsNotImplemented, "CvStatModel::write", "" );
}

void CvStatModel::read( const cv::FileNode& node )
{
    CV_UNUSED(node);
    OPENCV_ERROR( CV_StsNotImplemented, "CvStatModel::read", "" );
}

CvMat* icvGenerateRandomClusterCenters ( int seed, const CvMat* data,
                                         int num_of_clusters, CvMat* _centers )
{
    CvMat* centers = _centers;

    CV_FUNCNAME("icvGenerateRandomClusterCenters");
    __BEGIN__;

    CvRNG rng;
    CvMat data_comp, centers_comp;
    CvPoint minLoc, maxLoc; // Not used, just for function "cvMinMaxLoc"
    double minVal, maxVal;
    int i;
    int dim = data ? data->cols : 0;

    if( ICV_IS_MAT_OF_TYPE(data, CV_32FC1) )
    {
        if( _centers && !ICV_IS_MAT_OF_TYPE (_centers, CV_32FC1) )
        {
            CV_ERROR(CV_StsBadArg,"");
        }
        else if( !_centers )
            CV_CALL(centers = cvCreateMat (num_of_clusters, dim, CV_32FC1));
    }
    else if( ICV_IS_MAT_OF_TYPE(data, CV_64FC1) )
    {
        if( _centers && !ICV_IS_MAT_OF_TYPE (_centers, CV_64FC1) )
        {
            CV_ERROR(CV_StsBadArg,"");
        }
        else if( !_centers )
            CV_CALL(centers = cvCreateMat (num_of_clusters, dim, CV_64FC1));
    }
    else
        CV_ERROR (CV_StsBadArg,"");

    if( num_of_clusters < 1 )
        CV_ERROR (CV_StsBadArg,"");

    rng = cvRNG(seed);
    for (i = 0; i < dim; i++)
    {
        CV_CALL(cvGetCol (data, &data_comp, i));
        CV_CALL(cvMinMaxLoc (&data_comp, &minVal, &maxVal, &minLoc, &maxLoc));
        CV_CALL(cvGetCol (centers, &centers_comp, i));
        CV_CALL(cvRandArr (&rng, &centers_comp, CV_RAND_UNI, cvScalarAll(minVal), cvScalarAll(maxVal)));
    }

    __END__;

    if( (cvGetErrStatus () < 0) || (centers != _centers) )
        cvReleaseMat (&centers);

    return _centers ? _centers : centers;
} // end of icvGenerateRandomClusterCenters

static int CV_CDECL
icvCmpIntegers( const void* a, const void* b )
{
    return *(const int*)a - *(const int*)b;
}


static int CV_CDECL
icvCmpIntegersPtr( const void* _a, const void* _b )
{
    int a = **(const int**)_a;
    int b = **(const int**)_b;
    return (a < b ? -1 : 0)|(a > b);
}


CvMat*
cvPreprocessIndexArray( const CvMat* idx_arr, int data_arr_size, bool check_for_duplicates )
{
    CvMat* idx = 0;

    CV_FUNCNAME( "cvPreprocessIndexArray" );

    __BEGIN__;

    int i, idx_total, idx_selected = 0, step, type, prev = INT_MIN, is_sorted = 1;
    uchar* srcb = 0;
    int* srci = 0;
    int* dsti;

    if( !CV_IS_MAT(idx_arr) )
        CV_ERROR( CV_StsBadArg, "Invalid index array" );

    if( idx_arr->rows != 1 && idx_arr->cols != 1 )
        CV_ERROR( CV_StsBadSize, "the index array must be 1-dimensional" );

    idx_total = idx_arr->rows + idx_arr->cols - 1;
    srcb = idx_arr->data.ptr;
    srci = idx_arr->data.i;

    type = CV_MAT_TYPE(idx_arr->type);
    step = CV_IS_MAT_CONT(idx_arr->type) ? 1 : idx_arr->step/CV_ELEM_SIZE(type);

    switch( type )
    {
    case CV_8UC1:
    case CV_8SC1:
        // idx_arr is array of 1's and 0's -
        // i.e. it is a mask of the selected components
        if( idx_total != data_arr_size )
            CV_ERROR( CV_StsUnmatchedSizes,
            "Component mask should contain as many elements as the total number of input variables" );

        for( i = 0; i < idx_total; i++ )
            idx_selected += srcb[i*step] != 0;

        if( idx_selected == 0 )
            CV_ERROR( CV_StsOutOfRange, "No components/input_variables is selected!" );

        break;
    case CV_32SC1:
        // idx_arr is array of integer indices of selected components
        if( idx_total > data_arr_size )
            CV_ERROR( CV_StsOutOfRange,
            "index array may not contain more elements than the total number of input variables" );
        idx_selected = idx_total;
        // check if sorted already
        for( i = 0; i < idx_total; i++ )
        {
            int val = srci[i*step];
            if( val >= prev )
            {
                is_sorted = 0;
                break;
            }
            prev = val;
        }
        break;
    default:
        CV_ERROR( CV_StsUnsupportedFormat, "Unsupported index array data type "
                                           "(it should be 8uC1, 8sC1 or 32sC1)" );
    }

    CV_CALL( idx = cvCreateMat( 1, idx_selected, CV_32SC1 ));
    dsti = idx->data.i;

    if( type < CV_32SC1 )
    {
        for( i = 0; i < idx_total; i++ )
            if( srcb[i*step] )
                *dsti++ = i;
    }
    else
    {
        for( i = 0; i < idx_total; i++ )
            dsti[i] = srci[i*step];

        if( !is_sorted )
            qsort( dsti, idx_total, sizeof(dsti[0]), icvCmpIntegers );

        if( dsti[0] < 0 || dsti[idx_total-1] >= data_arr_size )
            CV_ERROR( CV_StsOutOfRange, "the index array elements are out of range" );

        if( check_for_duplicates )
        {
            for( i = 1; i < idx_total; i++ )
                if( dsti[i] <= dsti[i-1] )
                    CV_ERROR( CV_StsBadArg, "There are duplicated index array elements" );
        }
    }

    __END__;

    if( cvGetErrStatus() < 0 )
        cvReleaseMat( &idx );

    return idx;
}


CvMat*
cvPreprocessVarType( const CvMat* var_type, const CvMat* var_idx,
                     int var_count, int* response_type )
{
    CvMat* out_var_type = 0;
    CV_FUNCNAME( "cvPreprocessVarType" );

    if( response_type )
        *response_type = -1;

    __BEGIN__;

    int i, tm_size, tm_step;
    //int* map = 0;
    const uchar* src;
    uchar* dst;

    if( !CV_IS_MAT(var_type) )
        CV_ERROR( var_type ? CV_StsBadArg : CV_StsNullPtr, "Invalid or absent var_type array" );

    if( var_type->rows != 1 && var_type->cols != 1 )
        CV_ERROR( CV_StsBadSize, "var_type array must be 1-dimensional" );

    if( !CV_IS_MASK_ARR(var_type))
        CV_ERROR( CV_StsUnsupportedFormat, "type mask must be 8uC1 or 8sC1 array" );

    tm_size = var_type->rows + var_type->cols - 1;
    tm_step = var_type->rows == 1 ? 1 : var_type->step/CV_ELEM_SIZE(var_type->type);

    if( /*tm_size != var_count &&*/ tm_size != var_count + 1 )
        CV_ERROR( CV_StsBadArg,
        "type mask must be of <input var count> + 1 size" );

    if( response_type && tm_size > var_count )
        *response_type = var_type->data.ptr[var_count*tm_step] != 0;

    if( var_idx )
    {
        if( !CV_IS_MAT(var_idx) || CV_MAT_TYPE(var_idx->type) != CV_32SC1 ||
            (var_idx->rows != 1 && var_idx->cols != 1) || !CV_IS_MAT_CONT(var_idx->type) )
            CV_ERROR( CV_StsBadArg, "var index array should be continuous 1-dimensional integer vector" );
        if( var_idx->rows + var_idx->cols - 1 > var_count )
            CV_ERROR( CV_StsBadSize, "var index array is too large" );
        //map = var_idx->data.i;
        var_count = var_idx->rows + var_idx->cols - 1;
    }

    CV_CALL( out_var_type = cvCreateMat( 1, var_count, CV_8UC1 ));
    src = var_type->data.ptr;
    dst = out_var_type->data.ptr;

    for( i = 0; i < var_count; i++ )
    {
        //int idx = map ? map[i] : i;
        assert( (unsigned)/*idx*/i < (unsigned)tm_size );
        dst[i] = (uchar)(src[/*idx*/i*tm_step] != 0);
    }

    __END__;

    return out_var_type;
}


CvMat*
cvPreprocessOrderedResponses( const CvMat* responses, const CvMat* sample_idx, int sample_all )
{
    CvMat* out_responses = 0;

    CV_FUNCNAME( "cvPreprocessOrderedResponses" );

    __BEGIN__;

    int i, r_type, r_step;
    const int* map = 0;
    float* dst;
    int sample_count = sample_all;

    if( !CV_IS_MAT(responses) )
        CV_ERROR( CV_StsBadArg, "Invalid response array" );

    if( responses->rows != 1 && responses->cols != 1 )
        CV_ERROR( CV_StsBadSize, "Response array must be 1-dimensional" );

    if( responses->rows + responses->cols - 1 != sample_count )
        CV_ERROR( CV_StsUnmatchedSizes,
        "Response array must contain as many elements as the total number of samples" );

    r_type = CV_MAT_TYPE(responses->type);
    if( r_type != CV_32FC1 && r_type != CV_32SC1 )
        CV_ERROR( CV_StsUnsupportedFormat, "Unsupported response type" );

    r_step = responses->step ? responses->step / CV_ELEM_SIZE(responses->type) : 1;

    if( r_type == CV_32FC1 && CV_IS_MAT_CONT(responses->type) && !sample_idx )
    {
        out_responses = cvCloneMat( responses );
        EXIT;
    }

    if( sample_idx )
    {
        if( !CV_IS_MAT(sample_idx) || CV_MAT_TYPE(sample_idx->type) != CV_32SC1 ||
            (sample_idx->rows != 1 && sample_idx->cols != 1) || !CV_IS_MAT_CONT(sample_idx->type) )
            CV_ERROR( CV_StsBadArg, "sample index array should be continuous 1-dimensional integer vector" );
        if( sample_idx->rows + sample_idx->cols - 1 > sample_count )
            CV_ERROR( CV_StsBadSize, "sample index array is too large" );
        map = sample_idx->data.i;
        sample_count = sample_idx->rows + sample_idx->cols - 1;
    }

    CV_CALL( out_responses = cvCreateMat( 1, sample_count, CV_32FC1 ));

    dst = out_responses->data.fl;
    if( r_type == CV_32FC1 )
    {
        const float* src = responses->data.fl;
        for( i = 0; i < sample_count; i++ )
        {
            int idx = map ? map[i] : i;
            assert( (unsigned)idx < (unsigned)sample_all );
            dst[i] = src[idx*r_step];
        }
    }
    else
    {
        const int* src = responses->data.i;
        for( i = 0; i < sample_count; i++ )
        {
            int idx = map ? map[i] : i;
            assert( (unsigned)idx < (unsigned)sample_all );
            dst[i] = (float)src[idx*r_step];
        }
    }

    __END__;

    return out_responses;
}

CvMat*
cvPreprocessCategoricalResponses( const CvMat* responses,
    const CvMat* sample_idx, int sample_all,
    CvMat** out_response_map, CvMat** class_counts )
{
    CvMat* out_responses = 0;
    int** response_ptr = 0;

    CV_FUNCNAME( "cvPreprocessCategoricalResponses" );

    if( out_response_map )
        *out_response_map = 0;

    if( class_counts )
        *class_counts = 0;

    __BEGIN__;

    int i, r_type, r_step;
    int cls_count = 1, prev_cls, prev_i;
    const int* map = 0;
    const int* srci;
    const float* srcfl;
    int* dst;
    int* cls_map;
    int* cls_counts = 0;
    int sample_count = sample_all;

    if( !CV_IS_MAT(responses) )
        CV_ERROR( CV_StsBadArg, "Invalid response array" );

    if( responses->rows != 1 && responses->cols != 1 )
        CV_ERROR( CV_StsBadSize, "Response array must be 1-dimensional" );

    if( responses->rows + responses->cols - 1 != sample_count )
        CV_ERROR( CV_StsUnmatchedSizes,
        "Response array must contain as many elements as the total number of samples" );

    r_type = CV_MAT_TYPE(responses->type);
    if( r_type != CV_32FC1 && r_type != CV_32SC1 )
        CV_ERROR( CV_StsUnsupportedFormat, "Unsupported response type" );

    r_step = responses->rows == 1 ? 1 : responses->step / CV_ELEM_SIZE(responses->type);

    if( sample_idx )
    {
        if( !CV_IS_MAT(sample_idx) || CV_MAT_TYPE(sample_idx->type) != CV_32SC1 ||
            (sample_idx->rows != 1 && sample_idx->cols != 1) || !CV_IS_MAT_CONT(sample_idx->type) )
            CV_ERROR( CV_StsBadArg, "sample index array should be continuous 1-dimensional integer vector" );
        if( sample_idx->rows + sample_idx->cols - 1 > sample_count )
            CV_ERROR( CV_StsBadSize, "sample index array is too large" );
        map = sample_idx->data.i;
        sample_count = sample_idx->rows + sample_idx->cols - 1;
    }

    CV_CALL( out_responses = cvCreateMat( 1, sample_count, CV_32SC1 ));

    if( !out_response_map )
        CV_ERROR( CV_StsNullPtr, "out_response_map pointer is NULL" );

    CV_CALL( response_ptr = (int**)cvAlloc( sample_count*sizeof(response_ptr[0])));

    srci = responses->data.i;
    srcfl = responses->data.fl;
    dst = out_responses->data.i;

    for( i = 0; i < sample_count; i++ )
    {
        int idx = map ? map[i] : i;
        assert( (unsigned)idx < (unsigned)sample_all );
        if( r_type == CV_32SC1 )
            dst[i] = srci[idx*r_step];
        else
        {
            float rf = srcfl[idx*r_step];
            int ri = cvRound(rf);
            if( ri != rf )
            {
                char buf[100];
                sprintf( buf, "response #%d is not integral", idx );
                CV_ERROR( CV_StsBadArg, buf );
            }
            dst[i] = ri;
        }
        response_ptr[i] = dst + i;
    }

    qsort( response_ptr, sample_count, sizeof(int*), icvCmpIntegersPtr );

    // count the classes
    for( i = 1; i < sample_count; i++ )
        cls_count += *response_ptr[i] != *response_ptr[i-1];

    if( cls_count < 2 )
        CV_ERROR( CV_StsBadArg, "There is only a single class" );

    CV_CALL( *out_response_map = cvCreateMat( 1, cls_count, CV_32SC1 ));

    if( class_counts )
    {
        CV_CALL( *class_counts = cvCreateMat( 1, cls_count, CV_32SC1 ));
        cls_counts = (*class_counts)->data.i;
    }

    // compact the class indices and build the map
    prev_cls = ~*response_ptr[0];
    cls_count = -1;
    cls_map = (*out_response_map)->data.i;

    for( i = 0, prev_i = -1; i < sample_count; i++ )
    {
        int cur_cls = *response_ptr[i];
        if( cur_cls != prev_cls )
        {
            if( cls_counts && cls_count >= 0 )
                cls_counts[cls_count] = i - prev_i;
            cls_map[++cls_count] = prev_cls = cur_cls;
            prev_i = i;
        }
        *response_ptr[i] = cls_count;
    }

    if( cls_counts )
        cls_counts[cls_count] = i - prev_i;

    __END__;

    cvFree( &response_ptr );

    return out_responses;
}


const float**
cvGetTrainSamples( const CvMat* train_data, int tflag,
                   const CvMat* var_idx, const CvMat* sample_idx,
                   int* _var_count, int* _sample_count,
                   bool always_copy_data )
{
    float** samples = 0;

    CV_FUNCNAME( "cvGetTrainSamples" );

    __BEGIN__;

    int i, j, var_count, sample_count, s_step, v_step;
    bool copy_data;
    const float* data;
    const int *s_idx, *v_idx;

    if( !CV_IS_MAT(train_data) )
        CV_ERROR( CV_StsBadArg, "Invalid or NULL training data matrix" );

    var_count = var_idx ? var_idx->cols + var_idx->rows - 1 :
                tflag == CV_ROW_SAMPLE ? train_data->cols : train_data->rows;
    sample_count = sample_idx ? sample_idx->cols + sample_idx->rows - 1 :
                   tflag == CV_ROW_SAMPLE ? train_data->rows : train_data->cols;

    if( _var_count )
        *_var_count = var_count;

    if( _sample_count )
        *_sample_count = sample_count;

    copy_data = tflag != CV_ROW_SAMPLE || var_idx || always_copy_data;

    CV_CALL( samples = (float**)cvAlloc(sample_count*sizeof(samples[0]) +
                (copy_data ? 1 : 0)*var_count*sample_count*sizeof(samples[0][0])) );
    data = train_data->data.fl;
    s_step = train_data->step / sizeof(samples[0][0]);
    v_step = 1;
    s_idx = sample_idx ? sample_idx->data.i : 0;
    v_idx = var_idx ? var_idx->data.i : 0;

    if( !copy_data )
    {
        for( i = 0; i < sample_count; i++ )
            samples[i] = (float*)(data + (s_idx ? s_idx[i] : i)*s_step);
    }
    else
    {
        samples[0] = (float*)(samples + sample_count);
        if( tflag != CV_ROW_SAMPLE )
            CV_SWAP( s_step, v_step, i );

        for( i = 0; i < sample_count; i++ )
        {
            float* dst = samples[i] = samples[0] + i*var_count;
            const float* src = data + (s_idx ? s_idx[i] : i)*s_step;

            if( !v_idx )
                for( j = 0; j < var_count; j++ )
                    dst[j] = src[j*v_step];
            else
                for( j = 0; j < var_count; j++ )
                    dst[j] = src[v_idx[j]*v_step];
        }
    }

    __END__;

    return (const float**)samples;
}


void
cvCheckTrainData( const CvMat* train_data, int tflag,
                  const CvMat* missing_mask,
                  int* var_all, int* sample_all )
{
    CV_FUNCNAME( "cvCheckTrainData" );

    if( var_all )
        *var_all = 0;

    if( sample_all )
        *sample_all = 0;

    __BEGIN__;

    // check parameter types and sizes
    if( !CV_IS_MAT(train_data) || CV_MAT_TYPE(train_data->type) != CV_32FC1 )
        CV_ERROR( CV_StsBadArg, "train data must be floating-point matrix" );

    if( missing_mask )
    {
        if( !CV_IS_MAT(missing_mask) || !CV_IS_MASK_ARR(missing_mask) ||
            !CV_ARE_SIZES_EQ(train_data, missing_mask) )
            CV_ERROR( CV_StsBadArg,
            "missing value mask must be 8-bit matrix of the same size as training data" );
    }

    if( tflag != CV_ROW_SAMPLE && tflag != CV_COL_SAMPLE )
        CV_ERROR( CV_StsBadArg,
        "Unknown training data layout (must be CV_ROW_SAMPLE or CV_COL_SAMPLE)" );

    if( var_all )
        *var_all = tflag == CV_ROW_SAMPLE ? train_data->cols : train_data->rows;

    if( sample_all )
        *sample_all = tflag == CV_ROW_SAMPLE ? train_data->rows : train_data->cols;

    __END__;
}


int
cvPrepareTrainData( const char* /*funcname*/,
                    const CvMat* train_data, int tflag,
                    const CvMat* responses, int response_type,
                    const CvMat* var_idx,
                    const CvMat* sample_idx,
                    bool always_copy_data,
                    const float*** out_train_samples,
                    int* _sample_count,
                    int* _var_count,
                    int* _var_all,
                    CvMat** out_responses,
                    CvMat** out_response_map,
                    CvMat** out_var_idx,
                    CvMat** out_sample_idx )
{
    int ok = 0;
    CvMat* _var_idx = 0;
    CvMat* _sample_idx = 0;
    CvMat* _responses = 0;
    int sample_all = 0, sample_count = 0, var_all = 0, var_count = 0;

    CV_FUNCNAME( "cvPrepareTrainData" );

    // step 0. clear all the output pointers to ensure we do not try
    // to call free() with uninitialized pointers
    if( out_responses )
        *out_responses = 0;

    if( out_response_map )
        *out_response_map = 0;

    if( out_var_idx )
        *out_var_idx = 0;

    if( out_sample_idx )
        *out_sample_idx = 0;

    if( out_train_samples )
        *out_train_samples = 0;

    if( _sample_count )
        *_sample_count = 0;

    if( _var_count )
        *_var_count = 0;

    if( _var_all )
        *_var_all = 0;

    __BEGIN__;

    if( !out_train_samples )
        CV_ERROR( CV_StsBadArg, "output pointer to train samples is NULL" );

    CV_CALL( cvCheckTrainData( train_data, tflag, 0, &var_all, &sample_all ));

    if( sample_idx )
        CV_CALL( _sample_idx = cvPreprocessIndexArray( sample_idx, sample_all ));
    if( var_idx )
        CV_CALL( _var_idx = cvPreprocessIndexArray( var_idx, var_all ));

    if( responses )
    {
        if( !out_responses )
            CV_ERROR( CV_StsNullPtr, "output response pointer is NULL" );

        if( response_type == CV_VAR_NUMERICAL )
        {
            CV_CALL( _responses = cvPreprocessOrderedResponses( responses,
                                                _sample_idx, sample_all ));
        }
        else
        {
            CV_CALL( _responses = cvPreprocessCategoricalResponses( responses,
                                _sample_idx, sample_all, out_response_map, 0 ));
        }
    }

    CV_CALL( *out_train_samples =
                cvGetTrainSamples( train_data, tflag, _var_idx, _sample_idx,
                                   &var_count, &sample_count, always_copy_data ));

    ok = 1;

    __END__;

    if( ok )
    {
        if( out_responses )
            *out_responses = _responses, _responses = 0;

        if( out_var_idx )
            *out_var_idx = _var_idx, _var_idx = 0;

        if( out_sample_idx )
            *out_sample_idx = _sample_idx, _sample_idx = 0;

        if( _sample_count )
            *_sample_count = sample_count;

        if( _var_count )
            *_var_count = var_count;

        if( _var_all )
            *_var_all = var_all;
    }
    else
    {
        if( out_response_map )
            cvReleaseMat( out_response_map );
        cvFree( out_train_samples );
    }

    if( _responses != responses )
        cvReleaseMat( &_responses );
    cvReleaseMat( &_var_idx );
    cvReleaseMat( &_sample_idx );

    return ok;
}
/* End of file */
