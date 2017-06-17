#include <iostream>
#include <string>
#include "DigitRecognizer.h"

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

    redLowerRange1 = Scalar(0, 70, 50);
    redUpperRange1 = Scalar(10, 255, 255);
    redLowerRange2 = Scalar(170, 50, 50);
    redUpperRange2 = Scalar(180, 255, 255);

}

void DigitRecognizer::predict(const Mat& img)
{
	imgCopy = img;
	preprocess();
}

DigitRecognizer::~DigitRecognizer()
{

}


int DigitRecognizer::recognize(const Mat& img)
{
	return -1;
}

void DigitRecognizer::preprocess()
{
	clear();
	Mat img = imgCopy;

	GaussianBlur(img, img, Size(9, 9) , 0);
	cvtColor(img, img, CV_BGR2GRAY);
	Canny(img, img, thresh, thresh * 2, 3);

	Mat structuringElement = getStructuringElement(MORPH_RECT, Size(8, 8));
	dilate(img, img, structuringElement);

	vector<vector<Point> > contours;
	vector<Vec4i> hierarchy;
	findContours( img, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE, Point(0, 0) );

	vector<vector<Point> > contoursPolys;
	vector<Rect> boundRects;
	vector<Rect> croppedBoundRects;

	for ( int i = 0; i < contours.size(); i++ ) {
		vector<Point> curContoursPoly;
		approxPolyDP( contours.at(i), curContoursPoly, 3, true );
		contoursPolys.push_back(curContoursPoly);
		boundRects.push_back(boundingRect(Mat(curContoursPoly)));
	}

	Rect mnistLargeRect(Point(boundRects.at(0).tl().x, boundRects.at(0).br().y),
	                    Point(boundRects.at(boundRects.size() - 1).br().x, boundRects.at(boundRects.size() - 1).tl().y));
	/* Check Rect Height-Width Ratio */
	// for (int i = 0; i < boundRects.size(); i++) {
	// 	int height = boundRects.at(i).height;
	// 	int width = boundRects.at(i).width;
	// 	cout << "Rect " << i << "Ratio = " << (float)height / (float)width << endl;
	// 	cout << "Centre " << i << " (" << boundRects.at(i).x << "," << boundRects.at(i).y << ")" << endl;
	// }

	/* Calculate average mnist rectangle */
	Point mnistLittleTl(0, 0);
	Point mnistLittleBr(0, 0);
	for (int i = 0; i < boundRects.size(); i++)
	{
		mnistLittleTl += boundRects.at(i).tl();
		mnistLittleBr += boundRects.at(i).br();
		// rectangle( img, boundRects.at(i), Scalar(255, 255, 255), -1, 8, 0 );
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

	img = img(digitBoardRect);
    Mat hsvFrame;
    cvtColor(img, hsvFrame, CV_BGR2HSV);
    Mat temp1, temp2;
    inRange(hsvFrame, redLowerRange1, redUpperRange1, temp1);  
    inRange(hsvFrame, redLowerRange2, redUpperRange2, temp2);  
    addWeighted(temp1, 1.0, temp2, 1.0, 0.0, hsvFrame);
    structuringElement = getStructuringElement(MORPH_ELLIPSE, Size(1, 5));
    morphologyEx(hsvFrame, hsvFrame, MORPH_OPEN, structuringElement);

	while (waitKey(5) != 27)
	{
		imshow("board", img);
		imshow("hsv", hsvFrame);
	}
}

void DigitRecognizer::clear()
{
	digitImgs.clear();
	digitLabels.clear();
}
