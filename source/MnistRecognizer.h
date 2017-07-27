#pragma once

#include "tiny_dnn/tiny_dnn.h"
#include <opencv2/opencv.hpp>
#include "define.hpp"

using namespace tiny_dnn;
using namespace tiny_dnn::activation;
using namespace std;
using namespace cv;

class MnistRecognizer
{
public:
	MnistRecognizer(const string& dictionary = "LeNet-model");
	Point getCenter(int label);
	int getLabel(int index);
	~MnistRecognizer();	
	vector<Mat> mnistImgs;
	int recognize(const Mat& img);
	vector<pair<double, int> > recognize_primary(const Mat& img);
	Mat kmeanPreprocess(const Mat& inputImg);
private:
	void clear();
	bool fitMnist(const Mat& inputImg, Mat& resImg);
	network<sequential> nn;
	map<int, Point> mnistCenters; //<Label, Center>
	map<int, int> mnistLabels; // <Index, Label>

	RNG rng; // random number generator
	string Model_Path;

};
