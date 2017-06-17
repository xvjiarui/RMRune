#pragma once

#include <opencv2/opencv.hpp>

using namespace std;
using namespace cv;

class DigitRecognizer
{
public:
	DigitRecognizer();
	void predict(const Mat& img);
	~DigitRecognizer();	
	vector<Mat> digitImgs;
	vector<int> digitLabels;
private:
	int recognize(const Mat& img);
	void preprocess();
	void clear();
	Mat imgCopy;

	int offsetX;
	int offsetY;
	int thresh;

	Size2f ratioBM;
	Size2f mnistLittleRectReal;
	float digitBoardOffsetXReal;
	float digitBoardOffsetYReal;
    Scalar redLowerRange1;
    Scalar redUpperRange1;
    Scalar redLowerRange2;
    Scalar redUpperRange2;

	RNG rng; // random number generator

};