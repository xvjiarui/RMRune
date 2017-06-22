#include <iostream>
#include <string>
#include <stdexcept> 
#include "DigitRecognizer.h"
#include "RuneDetector.hpp"


DigitRecognizer::DigitRecognizer()
{

	rng = RNG(12345);

    lowerBound = Scalar(0, 0, 235);
    upperBound = Scalar(255, 255, 255);

    segmentTable = {
    	{119, 0},
    	{18, 1},
    	{94, 2},
    	{91, 3},
    	{58, 4},
    	{107, 5},
    	{111, 6},
    	{82, 7},
    	{127, 8},
    	{123, 9}
    };

    segmentRects = {
    	Rect(Point(5, 0), Point(35, 5)), // 0
    	Rect(Point(5, 0), Point(10, 30)), // 1
    	Rect(Point(30, 0), Point(35, 30)), //2
	    Rect(Point(5, 27), Point(35, 32)), // 3
	    Rect(Point(2, 30), Point(7, 60)), // 4
	    Rect(Point(27, 30), Point(32, 60)), //5
	    Rect(Point(5, 55), Point(35, 60)) // 6
    };

}

void DigitRecognizer::predict(const Mat& inputImg, Rect sudokuPanel)
{
	clear();

	// Rect2f digitBoardRect = Rect2f(sudokuPanel.width / (280.0*3) * 104.5 * 5, sudokuPanel.height / (170.0*3) * 122.0);
	Rect2f digitBoardRect = sudokuPanel;
	Mat img = inputImg(digitBoardRect);
	imshow("digit_board", img);
	waitKey(0);

    Mat hsvFrame;
    cvtColor(img, hsvFrame, CV_BGR2HSV);
    inRange(hsvFrame, lowerBound, upperBound, hsvFrame);
    hsvFrame.copyTo(img);

	vector<vector<Point> > digitContours;
	vector<Vec4i> digitHierarchy;
	findContours( hsvFrame, digitContours, digitHierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE, Point(0, 0) );

	vector<vector<Point> > digitContoursPolys;
	vector<Rect> digitBoundRects;

	for ( int i = 0; i < digitContours.size(); i++ ) 
	{
		vector<Point> curDigitContoursPoly;
		approxPolyDP( digitContours.at(i), curDigitContoursPoly, 3, true );
		digitContoursPolys.push_back(curDigitContoursPoly);
		digitBoundRects.push_back(boundingRect(Mat(curDigitContoursPoly)));
	}

	sort(digitBoundRects.begin(), digitBoundRects.end(), [] (Rect a, Rect b) { return a.x < b.x; });
	for (int i = 0; i < digitBoundRects.size(); ++i)
	{
		Mat curImg = img(digitBoundRects.at(i));
		resize(curImg, curImg, Size(40, 60));
		digitImgs.push_back(curImg);
	}
	for (int i = 0; i < digitImgs.size(); ++i)
	{
		digitLabels.push_back(recognize(digitImgs.at(i)));
	}
	// Rect digitSingleRect = Rect(Point(0, 0), img.size());
	// digitSingleRect.width /= digitBoundRects.size();
	// for (int i = 0; i < digitBoundRects.size(); i++) {
	// 	rectangle( img, digitBoundRects.at(i), Scalar(255, 255, 255));
	// }
	// for (int i = 0; i < digitBoundRects.size(); ++i)
	// {
	// 	Mat curImg = img(digitSingleRect);
	// 	resize(curImg, curImg, Size(40, 60));
	// 	digitImgs.push_back(curImg);
	// 	digitSingleRect.x += digitSingleRect.width;
	// }
}

DigitRecognizer::~DigitRecognizer()
{
	digitImgs.clear();
	digitLabels.clear();
}


int DigitRecognizer::recognize(const Mat& img)
{
	int ret = 0;
	for (int i = 0; i < segmentRects.size(); ++i)
	{
		ret <<= 1;
		Mat curImg = img(segmentRects.at(i));
		int total = countNonZero(curImg);
		if ((float)total/ (float) segmentRects.at(i).area() > 0.5)
		{
			ret += 1;
		}
	}
	try
	{
		ret = segmentTable[ret];
	}
	catch (out_of_range e)
	{
		cout << "Cannot recognize" << endl;
		return 1;
	}
	return ret;
}


void DigitRecognizer::clear()
{
	digitImgs.clear();
	digitLabels.clear();
}
