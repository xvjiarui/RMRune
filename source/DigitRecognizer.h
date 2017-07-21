#pragma once

#include "define.hpp"
#include <opencv2/opencv.hpp>
#include <unordered_map>
#include "Settings.hpp"

using namespace std;
using namespace cv;

typedef cv::Rect_<float> Rect2f;

class DigitRecognizer
{
public:
	DigitRecognizer(Settings::LightSetting);
	void predict(const Mat& original_img, const Rect2f & sudoku_panel);
	int process(const Mat& img);
	vector<pair<double, int> > process_primary(const Mat& img);
	~DigitRecognizer();	
	vector<Mat> digitImgs;
	vector<int> digitLabels;

private:
	Mat preprocess(const Mat& img);
	int recognize(const Mat& img);
	int adaptiveRecognize(const Mat& img);
	int similarityRecognize(const Mat& img);
	vector<pair<double, int> > similarityRecognize_primary(const Mat& img);
	void clear();
#ifdef ADJUST_HSV
	friend void AdjustHSVImg(int, void*);
#endif

	Mat imgCopy;
	unordered_map<int, int> segmentTable;
	vector<Rect> segmentRects;
    Scalar lowerBound;
    Scalar upperBound;
	RNG rng; // random number generator
	Rect horizontalRect;
	Rect verticalRect;
	float areaThreshold;
	bool fitDigit(const Mat& hsvImg, Mat& digitImg);

};
