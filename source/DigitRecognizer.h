#pragma once

#include <opencv2/opencv.hpp>
#include <unordered_map>

using namespace std;
using namespace cv;

typedef cv::Rect_<float> Rect2f;

class DigitRecognizer
{
public:
	DigitRecognizer();
	void predict(const Mat& original_img, const Rect2f & sudoku_panel);
	int process(const Mat& img);
	~DigitRecognizer();	
	vector<Mat> digitImgs;
	vector<int> digitLabels;

private:
	int recognize(const Mat& img);
	void clear();
	Mat imgCopy;
	unordered_map<int, int> segmentTable;
	vector<Rect> segmentRects;
	
    Scalar lowerBound;
    Scalar upperBound;

	RNG rng; // random number generator

};