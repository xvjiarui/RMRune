#pragma once

#include "tiny_dnn/tiny_dnn.h"
#include <opencv2/opencv.hpp>

using namespace tiny_dnn;
using namespace tiny_dnn::activation;
using namespace std;
using namespace cv;

class MnistRecognizer
{
public:
	MnistRecognizer(const string& dictionary);
	void predict(const Mat& img);
	~MnistRecognizer();	
	vector<Mat>* mnistImgs;
	vector<Point>* mnistCenters;
	vector<int>* mnistLabels;
private:
	int recognize(const Mat& img);
	void preprocess();
	Mat imgCopy;
	network<sequential> nn;

	int offset_x;
	int offset_y;
	int thresh;

	RNG rng; // random number generator
	string Model_Path;

};