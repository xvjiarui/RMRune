#include <iostream>
#include <string>
#include <stdexcept> 
#include "DigitRecognizer.h"
#include "RuneDetector.hpp"


DigitRecognizer::DigitRecognizer()
{

	rng = RNG(12345);

    lowerBound = Scalar(0, 0, 190);
    upperBound = Scalar(255, 255, 255);

    segmentTable = {
    	{119, 0},
    	{18, 1},
    	{93, 2},
    	{91, 3},
    	{58, 4},
    	{107, 5},
    	{111, 6},
    	{82, 7},
    	{127, 8},
    	{123, 9}
    };

    segmentRects = {
    	Rect(Point(10, 6), Point(35, 11)), // 0
    	Rect(Point(8, 5), Point(13, 27)), // 1
    	Rect(Point(32, 5), Point(37, 27)), //2
	    Rect(Point(5, 27), Point(35, 32)), // 3
	    Rect(Point(3, 32), Point(8, 58)), // 4
	    Rect(Point(27, 32), Point(32, 58)), //5
	    Rect(Point(5, 53), Point(30, 58)) // 6
    };

}

void DigitRecognizer::predict(const Mat& inputImg, const Rect2f & sudokuPanel)
{
	clear();

	Rect2f digitBoardRect = sudokuPanel;
	digitBoardRect.width = sudokuPanel.width / (280.0*3)*104.5*5;
	digitBoardRect.height = sudokuPanel.height / (160.0*3)*125.0;
	digitBoardRect -= Point2f(sudokuPanel.width * 0.25, sudokuPanel.height * 0.8);
	Mat img = inputImg(digitBoardRect);
	Mat grayImg;
	Mat digitBoardImg = img;
	cvtColor(img, grayImg, CV_BGR2GRAY);
	GaussianBlur(grayImg, grayImg, Size(9, 9), 0);
	Canny(grayImg, grayImg, 120, 120*2, 3);
	imshow("Canny", grayImg);


	vector<vector<Point> > digitContours;
	vector<Vec4i> digitHierarchy;
	findContours( grayImg, digitContours, digitHierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE, Point(0, 0) );

	vector<vector<Point> > digitContoursPolys;
	vector<Rect2f> digitBoundRects;
	vector<Rect2f> oneBoundRects;
	int digitAvgWidth = 0;
	float lowerThreshold = 1;
	float upperThreshold = 1.6;
	for ( int i = 0; i < digitContours.size(); i++ ) 
	{
		vector<Point> curDigitContoursPoly;
		approxPolyDP( digitContours.at(i), curDigitContoursPoly, 3, true );
		digitContoursPolys.push_back(curDigitContoursPoly);
		Rect2f curBoundingRect = boundingRect(Mat(curDigitContoursPoly));
		float ratio = (float) curBoundingRect.width / (float) curBoundingRect.height;	
		if (ratio < 0.5 * upperThreshold && ratio > 0.5 *lowerThreshold)
		{
			digitAvgWidth += curBoundingRect.width;
			digitBoundRects.push_back(curBoundingRect);
		}
		else if ( ratio < 0.32 && ratio > 0.25)
		{
			cout << "One Ratio: " << ratio << endl;
			oneBoundRects.push_back(curBoundingRect);
		}
	}
	digitAvgWidth /= digitBoundRects.size();
	if (digitBoundRects.size() != 5)
	{
		if (digitBoundRects.size() == 4 && oneBoundRects.size() == 1)
		{
			Rect2f curBoundingRect = oneBoundRects.at(0);
			curBoundingRect.width = digitAvgWidth;
			curBoundingRect -= Point2f(0.6 * digitAvgWidth, 0);
			digitBoundRects.push_back(curBoundingRect);
		}
		else return;
	}
	// for (int i = 0; i < digitBoundRects.size(); i++) {
	// 	rectangle( digitBoardImg, digitBoundRects.at(i), Scalar(255, 255, 255));
	// }
	// imshow("digitBoardRect", digitBoardImg);
	// waitKey(0);
	sort(digitBoundRects.begin(), digitBoundRects.end(), [] (Rect a, Rect b) { return a.x < b.x; });
	// digitBoardRect = Rect2f(digitBoundRects.at(0).tl(), digitBoundRects.at(digitBoundRects.size()-1).br());
	// int widthGap = (digitBoardRect.width - digitAvgWidth)/4;
	// digitAvgWidth /= digitBoundRects.size();
	// digitBoundRects.clear();
	// for (int i = 0; i < 5; ++i)
	// {
	// 	Rect2f curRect = Rect2f(0, 0, digitAvgWidth, digitBoardRect.height);
	// 	curRect = curRect + Point2f(i * (digitAvgWidth + widthGap), 0);
	// 	digitBoundRects.push_back(curRect);
	// }
	// Mat digitBoardImg = img(digitBoardRect);
	// for (int i = 0; i < digitBoundRects.size(); i++) {
	// 	rectangle( digitBoardImg, digitBoundRects.at(i), Scalar(255, 255, 255));
	// }
	// imshow("digitBoardRect", digitBoardImg);

    Mat hsvFrame;
    cvtColor(digitBoardImg, hsvFrame, CV_BGR2HSV);
    inRange(hsvFrame, lowerBound, upperBound, hsvFrame);
    hsvFrame.copyTo(digitBoardImg);
    imshow("hsvFrame", hsvFrame);

	for (int i = 0; i < digitBoundRects.size(); ++i)
	{
		Mat curImg = digitBoardImg(digitBoundRects.at(i));
		resize(curImg, curImg, Size(40, 60));
		digitImgs.push_back(curImg);
	}
	cout << "Digit: " << digitImgs.size() << endl;
	for (int i = 0; i < digitImgs.size(); ++i)
	{
		digitLabels.push_back(recognize(digitImgs.at(i)));
		cout << digitLabels.at(i);
	}
	cout << endl;
	
}

int DigitRecognizer::process(const Mat& img)
{
	Mat hsvImg;
	GaussianBlur(img, hsvImg, Size(9, 9), 0);
	cvtColor(hsvImg, hsvImg, CV_BGR2HSV);
	inRange(hsvImg, lowerBound, upperBound, hsvImg);
	resize(hsvImg, hsvImg, Size(40, 60));
	imshow("hsvImg", hsvImg);
	return recognize(hsvImg);
}

DigitRecognizer::~DigitRecognizer()
{
	digitImgs.clear();
	digitLabels.clear();
}


int DigitRecognizer::recognize(const Mat& img)
{
	int ret = 0;
	Mat temp;
	img.copyTo(temp);

	for (int i = 0; i < segmentRects.size(); ++i)
	{
		ret <<= 1;
		Mat curImg = img(segmentRects.at(i));
		int total = countNonZero(curImg);
		if ((float)total/ (float) segmentRects.at(i).area() > 0.5)
		{
			ret += 1;
		}
		rectangle(temp, segmentRects.at(i), Scalar(255, 255, 255));
	}
	try
	{
		ret = segmentTable.at(ret);
	}
	catch (out_of_range e)
	{
		return -1;
	}
	imshow("temp", temp);
	return ret;
}


void DigitRecognizer::clear()
{
	digitImgs.clear();
	digitLabels.clear();
}
