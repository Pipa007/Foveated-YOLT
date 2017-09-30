#!/bin/bash

#if [ -z "$1" ] 
#  then
#    echo "No folder supplied!"
#    echo "Usage: bash `basename "$0"` mot_video_folder"
#    exit
#fi

DIR=$PWD; # Foveated_YOLT

# Choose set mode for caffe  [CPU | GPU]
SET_MODE=GPU

# Choose which GPU the detector runs on
GPU_ID=0

# Choose number of top prediction that you want
TOP=5

# Size of the images received by the network
SIZE_MAP=227

# Define number of kernel levels
LEVELS=5

# Define size of the fovea
#SIGMAs={10,20,30,40,50,60,70,80,90,100,110,120,130,140}
#SIGMAS="1,10,20,30,40,50,60,70,80,90,100"
#SIGMAS="1.8923,1.7726,1.5982,1.3808,1.2351,1.1304,1.0508" #cartesian
#SIGMAS="5.0000,5.2578,5.2943,5.9613,5.9774,6.9499,6.9757,8.0560,8.4695,9.1877,10.3100,11.4139,12.4997,13.5701,14.6277,15.6752,16.7145,17.7473,18.7748,19.7979,20.8174,21.8338,22.8475,23.8585,24.8658,25.8715,26.8824,27.8892,28.8930,29.9085" 
#SIGMAS="5.0000,4.7756,4.7415,4.1880,4.1762,3.5588,3.5446,3.0323,2.8694,2.6197,2.2953,2.0339,1.8176,1.6342,1.4752,1.3349,1.2091,1.0944,0.9885,0.8891,0.7946,0.7030,0.6122,0.5176,0.4090,0.3012,0.2394,0.1998,0.1693,0.1447"
SIGMAS=30
#foveal
# Choose segmentation threshold
THRESHOLDS="0.0,0.05,0.1,0.15,0.2,0.25,0.3,0.35,0.4,0.45,0.5,0.55,0.6,0.65,0.7,0.75,0.80,0.85,0.90,0.95,1.0"
#THRESHOLDS="0.75"

# Choose method (1-CARTESIAN 2-FOVEATION 3-HYBRID)
MODE=2

# Reclassify (2nd passage)
RECLASSIFY=0

# change this path to the absolute location of the network related files
FILES_FOLDER_ABSOLUTE_PATH=$PWD"/files/"
MODEL_FILE="deploy_alexnet.prototxt"
WEIGHTS_FILE="bvlc_alexnet.caffemodel"
MEAN_FILE="imagenet_mean.binaryproto"
LABELS_FILE="synset_words_change.txt"
DATASET="/media/Data/filipa/ILSVRC2012_img_test"
#DATASET="/home/rui/ILSVR2007"
#GROUND_TRUTH_LABELS=$FILES_FOLDER_ABSOLUTE_PATH"ground_truth_labels_ilsvrc12.txt"
RESULTS_FOLDER_ABSOLUTE_PATH=$PWD"/results/"
DEBUG=1
TOTAL_IMAGES=100

build/yolt $FILES_FOLDER_ABSOLUTE_PATH $MODEL_FILE $WEIGHTS_FILE $MEAN_FILE $LABELS_FILE  $DATASET $TOP $THRESHOLDS $SIZE_MAP $LEVELS $SIGMAS $RESULTS_FOLDER_ABSOLUTE_PATH $MODE $RECLASSIFY $DEBUG $TOTAL_IMAGES $SET_MODE $GPU_ID




