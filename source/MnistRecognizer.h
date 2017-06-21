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
	Point getCenter(int label);
	int getLabel(int index);
	~MnistRecognizer();	
	vector<Mat> mnistImgs;
	int recognize(const Mat& img);
private:
	void preprocess();
	void clear();
	Mat imgCopy;
	network<sequential> nn;
	map<int, Point> mnistCenters; //<Label, Center>
	map<int, int> mnistLabels; // <Index, Label>

	int offset_x;
	int offset_y;
	int thresh;

	RNG rng; // random number generator
	string Model_Path;

};