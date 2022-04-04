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

#ifndef OPENCV_OLD_ML_HPP
#define OPENCV_OLD_ML_HPP

#ifdef __cplusplus
#  include "opencv2/core.hpp"
#endif

#include "opencv2/core/core_c.h"
#include <limits.h>

#ifdef __cplusplus

#include <map>
#include <iostream>

// Apple defines a check() macro somewhere in the debug headers
// that interferes with a method definition in this header
#undef check

/****************************************************************************************\
*                               Main struct definitions                                  *
\****************************************************************************************/

/* log(2*PI) */
#define CV_LOG2PI (1.8378770664093454835606594728112)

/* columns of <trainData> matrix are training samples */
#define CV_COL_SAMPLE 0

/* rows of <trainData> matrix are training samples */
#define CV_ROW_SAMPLE 1

#define CV_IS_ROW_SAMPLE(flags) ((flags) & CV_ROW_SAMPLE)

struct CvVectors
{
    int type;
    int dims, count;
    CvVectors* next;
    union
    {
        uchar** ptr;
        float** fl;
        double** db;
    } data;
};

#if 0
/* A structure, representing the lattice range of statmodel parameters.
   It is used for optimizing statmodel parameters by cross-validation method.
   The lattice is logarithmic, so <step> must be greater than 1. */
typedef struct CvParamLattice
{
    double min_val;
    double max_val;
    double step;
}
CvParamLattice;

CV_INLINE CvParamLattice cvParamLattice( double min_val, double max_val,
                                         double log_step )
{
    CvParamLattice pl;
    pl.min_val = MIN( min_val, max_val );
    pl.max_val = MAX( min_val, max_val );
    pl.step = MAX( log_step, 1. );
    return pl;
}

CV_INLINE CvParamLattice cvDefaultParamLattice( void )
{
    CvParamLattice pl = {0,0,0};
    return pl;
}
#endif

/* Variable type */
#define CV_VAR_NUMERICAL    0
#define CV_VAR_ORDERED      0
#define CV_VAR_CATEGORICAL  1

#define CV_TYPE_NAME_ML_SVM         "opencv-ml-svm"
#define CV_TYPE_NAME_ML_KNN         "opencv-ml-knn"
#define CV_TYPE_NAME_ML_NBAYES      "opencv-ml-bayesian"
#define CV_TYPE_NAME_ML_BOOSTING    "opencv-ml-boost-tree"
#define CV_TYPE_NAME_ML_TREE        "opencv-ml-tree"
#define CV_TYPE_NAME_ML_ANN_MLP     "opencv-ml-ann-mlp"
#define CV_TYPE_NAME_ML_CNN         "opencv-ml-cnn"
#define CV_TYPE_NAME_ML_RTREES      "opencv-ml-random-trees"
#define CV_TYPE_NAME_ML_ERTREES     "opencv-ml-extremely-randomized-trees"
#define CV_TYPE_NAME_ML_GBT         "opencv-ml-gradient-boosting-trees"

#define CV_TRAIN_ERROR  0
#define CV_TEST_ERROR   1

class CvStatModel
{
public:
    CvStatModel();
    virtual ~CvStatModel();

    virtual void clear();

    CV_WRAP virtual void save( const char* filename, const char* name=0 ) const;
    CV_WRAP virtual void load( const char* filename, const char* name=0 );

    virtual void write( cv::FileStorage& storage, const char* name ) const;
    virtual void read( const cv::FileNode& node );

protected:
    const char* default_model_name;
};

class CvMLData;

/****************************************************************************************\
*                                      Decision Tree                                     *
\****************************************************************************************/\
struct CvPair16u32s
{
    unsigned short* u;
    int* i;
};


#define CV_DTREE_CAT_DIR(idx,subset) \
    (2*((subset[(idx)>>5]&(1 << ((idx) & 31)))==0)-1)

struct CvDTreeSplit
{
    int var_idx;
    int condensed_idx;
    int inversed;
    float quality;
    CvDTreeSplit* next;
    union
    {
        int subset[2];
        struct
        {
            float c;
            int split_point;
        }
        ord;
    };
};

struct CvDTreeNode
{
    int class_idx;
    int Tn;
    double value;

    CvDTreeNode* parent;
    CvDTreeNode* left;
    CvDTreeNode* right;

    CvDTreeSplit* split;

    int sample_count;
    int depth;
    int* num_valid;
    int offset;
    int buf_idx;
    double maxlr;

    // global pruning data
    int complexity;
    double alpha;
    double node_risk, tree_risk, tree_error;

    // cross-validation pruning data
    int* cv_Tn;
    double* cv_node_risk;
    double* cv_node_error;

    int get_num_valid(int vi) { return num_valid ? num_valid[vi] : sample_count; }
    void set_num_valid(int vi, int n) { if( num_valid ) num_valid[vi] = n; }
};


struct CvDTreeParams
{
    CV_PROP_RW int   max_categories;
    CV_PROP_RW int   max_depth;
    CV_PROP_RW int   min_sample_count;
    CV_PROP_RW int   cv_folds;
    CV_PROP_RW bool  use_surrogates;
    CV_PROP_RW bool  use_1se_rule;
    CV_PROP_RW bool  truncate_pruned_tree;
    CV_PROP_RW float regression_accuracy;
    const float* priors;

    CvDTreeParams();
    CvDTreeParams( int max_depth, int min_sample_count,
                   float regression_accuracy, bool use_surrogates,
                   int max_categories, int cv_folds,
                   bool use_1se_rule, bool truncate_pruned_tree,
                   const float* priors );
};


struct CvDTreeTrainData
{
    CvDTreeTrainData();
    CvDTreeTrainData( const CvMat* trainData, int tflag,
                      const CvMat* responses, const CvMat* varIdx=0,
                      const CvMat* sampleIdx=0, const CvMat* varType=0,
                      const CvMat* missingDataMask=0,
                      const CvDTreeParams& params=CvDTreeParams(),
                      bool _shared=false, bool _add_labels=false );
    virtual ~CvDTreeTrainData();

    virtual void set_data( const CvMat* trainData, int tflag,
                          const CvMat* responses, const CvMat* varIdx=0,
                          const CvMat* sampleIdx=0, const CvMat* varType=0,
                          const CvMat* missingDataMask=0,
                          const CvDTreeParams& params=CvDTreeParams(),
                          bool _shared=false, bool _add_labels=false,
                          bool _update_data=false );
    virtual void do_responses_copy();

    virtual void get_vectors( const CvMat* _subsample_idx,
         float* values, uchar* missing, float* responses, bool get_class_idx=false );

    virtual CvDTreeNode* subsample_data( const CvMat* _subsample_idx );

    virtual void write_params( cv::FileStorage& fs ) const;
    virtual void read_params( cv::FileStorage& fs, cv::FileNode& node );

    // release all the data
    virtual void clear();

    int get_num_classes() const;
    int get_var_type(int vi) const;
    int get_work_var_count() const {return work_var_count;}

    virtual const float* get_ord_responses( CvDTreeNode* n, float* values_buf, int* sample_indices_buf );
    virtual const int* get_class_labels( CvDTreeNode* n, int* labels_buf );
    virtual const int* get_cv_labels( CvDTreeNode* n, int* labels_buf );
    virtual const int* get_sample_indices( CvDTreeNode* n, int* indices_buf );
    virtual const int* get_cat_var_data( CvDTreeNode* n, int vi, int* cat_values_buf );
    virtual void get_ord_var_data( CvDTreeNode* n, int vi, float* ord_values_buf, int* sorted_indices_buf,
                                   const float** ord_values, const int** sorted_indices, int* sample_indices_buf );
    virtual int get_child_buf_idx( CvDTreeNode* n );

    ////////////////////////////////////

    virtual bool set_params( const CvDTreeParams& params );
    virtual CvDTreeNode* new_node( CvDTreeNode* parent, int count,
                                   int storage_idx, int offset );

    virtual CvDTreeSplit* new_split_ord( int vi, float cmp_val,
                int split_point, int inversed, float quality );
    virtual CvDTreeSplit* new_split_cat( int vi, float quality );
    virtual void free_node_data( CvDTreeNode* node );
    virtual void free_train_data();
    virtual void free_node( CvDTreeNode* node );

    int sample_count, var_all, var_count, max_c_count;
    int ord_var_count, cat_var_count, work_var_count;
    bool have_labels, have_priors;
    bool is_classifier;
    int tflag;

    const CvMat* train_data;
    const CvMat* responses;
    CvMat* responses_copy; // used in Boosting

    int buf_count, buf_size; // buf_size is obsolete, please do not use it, use expression ((int64)buf->rows * (int64)buf->cols / buf_count) instead
    bool shared;
    int is_buf_16u;

    CvMat* cat_count;
    CvMat* cat_ofs;
    CvMat* cat_map;

    CvMat* counts;
    CvMat* buf;
    inline size_t get_length_subbuf() const
    {
        size_t res = (size_t)(work_var_count + 1) * (size_t)sample_count;
        return res;
    }

    CvMat* direction;
    CvMat* split_buf;

    CvMat* var_idx;
    CvMat* var_type; // i-th element =
                     //   k<0  - ordered
                     //   k>=0 - categorical, see k-th element of cat_* arrays
    CvMat* priors;
    CvMat* priors_mult;

    CvDTreeParams params;

    CvMemStorage* tree_storage;
    CvMemStorage* temp_storage;

    CvDTreeNode* data_root;

    CvSet* node_heap;
    CvSet* split_heap;
    CvSet* cv_heap;
    CvSet* nv_heap;

    cv::RNG* rng;
};

class CvDTree;
class CvForestTree;

namespace cv
{
    struct DTreeBestSplitFinder;
    struct ForestTreeBestSplitFinder;
}

class CvDTree : public CvStatModel
{
public:
    CV_WRAP CvDTree();
    virtual ~CvDTree();

    virtual bool train( const CvMat* trainData, int tflag,
                        const CvMat* responses, const CvMat* varIdx=0,
                        const CvMat* sampleIdx=0, const CvMat* varType=0,
                        const CvMat* missingDataMask=0,
                        CvDTreeParams params=CvDTreeParams() );

    virtual bool train( CvMLData* trainData, CvDTreeParams params=CvDTreeParams() );

    // type in {CV_TRAIN_ERROR, CV_TEST_ERROR}
    virtual float calc_error( CvMLData* trainData, int type, std::vector<float> *resp = 0 );

    virtual bool train( CvDTreeTrainData* trainData, const CvMat* subsampleIdx );

    virtual CvDTreeNode* predict( const CvMat* sample, const CvMat* missingDataMask=0,
                                  bool preprocessedInput=false ) const;

    CV_WRAP virtual bool train( const cv::Mat& trainData, int tflag,
                       const cv::Mat& responses, const cv::Mat& varIdx=cv::Mat(),
                       const cv::Mat& sampleIdx=cv::Mat(), const cv::Mat& varType=cv::Mat(),
                       const cv::Mat& missingDataMask=cv::Mat(),
                       CvDTreeParams params=CvDTreeParams() );

    CV_WRAP virtual CvDTreeNode* predict( const cv::Mat& sample, const cv::Mat& missingDataMask=cv::Mat(),
                                  bool preprocessedInput=false ) const;
    CV_WRAP virtual cv::Mat getVarImportance();

    virtual const CvMat* get_var_importance();
    CV_WRAP virtual void clear();

    virtual void read( const cv::FileStorage& fs, const cv::FileNode& node );
    virtual void write( const cv::FileStorage& fs, const char* name ) const;

    // special read & write methods for trees in the tree ensembles
    virtual void read( const cv::FileStorage& fs, const cv::FileNode& node,
                       CvDTreeTrainData* data );
    virtual void write( const cv::FileStorage& fs ) const;

    const CvDTreeNode* get_root() const;
    int get_pruned_tree_idx() const;
    CvDTreeTrainData* get_data();

protected:
    friend struct cv::DTreeBestSplitFinder;

    virtual bool do_train( const CvMat* _subsample_idx );

    virtual void try_split_node( CvDTreeNode* n );
    virtual void split_node_data( CvDTreeNode* n );
    virtual CvDTreeSplit* find_best_split( CvDTreeNode* n );
    virtual CvDTreeSplit* find_split_ord_class( CvDTreeNode* n, int vi,
                            float init_quality = 0, CvDTreeSplit* _split = 0, uchar* ext_buf = 0 );
    virtual CvDTreeSplit* find_split_cat_class( CvDTreeNode* n, int vi,
                            float init_quality = 0, CvDTreeSplit* _split = 0, uchar* ext_buf = 0 );
    virtual CvDTreeSplit* find_split_ord_reg( CvDTreeNode* n, int vi,
                            float init_quality = 0, CvDTreeSplit* _split = 0, uchar* ext_buf = 0 );
    virtual CvDTreeSplit* find_split_cat_reg( CvDTreeNode* n, int vi,
                            float init_quality = 0, CvDTreeSplit* _split = 0, uchar* ext_buf = 0 );
    virtual CvDTreeSplit* find_surrogate_split_ord( CvDTreeNode* n, int vi, uchar* ext_buf = 0 );
    virtual CvDTreeSplit* find_surrogate_split_cat( CvDTreeNode* n, int vi, uchar* ext_buf = 0 );
    virtual double calc_node_dir( CvDTreeNode* node );
    virtual void complete_node_dir( CvDTreeNode* node );
    virtual void cluster_categories( const int* vectors, int vector_count,
        int var_count, int* sums, int k, int* cluster_labels );

    virtual void calc_node_value( CvDTreeNode* node );

    virtual void prune_cv();
    virtual double update_tree_rnc( int T, int fold );
    virtual int cut_tree( int T, int fold, double min_alpha );
    virtual void free_prune_data(bool cut_tree);
    virtual void free_tree();

    virtual void write_node( const cv::FileStorage& fs, CvDTreeNode* node ) const;
    virtual void write_split( const cv::FileStorage& fs, CvDTreeSplit* split ) const;
    virtual CvDTreeNode* read_node( const cv::FileStorage& fs, const cv::FileNode& node, CvDTreeNode* parent );
    virtual CvDTreeSplit* read_split( const cv::FileStorage& fs, const cv::FileNode& node );
    virtual void write_tree_nodes( const cv::FileStorage& fs ) const;
    virtual void read_tree_nodes( const cv::FileStorage& fs, const cv::FileNode& node );

    CvDTreeNode* root;
    CvMat* var_importance;
    CvDTreeTrainData* data;
    CvMat train_data_hdr, responses_hdr;
    cv::Mat train_data_mat, responses_mat;

public:
    int pruned_tree_idx;
};


/****************************************************************************************\
*                                   Boosted tree classifier                              *
\****************************************************************************************/

struct CvBoostParams : public CvDTreeParams
{
    CV_PROP_RW int boost_type;
    CV_PROP_RW int weak_count;
    CV_PROP_RW int split_criteria;
    CV_PROP_RW double weight_trim_rate;

    CvBoostParams();
    CvBoostParams( int boost_type, int weak_count, double weight_trim_rate,
                   int max_depth, bool use_surrogates, const float* priors );
};


class CvBoost;

class CvBoostTree: public CvDTree
{
public:
    CvBoostTree();
    virtual ~CvBoostTree();

    virtual bool train( CvDTreeTrainData* trainData,
                        const CvMat* subsample_idx, CvBoost* ensemble );

    virtual void scale( double s );
    virtual void read( const cv::FileStorage& fs, const cv::FileNode& node,
                       CvBoost* ensemble, CvDTreeTrainData* _data );
    virtual void clear();

    /* dummy methods to avoid warnings: BEGIN */
    virtual bool train( const CvMat* trainData, int tflag,
                        const CvMat* responses, const CvMat* varIdx=0,
                        const CvMat* sampleIdx=0, const CvMat* varType=0,
                        const CvMat* missingDataMask=0,
                        CvDTreeParams params=CvDTreeParams() );
    virtual bool train( CvDTreeTrainData* trainData, const CvMat* _subsample_idx );

    virtual void read( const cv::FileStorage& fs, const cv::FileNode& node );
    virtual void read( const cv::FileStorage& fs, const cv::FileNode& node,
                       CvDTreeTrainData* data );
    /* dummy methods to avoid warnings: END */

protected:

    virtual void try_split_node( CvDTreeNode* n );
    virtual CvDTreeSplit* find_surrogate_split_ord( CvDTreeNode* n, int vi, uchar* ext_buf = 0 );
    virtual CvDTreeSplit* find_surrogate_split_cat( CvDTreeNode* n, int vi, uchar* ext_buf = 0 );
    virtual CvDTreeSplit* find_split_ord_class( CvDTreeNode* n, int vi,
        float init_quality = 0, CvDTreeSplit* _split = 0, uchar* ext_buf = 0 );
    virtual CvDTreeSplit* find_split_cat_class( CvDTreeNode* n, int vi,
        float init_quality = 0, CvDTreeSplit* _split = 0, uchar* ext_buf = 0 );
    virtual CvDTreeSplit* find_split_ord_reg( CvDTreeNode* n, int vi,
        float init_quality = 0, CvDTreeSplit* _split = 0, uchar* ext_buf = 0 );
    virtual CvDTreeSplit* find_split_cat_reg( CvDTreeNode* n, int vi,
        float init_quality = 0, CvDTreeSplit* _split = 0, uchar* ext_buf = 0 );
    virtual void calc_node_value( CvDTreeNode* n );
    virtual double calc_node_dir( CvDTreeNode* n );

    CvBoost* ensemble;
};


class CvBoost : public CvStatModel
{
public:
    // Boosting type
    enum { DISCRETE=0, REAL=1, LOGIT=2, GENTLE=3 };

    // Splitting criteria
    enum { DEFAULT=0, GINI=1, MISCLASS=3, SQERR=4 };

    CV_WRAP CvBoost();
    virtual ~CvBoost();

    CvBoost( const CvMat* trainData, int tflag,
             const CvMat* responses, const CvMat* varIdx=0,
             const CvMat* sampleIdx=0, const CvMat* varType=0,
             const CvMat* missingDataMask=0,
             CvBoostParams params=CvBoostParams() );

    virtual bool train( const CvMat* trainData, int tflag,
             const CvMat* responses, const CvMat* varIdx=0,
             const CvMat* sampleIdx=0, const CvMat* varType=0,
             const CvMat* missingDataMask=0,
             CvBoostParams params=CvBoostParams(),
             bool update=false );

    virtual bool train( CvMLData* data,
             CvBoostParams params=CvBoostParams(),
             bool update=false );

    virtual float predict( const CvMat* sample, const CvMat* missing=0,
                           CvMat* weak_responses=0, CvSlice slice=CV_WHOLE_SEQ,
                           bool raw_mode=false, bool return_sum=false ) const;

    CV_WRAP CvBoost( const cv::Mat& trainData, int tflag,
            const cv::Mat& responses, const cv::Mat& varIdx=cv::Mat(),
            const cv::Mat& sampleIdx=cv::Mat(), const cv::Mat& varType=cv::Mat(),
            const cv::Mat& missingDataMask=cv::Mat(),
            CvBoostParams params=CvBoostParams() );

    CV_WRAP virtual bool train( const cv::Mat& trainData, int tflag,
                       const cv::Mat& responses, const cv::Mat& varIdx=cv::Mat(),
                       const cv::Mat& sampleIdx=cv::Mat(), const cv::Mat& varType=cv::Mat(),
                       const cv::Mat& missingDataMask=cv::Mat(),
                       CvBoostParams params=CvBoostParams(),
                       bool update=false );

    CV_WRAP virtual float predict( const cv::Mat& sample, const cv::Mat& missing=cv::Mat(),
                                   const cv::Range& slice=cv::Range::all(), bool rawMode=false,
                                   bool returnSum=false ) const;

    virtual float calc_error( CvMLData* _data, int type , std::vector<float> *resp = 0 ); // type in {CV_TRAIN_ERROR, CV_TEST_ERROR}

    CV_WRAP virtual void prune( CvSlice slice );

    CV_WRAP virtual void clear();

    virtual void write( const cv::FileStorage& storage, const char* name ) const;
    virtual void read( const cv::FileStorage& storage, const cv::FileNode& node );
    virtual const CvMat* get_active_vars(bool absolute_idx=true);

    CvSeq* get_weak_predictors();

    CvMat* get_weights();
    CvMat* get_subtree_weights();
    CvMat* get_weak_response();
    const CvBoostParams& get_params() const;
    const CvDTreeTrainData* get_data() const;

protected:

    virtual bool set_params( const CvBoostParams& params );
    virtual void update_weights( CvBoostTree* tree );
    virtual void trim_weights();
    virtual void write_params( const cv::FileStorage& fs ) const;
    virtual void read_params( const cv::FileStorage& fs, const cv::FileNode& node );

    virtual void initialize_weights(double (&p)[2]);

    CvDTreeTrainData* data;
    CvMat train_data_hdr, responses_hdr;
    cv::Mat train_data_mat, responses_mat;
    CvBoostParams params;
    CvSeq* weak;

    CvMat* active_vars;
    CvMat* active_vars_abs;
    bool have_active_cat_vars;

    CvMat* orig_response;
    CvMat* sum_response;
    CvMat* weak_eval;
    CvMat* subsample_mask;
    CvMat* weights;
    CvMat* subtree_weights;
    bool have_subsample;
};


/****************************************************************************************\
*                                      Data                                             *
\****************************************************************************************/

#define CV_COUNT     0
#define CV_PORTION   1

struct CvTrainTestSplit
{
    CvTrainTestSplit();
    CvTrainTestSplit( int train_sample_count, bool mix = true);
    CvTrainTestSplit( float train_sample_portion, bool mix = true);

    union
    {
        int count;
        float portion;
    } train_sample_part;
    int train_sample_part_mode;

    bool mix;
};

class CvMLData
{
public:
    CvMLData();
    virtual ~CvMLData();

    // returns:
    // 0 - OK
    // -1 - file can not be opened or is not correct
    int read_csv( const char* filename );

    const CvMat* get_values() const;
    const CvMat* get_responses();
    const CvMat* get_missing() const;

    void set_header_lines_number( int n );
    int get_header_lines_number() const;

    void set_response_idx( int idx ); // old response become predictors, new response_idx = idx
                                      // if idx < 0 there will be no response
    int get_response_idx() const;

    void set_train_test_split( const CvTrainTestSplit * spl );
    const CvMat* get_train_sample_idx() const;
    const CvMat* get_test_sample_idx() const;
    void mix_train_and_test_idx();

    const CvMat* get_var_idx();
    void chahge_var_idx( int vi, bool state ); // misspelled (saved for back compitability),
                                               // use change_var_idx
    void change_var_idx( int vi, bool state ); // state == true to set vi-variable as predictor

    const CvMat* get_var_types();
    int get_var_type( int var_idx ) const;
    // following 2 methods enable to change vars type
    // use these methods to assign CV_VAR_CATEGORICAL type for categorical variable
    // with numerical labels; in the other cases var types are correctly determined automatically
    void set_var_types( const char* str );  // str examples:
                                            // "ord[0-17],cat[18]", "ord[0,2,4,10-12], cat[1,3,5-9,13,14]",
                                            // "cat", "ord" (all vars are categorical/ordered)
    void change_var_type( int var_idx, int type); // type in { CV_VAR_ORDERED, CV_VAR_CATEGORICAL }

    void set_delimiter( char ch );
    char get_delimiter() const;

    void set_miss_ch( char ch );
    char get_miss_ch() const;

    const std::map<cv::String, int>& get_class_labels_map() const;

protected:
    virtual void clear();

    void str_to_flt_elem( const char* token, float& flt_elem, int& type);
    void free_train_test_idx();

    char delimiter;
    char miss_ch;
    //char flt_separator;

    CvMat* values;
    CvMat* missing;
    CvMat* var_types;
    CvMat* var_idx_mask;

    CvMat* response_out; // header
    CvMat* var_idx_out; // mat
    CvMat* var_types_out; // mat

    int header_lines_number;

    int response_idx;

    int train_sample_count;
    bool mix;

    int total_class_count;
    std::map<cv::String, int> class_map;

    CvMat* train_sample_idx;
    CvMat* test_sample_idx;
    int* sample_idx; // data of train_sample_idx and test_sample_idx

    cv::RNG* rng;
};


namespace cv
{

typedef CvStatModel StatModel;
typedef CvDTreeParams DTreeParams;
typedef CvMLData TrainData;
typedef CvDTree DecisionTree;
typedef CvForestTree ForestTree;
typedef CvBoostParams BoostParams;
typedef CvBoostTree BoostTree;
typedef CvBoost Boost;

template<> struct DefaultDeleter<CvDTreeSplit>{ void operator ()(CvDTreeSplit* obj) const; };

}

#endif // __cplusplus
#endif // OPENCV_OLD_ML_HPP

/* End of file. */
