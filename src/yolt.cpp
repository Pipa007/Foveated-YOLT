#include <caffe/caffe.hpp>
#include <caffe/util/io.hpp>
#include <caffe/blob.hpp>
#include <caffe/layer.hpp>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include <vector>
#include <memory>
#include <math.h>
#include <limits>
#include <boost/algorithm/string.hpp>

#include "network_classes.hpp"
#include "laplacian_foveation.hpp"

const int CARTESIAN=1;
const int FOVEATION=2;
const int HYBRID=3;

using namespace caffe;
using namespace std;

using std::string;


/* Pair (label, confidence) representing a prediction. */
typedef std::pair<string, float> Prediction;


cv::Mat foveate(const cv::Mat & img, const int & size_map, const int & levels, const double & sigma, const cv::Mat & fixation_point)
{
    // Foveate images
    int m = floor(4*img.size().height);
    int n = floor(4*img.size().width);

    cv::Mat image;
    img.convertTo(image, CV_64F);

    // Compute kernels
    std::vector<Mat> kernels = createFilterPyr(m, n, levels, sigma);

    // Construct Pyramid
    LaplacianBlending pyramid(image,levels, kernels);

    // Foveate
    cv::Mat foveated_image = pyramid.foveate(fixation_point);

    foveated_image.convertTo(foveated_image,CV_8UC3);
    cv::resize(foveated_image,foveated_image,Size(size_map,size_map));

    return foveated_image;
}

/******************/
//		MAIN
/******************/

int main(int argc, char** argv) {

    // Init
    ::google::InitGoogleLogging(argv[0]);

    /* network params */
    const string absolute_path_folder = string(argv[1]);
    std::cout << "absolute_path_folder: " << absolute_path_folder<< std::endl;

    const string model_file = absolute_path_folder + string(argv[2]);
    std::cout << "model_file: " << model_file<< std::endl;

    const string weight_file = absolute_path_folder + string(argv[3]);
    std::cout << "weight_file: " << weight_file<< std::endl;

    const string mean_file = absolute_path_folder + string(argv[4]);
    std::cout << "mean_file: " << mean_file<< std::endl;

    const string label_file = absolute_path_folder + string(argv[5]);
    std::cout << "label_file: " << label_file<< std::endl;

    const string dataset_folder = string(argv[6]);
    std::cout << "dataset_folder: " << dataset_folder<< std::endl;

    /* method specific params */
    static int N = atoi(argv[7]);         // define number of top predicted labels
    std::cout << "top classes: " << N<< std::endl;

    static string threshs_ = string(argv[8]);  // segmentation threshold for mask
    std::cout << "threshs: " << threshs_<< std::endl;

    std::vector<float> threshs;

    std::stringstream ss_(threshs_);


    float j;

    while (ss_ >> j)
    {
        threshs.push_back(j);

        if (ss_.peek() == ',')
            ss_.ignore();
    }

    static int size_map = atoi(argv[9]);  // Size of the network input images (227,227)
    std::cout << "size_map: " << size_map<< std::endl;

    static int levels = atoi(argv[10]);   // Levels
    std::cout << "levels: " << levels<< std::endl;

    static string sigmas_ = string(argv[11]);           // Sigmas
    std::cout << "sigmas: " << sigmas_<< std::endl;

    std::vector<double> sigmas;

    std::stringstream ss(sigmas_);

    double i;

    while (ss >> i)
    {
        sigmas.push_back(i);

        if (ss.peek() == ',')
            ss.ignore();
    }


    static string results_folder = string(argv[12]);    // Results folder
    std::cout << "results_folder: " << results_folder<< std::endl;

    static int mode = atoi(argv[13]);    // Mode
    std::cout << "mode: " << mode << std::endl;

    static int reclassify = atoi(argv[14]);
    std::cout << "reclassify: " << reclassify << std::endl;

    static bool debug = atoi(argv[15]);    // Debug
    std::cout << "debug: " << debug << std::endl;

    static int total_images = atoi(argv[16]);    // Total images
    std::cout << "total_images: " << total_images << std::endl;

    // Set mode
    if (strcmp(argv[17], "CPU") == 0)
    {
        Caffe::set_mode(Caffe::CPU);
    }
    else
    {
        Caffe::set_mode(Caffe::GPU);
        int device_id = atoi(argv[18]);
        Caffe::SetDevice(device_id);
    }


    // Load network, pre-processment, set mean and load labels
    Network Network(model_file, weight_file, mean_file, label_file);

    /**********************************************************************/
    //              LOAD LIST OF IMAGES AND BBOX OF DIRECTORY             //
    /**********************************************************************/

    std::vector<cv::String> image_image_files ;

    image_image_files = Network.GetDir (dataset_folder, image_image_files);

    if(total_images>image_image_files.size())
        total_images=image_image_files.size();
    glob(dataset_folder, image_image_files);

    ofstream feedforward_detection;
    ofstream feedback_detection;

    // store results
    std::string feedforward_detection_str=results_folder+string("feedforward_detection_parse.txt");
    feedforward_detection.open (feedforward_detection_str.c_str(),ios::out);          // file with 5 classes + scores + 5 bounding boxes
    feedforward_detection<<"sigma;thres;class1;score1;x1;y1;w1;h1;class2;score2;x2;y2;w2;h2;class3;score3;x3;y3;w3;h3;class4;score4;x4;y4;w4;h4;class5;score5;x5;y5;w5;h5"<<std::endl;

    if(reclassify)
    {
        std::string feedback_detection_str=results_folder+string("feedback_detection_parse.txt");
        feedback_detection.open (feedback_detection_str.c_str(), ios::out);  // file with 25 predicted classes for each image
        feedback_detection<<"sigma;thres;class1;score1;class2;score2;class3;score3;class4;score4;class5;score5;class6;score6;class7;score7;class8;score8;class9;score9;class10;score10;class11;score11;class12;score12;class13;score13;class14;score14;class15;score15;class16;score16;class17;score17;class18;score18;class19;score19;class20;score20;class21;score21;class22;score22;class23;score23;class24;score24;class25;score25"<<std::endl;
    }
    for (unsigned int thresh_index = 0;thresh_index < threshs.size(); ++thresh_index){

        float thresh=threshs[thresh_index];

        for (unsigned int sigma_index = 0;sigma_index < sigmas.size(); ++sigma_index){

            double sigma=sigmas[sigma_index];

            // FOR EACH IMAGE OF THE DATASET (TODO: OPTIMIZATION -> PROCESS BATCH OF IMAGES INSTEAD OF SINGLE IMAGES)
            for (unsigned int input = 0;input < total_images; ++input){
                std::cout << "thresh:" << thresh << " sigma:" << sigma <<" Procesing image " << input+1 << " of " << total_images << ": iteration "<<input+1 + sigma_index*total_images+thresh_index*total_images*sigmas.size() << " of a total of "  << total_images*threshs.size()*sigmas.size() << " iterations" <<"("<< 100.0*(input+1 + sigma_index*total_images+thresh_index*total_images*sigmas.size())/ (total_images*threshs.size()*sigmas.size()) << "%)"<< std::endl;

                string file = image_image_files[input];
                std::vector<string> new_labels;
                std::vector<float> new_scores;
                std::vector<Rect> bboxes;

                cv::Mat img = cv::imread(file, 1);		  // Read image
                resize(img,img, Size(size_map,size_map)); // Resize to network size
                cv::Mat img_orig=img.clone();

                ClassData mydata(N);

                if(mode==FOVEATION)
                {
                    cv::Mat fixation_point(2,1,CV_32S);



                    if(reclassify)
                    {
                        // Random
                        fixation_point.at<int>(0,0) = img.size().width * (rand() / (RAND_MAX + 1.0));
                        fixation_point.at<int>(1,0) = img.size().height * (rand() / (RAND_MAX + 1.0));
                    }
                    else
                    {
                        // Center
                        fixation_point.at<int>(0,0) = img.size().width*0.5;
                        fixation_point.at<int>(1,0) = img.size().height*0.5;
                    }


                    img=foveate(img,size_map,levels,sigma,fixation_point);
                }
                //else
               // {
               //     GaussianBlur(img,img, Size(5,5), sigma, sigma);
               // }

                cv::Mat img_first_pass_viz=img.clone();


                // FEEDFORWAD - PREDICT CLASSES (TOP N)
                mydata = Network.Classify(img, N);

                // store results
                feedforward_detection << std::fixed << std::setprecision(4) << sigma << ";" << thresh << ";";

                if(reclassify)
                {
                    feedback_detection <<  std::fixed << std::setprecision(4) << sigma << ";" << thresh << ";";
                }

                // For each predicted class label:
                for (int i = 0; i < N; ++i) {
                    cv::Mat img_second;

                    //////////////////////////////////////////////////
                    //  Weakly Supervised Object Localization  //
                    // Saliency Map + Segmentation Mask + BBox //
                    //////////////////////////////////////////////////
                    Rect Min_Rect = Network.CalcBBox(N, i,img, mydata, thresh);
                    bboxes.push_back(Min_Rect); // save all bounding boxes

                    // store results
                    if (i==N-1)
                        feedforward_detection << mydata.label[i] << ";" << mydata.score[i] << ";" << Min_Rect.x << ";" << Min_Rect.y << ";" << Min_Rect.width << ";" << Min_Rect.height;
                    else
                        feedforward_detection << mydata.label[i] << ";" << mydata.score[i] << ";" << Min_Rect.x << ";" << Min_Rect.y << ";" << Min_Rect.width << ";" << Min_Rect.height << ";";


                    /////////////////////////////////////////////////////////
                    //      Image Re-Classification with Attention         //
                    // Foveated Image + Forward + Predict new class labels //
                    /////////////////////////////////////////////////////////
                    if(reclassify)
                    {
                        if(mode==FOVEATION||mode==HYBRID)
                        {
                            cv::Mat fixation_point(2,1,CV_32S);
                            fixation_point.at<int>(0,0) = Min_Rect.y + Min_Rect.height/2;
                            fixation_point.at<int>(1,0) = Min_Rect.x + Min_Rect.width/2;
                            img_second=foveate(img_orig,size_map,levels,sigma,fixation_point);
                        }
                        else
                        {
                            img_second = img_orig(Min_Rect);  // crop image by bbox

                            if (img_second.size().width != 0 && img_second.size().height != 0 )
                            {
                                cv::resize(img_second,img_second,Size(size_map,size_map));
                            }
                            else
                            {
                                img_first_pass_viz.copyTo(img_second);
                            }
                        }

                        if(debug)
                        {
                            Mat dst;
                            cv::hconcat(img_orig,img_first_pass_viz, dst); // horizontal
                            cv::hconcat(dst, img_second, dst); // horizontal
                            //cv::vconcat(a, b, dst); // vertical
                            namedWindow( "original image,    first pass,   second pass     class", WINDOW_AUTOSIZE ); // Create a window for display.
                            imshow( "original image,    first pass,   second pass     class", dst );                  // Show our image inside it.
                            waitKey(1);
                        }

                        // Forward

                        // Predict New top 5 of each predicted class
                        ClassData feedback_data = Network.Classify(img_second, N);

                        // For each bounding box
                        for(int m=0; m<N; ++m){
                            new_labels.push_back(feedback_data.label[m]);
                            new_scores.push_back(feedback_data.score[m]);
                        }
                    }
                    else {

                        if(debug)
                        {
                            Mat dst;
                            cv::hconcat(img_orig,img_first_pass_viz, dst); // horizontal
                            //cv::vconcat(a, b, dst); // vertical
                            namedWindow( "original image,    first pass", WINDOW_AUTOSIZE ); // Create a window for display.
                            imshow( "original image,    first pass", dst );                  // Show our image inside it.
                            waitKey(1);
                        }

                    }

                }
                feedforward_detection << endl;

                if(reclassify)
                {
                    // Feedback results
                    for (int aux=0; aux<N*N; ++aux){
                        if (aux==N*N-1)
                            feedback_detection <<  new_labels[aux] << ";" << new_scores[aux];
                        else
                            feedback_detection <<  new_labels[aux] << ";" << new_scores[aux] << ";";
                    }
                    feedback_detection << endl;
                }

            }

        }
    }
    feedback_detection.close();


    /*********************************************************/
    //               Rank Top 5 final solution               //
    //      From 25 predicted labels, find highest 5         //
    /*********************************************************/

    //for (int k=0; k<new_labels.size(); ++k)
    //    cout << "New Prediction: " << new_labels[k] << "\t\t" << new_scores[k] << endl;

    //        std::vector<string> top_final_labels;
    //        std::vector<float> top_final_scores;


    //        std::vector<int> topN = Argmax(new_scores, N);

    //        for (int top = 0; top <N; ++top){
    //            int idx = topN[top];

    //            top_final_labels.push_back(new_labels[idx]);
    //            top_final_scores.push_back(new_scores[idx]);
    //        }

    //        cout << "Final Solution:\n " << endl;
    //        for (int top = 0; top <N; ++top)
    //            cout << "Score: " << top_final_scores[top]  << "\t Label: " << top_final_labels[top] << endl;


    // Check if predicted labels = ground truth labels - YOLT
    //        if (strstr(top_final_labels[0].c_str(), ground_class.c_str())){
    //            counter_top1_yolt-=1; // acertou a primeira na class
    //        }
    //        for (int k=0; k<N; ++k){
    //            if (strstr(top_final_labels[k].c_str(), ground_class.c_str()))
    //                counter_top5_yolt-=1; // class verdadeira esta no top
    //                break;
    //        }


    //        /***************************************************************/
    //        //                  LOCALIZATION ERROR                         //
    //        /***************************************************************/

    //        xpath_node_set get_width = doc.select_nodes("/annotation/size/width");
    //        xpath_node_set get_height = doc.select_nodes("/annotation/size/height");

    //        for (xpath_node_set::const_iterator it = get_width.begin(); it!= get_width.end(); ++it){
    //            xpath_node node_width = *it;
    //            cout << "name " << node_width.node().name() << endl;
    //            cout << "Width " << node_width.node().child_value() << endl;
    //            string img_width = node_width.node().child_value();
    //        }
    //        for (xpath_node_set::const_iterator it = get_height.begin(); it!= get_height.end(); ++it){
    //            xpath_node node_height = *it;
    //            cout << "name " << node_height.node().name() << endl;
    //            cout << "Height " << node_height.node().child_value() << endl;
    //            string img_height = node_height.node().child_value();
    //        }


    //   }

    //ground_truth_file.close();




    /*********************************************************/
    //           WRITE OUTPUT FILE WITH THE RESULTS          //
    /*********************************************************/

    //    // Write data to output file
    //    ofstream output_file;
    //    output_file.open ("final_results.txt",ios::app);  //appending the content to the current content of the file.
    //    output_file << std::fixed << std::setprecision(4) ;
    //    output_file << sigma << " " << thresh << " " << double(counter_top1_yolo)/double(image_files.size());
    //    output_file << " " << double(counter_top5_yolo)/double(image_files.size());
    //    output_file <<" " << double(counter_top1_yolt)/double(image_files.size()) << " " << double(counter_top5_yolt)/double(image_files.size()) << endl;
    //    output_file.close();

    //    feedforward_detection.open("raw_bbox.txt",ios::app);
    //    feedforward_detection << "\n" ;
    //    feedforward_detection.close();

}


