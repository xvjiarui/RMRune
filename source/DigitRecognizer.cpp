#include <iostream>
#include <string>
#include <stdexcept> 
#include "DigitRecognizer.h"
#include "RuneDetector.hpp"


bool sortL2R(Rect a, Rect b)
{
	return a.x <b.y;
}
DigitRecognizer::DigitRecognizer()
{
	offsetX = 5;
	offsetY = 5;
	thresh = 182;

	ratioBM = Size2f(5 * 104.5 / 280.0, 130.0 / 160.0);
	mnistLittleRectReal = Size2f(280.0, 160.0);
	digitBoardOffsetXReal = 121.25;
	digitBoardOffsetYReal = 73.56;
	rng = RNG(12345);

    redLowerRange1 = Scalar(0, 0, 235);
    redUpperRange1 = Scalar(255, 255, 255);
    redLowerRange2 = Scalar(170, 50, 50);
    redUpperRange2 = Scalar(180, 255, 255);

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

void DigitRecognizer::predict(const Mat& img)
{
	img.copyTo(imgCopy);
	preprocess();
	for (int i = 0; i < digitImgs.size(); ++i)
	{
		digitLabels.push_back(recognize(digitImgs.at(i)));
	}
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


void DigitRecognizer::preprocess()
{
	clear();
	Mat img;
	imgCopy.copyTo(img);

	GaussianBlur(img, img, Size(9, 9) , 0);
	cvtColor(img, img, CV_BGR2GRAY);
	Canny(img, img, thresh, thresh * 2, 3);

	dilate(img, img, getStructuringElement(MORPH_RECT, Size(8, 8)));

	vector<vector<Point> > contours;
	vector<Vec4i> hierarchy;
	findContours( img, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE, Point(0, 0) );

	vector<vector<Point> > contoursPolys;
	vector<Rect> boundRects;
	vector<Rect> croppedBoundRects;

	for ( int i = 0; i < contours.size(); i++ ) 
	{
		vector<Point> curContoursPoly;
		approxPolyDP( contours.at(i), curContoursPoly, 3, true );
		contoursPolys.push_back(curContoursPoly);
		boundRects.push_back(boundingRect(Mat(curContoursPoly)));
	}

	Rect mnistLargeRect(Point(boundRects.at(0).tl().x, boundRects.at(0).br().y),
	                    Point(boundRects.at(boundRects.size() - 1).br().x, boundRects.at(boundRects.size() - 1).tl().y));

	/* Calculate average mnist rectangle */
	Point mnistLittleTl(0, 0);
	Point mnistLittleBr(0, 0);
	for (int i = 0; i < boundRects.size(); i++)
	{
		mnistLittleTl += boundRects.at(i).tl();
		mnistLittleBr += boundRects.at(i).br();
	}
	mnistLittleTl = Point(mnistLittleTl.x / boundRects.size(), mnistLittleTl.y / boundRects.size());
	mnistLittleBr = Point(mnistLittleBr.x / boundRects.size(), mnistLittleBr.y / boundRects.size());

	/* Get digit board position */
	Rect mnistLittleRect(mnistLittleTl, mnistLittleBr);
	Rect digitBoardRect(mnistLittleRect.x, mnistLittleRect.y,
	                    mnistLittleRect.width * ratioBM.width, mnistLittleRect.height * ratioBM.height);
	float real2Mat = 0.5 * (mnistLittleRect.width / mnistLittleRectReal.width + mnistLittleRect.height / mnistLittleRectReal.height);
	digitBoardRect -= Point(digitBoardOffsetXReal * real2Mat, 2 * mnistLittleRect.height * ratioBM.height + 1.65 * digitBoardOffsetYReal * real2Mat);

	rectangle(img, digitBoardRect, Scalar(255, 255, 255), -1, 8, 0);
	cvtColor(img, img, CV_GRAY2BGR);
	bitwise_and(imgCopy, img, img);

	imgCopy(digitBoardRect).copyTo(img);
	// imgCopy(digitBoardRect).copyTo(show);
	// cvtColor(show, show, CV_BGR2HSV);
	// GaussianBlur(img, img, Size(9, 9) , 0);
	// erode(img, img, getStructuringElement(MORPH_RECT, Size(3, 3)));
    // Mat temp1, temp2;
    // inRange(hsvFrame, redLowerRange1, redUpperRange1, temp1);  
    // inRange(hsvFrame, redLowerRange2, redUpperRange2, temp2);  
    // addWeighted(temp1, 1.0, temp2, 1.0, 0.0, hsvFrame);
    // morphologyEx(hsvFrame, hsvFrame, MORPH_OPEN, getStructuringElement(MORPH_ELLIPSE, Size(1, 5)));

    Mat hsvFrame;
    cvtColor(img, hsvFrame, CV_BGR2HSV);
    inRange(hsvFrame, redLowerRange1, redUpperRange1, hsvFrame);
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

	sort(digitBoundRects.begin(), digitBoundRects.end(), sortL2R);
	for (int i = 0; i < digitBoundRects.size(); ++i)
	{
		Mat curImg = img(digitBoundRects.at(i));
		resize(curImg, curImg, Size(40, 60));
		digitImgs.push_back(curImg);
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

void DigitRecognizer::clear()
{
	digitImgs.clear();
	digitLabels.clear();
}
