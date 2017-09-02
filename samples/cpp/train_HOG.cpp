
#include "opencv2/imgproc.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/ml.hpp"
#include "opencv2/objdetect.hpp"

#include <iostream>
#include <time.h>

using namespace cv;
using namespace cv::ml;
using namespace std;

void get_svm_detector(const Ptr<SVM>& svm, vector< float > & hog_detector );
void convert_to_ml(const std::vector< cv::Mat > & train_samples, cv::Mat& trainData );
void load_images( const string & prefix, vector< Mat > & img_lst, bool showImages);
void sample_neg( const vector< Mat > & full_neg_lst, vector< Mat > & neg_lst, const Size & size );
Mat get_hogdescriptor_visu(const Mat& color_origImg, vector<float>& descriptorValues, const Size & size );
void compute_hog( const vector< Mat > & img_lst, vector< Mat > & gradient_lst );
void train_svm( const vector< Mat > & gradient_lst, const vector< int > & labels );
int test_it( const Size & size );
String test_dir,filename;
bool visualize;

void get_svm_detector(const Ptr<SVM>& svm, vector< float > & hog_detector )
{
    // get the support vectors
    Mat sv = svm->getSupportVectors();
    const int sv_total = sv.rows;
    // get the decision function
    Mat alpha, svidx;
    double rho = svm->getDecisionFunction(0, alpha, svidx);

    CV_Assert( alpha.total() == 1 && svidx.total() == 1 && sv_total == 1 );
    CV_Assert( (alpha.type() == CV_64F && alpha.at<double>(0) == 1.) ||
               (alpha.type() == CV_32F && alpha.at<float>(0) == 1.f) );
    CV_Assert( sv.type() == CV_32F );
    hog_detector.clear();

    hog_detector.resize(sv.cols + 1);
    memcpy(&hog_detector[0], sv.ptr(), sv.cols*sizeof(hog_detector[0]));
    hog_detector[sv.cols] = (float)-rho;
}

/*
* Convert training/testing set to be used by OpenCV Machine Learning algorithms.
* TrainData is a matrix of size (#samples x max(#cols,#rows) per samples), in 32FC1.
* Transposition of samples are made if needed.
*/
void convert_to_ml(const std::vector< cv::Mat > & train_samples, cv::Mat& trainData )
{
    //--Convert data
    const int rows = (int)train_samples.size();
    const int cols = (int)std::max( train_samples[0].cols, train_samples[0].rows );
    cv::Mat tmp(1, cols, CV_32FC1); //< used for transposition if needed
    trainData = cv::Mat(rows, cols, CV_32FC1 );
    vector< Mat >::const_iterator itr = train_samples.begin();
    vector< Mat >::const_iterator end = train_samples.end();
    for( int i = 0 ; itr != end ; ++itr, ++i )
    {
        CV_Assert( itr->cols == 1 || itr->rows == 1 );

        if( itr->cols == 1 )
        {
            transpose( *(itr), tmp );
            tmp.copyTo( trainData.row( i ) );
        }
        else if( itr->rows == 1 )
        {
            itr->copyTo( trainData.row( i ) );
        }
    }
}

void load_images( const string & prefix, vector< Mat > & img_lst, bool showImages = true )
{
    vector<String> files;
    glob(prefix, files);

    for (size_t i = 0; i < files.size(); ++i)
    {

        Mat img = imread(files[i]); // load the image
        if (img.empty()) // invalid image, just skip it.
        {
            cout << files[i] << " is invalid!" << endl;
            continue;
        }

        if (visualize & showImages)
        {
            imshow("image", img);
            waitKey(10);
        }
        img_lst.push_back(img);
    }
}

void sample_neg( const vector< Mat > & full_neg_lst, vector< Mat > & neg_lst, const Size & size )
{
    Rect box;
    box.width = size.width;
    box.height = size.height;

    const int size_x = box.width;
    const int size_y = box.height;

    srand( (unsigned int)time( NULL ) );

    vector< Mat >::const_iterator img = full_neg_lst.begin();
    vector< Mat >::const_iterator end = full_neg_lst.end();
    for( ; img != end ; ++img )
    {
        box.x = rand() % (img->cols - size_x);
        box.y = rand() % (img->rows - size_y);
        Mat roi = (*img)(box);
        neg_lst.push_back( roi.clone() );
    }
}

// From http://www.juergenwiki.de/work/wiki/doku.php?id=public:hog_descriptor_computation_and_visualization
Mat get_hogdescriptor_visu(const Mat& color_origImg, vector<float>& descriptorValues, const Size & size )
{
    const int DIMX = size.width;
    const int DIMY = size.height;
    float zoomFac = 3;
    Mat visu;
    resize(color_origImg, visu, Size( (int)(color_origImg.cols*zoomFac), (int)(color_origImg.rows*zoomFac) ) );

    int cellSize        = 8;
    int gradientBinSize = 9;
    float radRangeForOneBin = (float)(CV_PI/(float)gradientBinSize); // dividing 180 into 9 bins, how large (in rad) is one bin?

    // prepare data structure: 9 orientation / gradient strenghts for each cell
    int cells_in_x_dir = DIMX / cellSize;
    int cells_in_y_dir = DIMY / cellSize;
    float*** gradientStrengths = new float**[cells_in_y_dir];
    int** cellUpdateCounter   = new int*[cells_in_y_dir];
    for (int y=0; y<cells_in_y_dir; y++)
    {
        gradientStrengths[y] = new float*[cells_in_x_dir];
        cellUpdateCounter[y] = new int[cells_in_x_dir];
        for (int x=0; x<cells_in_x_dir; x++)
        {
            gradientStrengths[y][x] = new float[gradientBinSize];
            cellUpdateCounter[y][x] = 0;

            for (int bin=0; bin<gradientBinSize; bin++)
                gradientStrengths[y][x][bin] = 0.0;
        }
    }

    // nr of blocks = nr of cells - 1
    // since there is a new block on each cell (overlapping blocks!) but the last one
    int blocks_in_x_dir = cells_in_x_dir - 1;
    int blocks_in_y_dir = cells_in_y_dir - 1;

    // compute gradient strengths per cell
    int descriptorDataIdx = 0;
    int cellx = 0;
    int celly = 0;

    for (int blockx=0; blockx<blocks_in_x_dir; blockx++)
    {
        for (int blocky=0; blocky<blocks_in_y_dir; blocky++)
        {
            // 4 cells per block ...
            for (int cellNr=0; cellNr<4; cellNr++)
            {
                // compute corresponding cell nr
                cellx = blockx;
                celly = blocky;
                if (cellNr==1) celly++;
                if (cellNr==2) cellx++;
                if (cellNr==3)
                {
                    cellx++;
                    celly++;
                }

                for (int bin=0; bin<gradientBinSize; bin++)
                {
                    float gradientStrength = descriptorValues[ descriptorDataIdx ];
                    descriptorDataIdx++;

                    gradientStrengths[celly][cellx][bin] += gradientStrength;

                } // for (all bins)


                // note: overlapping blocks lead to multiple updates of this sum!
                // we therefore keep track how often a cell was updated,
                // to compute average gradient strengths
                cellUpdateCounter[celly][cellx]++;

            } // for (all cells)


        } // for (all block x pos)
    } // for (all block y pos)


    // compute average gradient strengths
    for (celly=0; celly<cells_in_y_dir; celly++)
    {
        for (cellx=0; cellx<cells_in_x_dir; cellx++)
        {

            float NrUpdatesForThisCell = (float)cellUpdateCounter[celly][cellx];

            // compute average gradient strenghts for each gradient bin direction
            for (int bin=0; bin<gradientBinSize; bin++)
            {
                gradientStrengths[celly][cellx][bin] /= NrUpdatesForThisCell;
            }
        }
    }

    // draw cells
    for (celly=0; celly<cells_in_y_dir; celly++)
    {
        for (cellx=0; cellx<cells_in_x_dir; cellx++)
        {
            int drawX = cellx * cellSize;
            int drawY = celly * cellSize;

            int mx = drawX + cellSize/2;
            int my = drawY + cellSize/2;

            rectangle(visu, Point((int)(drawX*zoomFac), (int)(drawY*zoomFac)), Point((int)((drawX+cellSize)*zoomFac), (int)((drawY+cellSize)*zoomFac)), Scalar(100,100,100), 1);

            // draw in each cell all 9 gradient strengths
            for (int bin=0; bin<gradientBinSize; bin++)
            {
                float currentGradStrength = gradientStrengths[celly][cellx][bin];

                // no line to draw?
                if (currentGradStrength==0)
                    continue;

                float currRad = bin * radRangeForOneBin + radRangeForOneBin/2;

                float dirVecX = cos( currRad );
                float dirVecY = sin( currRad );
                float maxVecLen = (float)(cellSize/2.f);
                float scale = 2.5; // just a visualization scale, to see the lines better

                // compute line coordinates
                float x1 = mx - dirVecX * currentGradStrength * maxVecLen * scale;
                float y1 = my - dirVecY * currentGradStrength * maxVecLen * scale;
                float x2 = mx + dirVecX * currentGradStrength * maxVecLen * scale;
                float y2 = my + dirVecY * currentGradStrength * maxVecLen * scale;

                // draw gradient visualization
                line(visu, Point((int)(x1*zoomFac),(int)(y1*zoomFac)), Point((int)(x2*zoomFac),(int)(y2*zoomFac)), Scalar(0,255,0), 1);

            } // for (all bins)

        } // for (cellx)
    } // for (celly)


    // don't forget to free memory allocated by helper data structures!
    for (int y=0; y<cells_in_y_dir; y++)
    {
        for (int x=0; x<cells_in_x_dir; x++)
        {
            delete[] gradientStrengths[y][x];
        }
        delete[] gradientStrengths[y];
        delete[] cellUpdateCounter[y];
    }
    delete[] gradientStrengths;
    delete[] cellUpdateCounter;

    return visu;

} // get_hogdescriptor_visu

void compute_hog( const vector< Mat > & img_lst, vector< Mat > & gradient_lst )
{
    HOGDescriptor hog;
    Rect r = Rect(0, 0, (img_lst[0].cols / 8) * 8, (img_lst[0].rows/8)*8);
    r.x += (img_lst[0].cols - r.width) / 2;
    r.y += (img_lst[0].rows - r.height) / 2;
    hog.winSize = r.size();
    Mat gray;
    vector< Point > location;
    vector< float > descriptors;

    for(int i=0 ; i< img_lst.size(); i++ )
    {
        cvtColor(img_lst[i](r), gray, COLOR_BGR2GRAY );
        hog.compute( gray, descriptors, Size( 8, 8 ), Size( 0, 0 ), location );
        gradient_lst.push_back( Mat( descriptors ).clone() );
        if (visualize)
        {
            imshow("gradient", get_hogdescriptor_visu(img_lst[i](r), descriptors,r.size()));
            waitKey(10);
        }
    }
}

void train_svm( const vector< Mat > & gradient_lst, const vector< int > & labels )
{
    Mat train_data;
    convert_to_ml( gradient_lst, train_data );

    clog << "Training SVM...";
    Ptr<SVM> svm = SVM::create();
    /* Default values to train SVM */
    svm->setCoef0(0.0);
    svm->setDegree(3);
    svm->setTermCriteria(TermCriteria( CV_TERMCRIT_ITER+CV_TERMCRIT_EPS, 1000, 1e-3 ));
    svm->setGamma(0);
    svm->setKernel(SVM::LINEAR);
    svm->setNu(0.5);
    svm->setP(0.1); // for EPSILON_SVR, epsilon in loss function?
    svm->setC(0.01); // From paper, soft classifier
    svm->setType(SVM::EPS_SVR); // C_SVC; // EPSILON_SVR; // may be also NU_SVR; // do regression task
    svm->train(train_data, ROW_SAMPLE, Mat(labels));
    clog << "...[done]" << endl;

    svm->save( filename );
}

int test_it( const Size & size )
{
    cout << "Testing trained detector..." << endl;
    Ptr<SVM> svm;
    HOGDescriptor my_hog;
    my_hog.winSize = size;
    vector< Rect > locations;

    // Load the trained SVM.
    svm = StatModel::load<SVM>( filename );
    // Set the trained svm to my_hog
    vector< float > hog_detector;
    get_svm_detector( svm, hog_detector );
    my_hog.setSVMDetector( hog_detector );

    vector<String> files;
    glob(test_dir, files);

    for( int i=0; i<files.size(); i++)
    {
        Mat img = imread(files[i]);
        vector<Rect> detections;
        vector<double> foundWeights;

        my_hog.detectMultiScale(img, detections, foundWeights);
        for (int j = 0; j < detections.size(); j++)
        {
            cout << foundWeights[j]<< endl;
            Scalar color = Scalar(0, foundWeights[j]* foundWeights[j]*200, 0);
            rectangle(img, detections[j], color, 2);
        }

        imshow( "detections", img);
        if( 27 == waitKey(0))
            return 0;
    }
    return 0;
}

int main( int argc, char** argv )
{
    cv::CommandLineParser parser(argc, argv, "{help h|| show help message}"
                                 "{pd||pos_dir}{nd||neg_dir}{td || test dir} {fn || file name}{v |false| visualization}");
    if (parser.has("help"))
    {
        parser.printMessage();
        exit(0);
    }
    vector< Mat > pos_lst, full_neg_lst, neg_lst, gradient_lst;
    vector< int > labels;
    string pos_dir = parser.get<string>("pd");
    string neg_dir = parser.get<string>("nd");
    test_dir = parser.get<string>("td");
    filename = parser.get<string>("fn");
    visualize = parser.get<bool>("v");
    if( pos_dir.empty() || neg_dir.empty() )
    {
        parser.printMessage();
        cout << "Wrong number of parameters." << endl
             << "example: " << argv[0] << " --pd=./INRIA_dataset_pos/ --nd=./INRIA_dataset_neg/" << endl;
        exit( -1 );
    }
    clog << "Positive images are being loaded..." ;
    load_images( pos_dir, pos_lst,false );
    clog << "...[done]" << endl;
    Size pos_image_size = pos_lst[0].size();
    for (size_t i = 0; i < pos_lst.size(); ++i)
    {
        if( pos_lst[i].size() != pos_image_size)
        {
            cout << "All positive images should be same size!" << endl;
            exit( 1 );
        }
    }
    labels.assign( pos_lst.size(), +1 );
    const unsigned int old = (unsigned int)labels.size();
    clog << "Negative images are being loaded...";
    load_images( neg_dir, full_neg_lst,false );
    sample_neg( full_neg_lst, neg_lst, pos_image_size );
    clog << "...[done]" << endl;

    labels.insert( labels.end(), neg_lst.size(), -1 );
    CV_Assert( old < labels.size() );

    clog << "Histogram of Gradients are being calculated for positive images...";
    compute_hog( pos_lst, gradient_lst );
    clog << "...[done]" << endl;

    clog << "Histogram of Gradients are being calculated for negative images...";
    compute_hog( neg_lst, gradient_lst );
    clog << "...[done]" << endl;

    train_svm( gradient_lst, labels );

    pos_image_size.height = pos_image_size.height / 8 * 8;
    pos_image_size.width = pos_image_size.width / 8 * 8;
    test_it( pos_image_size );

    return 0;
}
