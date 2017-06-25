#include <iostream>
#include <string>
#include <stdexcept> 
#include "DigitRecognizer.h"
#include "RuneDetector.hpp"


DigitRecognizer::DigitRecognizer()
{

	rng = RNG(12345);

    lowerBound = Scalar(0, 0, 180);
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

void DigitRecognizer::predict(const Mat& inputImg, const Rect2f & sudokuPanel)
{
	clear();

	Rect2f digitBoardRect = sudokuPanel;
	digitBoardRect.width = sudokuPanel.width / (280.0*3)*90*5;
	digitBoardRect.height = sudokuPanel.height / (160.0*3)*122.0;
	digitBoardRect -= Point2f(sudokuPanel.width * 0.22, sudokuPanel.height * 0.8);
	Mat img = inputImg(digitBoardRect);
	Mat grayImg;
	cvtColor(img, grayImg, CV_BGR2GRAY);
	GaussianBlur(grayImg, grayImg, Size(9, 9), 0);
	Canny(grayImg, grayImg, 120, 120*2, 3);
	imshow("Canny", grayImg);


	vector<vector<Point> > digitContours;
	vector<Vec4i> digitHierarchy;
	findContours( grayImg, digitContours, digitHierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE, Point(0, 0) );
	Mat drawing = Mat::zeros(grayImg.size(), CV_8UC3);
	for (int i = 0; i < digitContours.size(); i++)
	{
		Scalar color = Scalar(rng.uniform(0, 255), rng.uniform(0, 255), rng.uniform(0, 255));
		drawContours(drawing, digitContours, i, color, 2, 8, digitHierarchy, 0, Point());
	}
	imshow("Contours", drawing);

	vector<vector<Point> > digitContoursPolys;
	vector<Rect2f> digitBoundRects;
	int digitAvgWidth = 0;
	int digitAvgCount = 0;
	float lowerThreshold = 1.2;
	float upperThreshold = 1.6;
	for ( int i = 0; i < digitContours.size(); i++ ) 
	{
		vector<Point> curDigitContoursPoly;
		approxPolyDP( digitContours.at(i), curDigitContoursPoly, 3, true );
		digitContoursPolys.push_back(curDigitContoursPoly);
		Rect curBoundingRect = boundingRect(Mat(curDigitContoursPoly));
		float ratio = (float) curBoundingRect.width / (float) curBoundingRect.height;	
		if (ratio < 0.5 * upperThreshold && ratio > 0.5 *lowerThreshold)
		{
			digitAvgWidth += curBoundingRect.width;
			digitBoundRects.push_back(curBoundingRect);
		}
	}
	if (digitBoundRects.size() != 5)
	{
		return;
	}
	sort(digitBoundRects.begin(), digitBoundRects.end(), [] (Rect a, Rect b) { return a.x < b.x; });
	digitBoardRect = Rect2f(digitBoundRects.at(0).tl(), digitBoundRects.at(digitBoundRects.size()-1).br());
	int widthGap = (digitBoardRect.width - digitAvgWidth)/4;
	digitAvgWidth /= digitBoundRects.size();
	digitBoundRects.clear();
	for (int i = 0; i < 5; ++i)
	{
		Rect2f curRect = Rect2f(0, 0, digitAvgWidth, digitBoardRect.height);
		curRect = curRect + Point2f(i * (digitAvgWidth + widthGap), 0);
		digitBoundRects.push_back(curRect);
	}
	Mat digitBoardImg = img(digitBoardRect);
	for (int i = 0; i < digitBoundRects.size(); i++) {
		rectangle( digitBoardImg, digitBoundRects.at(i), Scalar(255, 255, 255));
	}
	imshow("digitBoardRect", digitBoardImg);

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
		return -1;
	}
	return ret;
}


void DigitRecognizer::clear()
{
	digitImgs.clear();
	digitLabels.clear();
}
