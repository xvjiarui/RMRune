#include <iostream>
#include <string>
#include <stdexcept> 
#include "DigitRecognizer.h"
#include "RuneDetector.hpp"

void binaryMat2points(const Mat & img, vector<Point> & pts)
{
	for(int x = 0; x < img.cols; x++)
	{
		for(int y = 0; y < img.rows; y++)
		{
			if(img.at<char>(x, y) >0)
			{
				pts.push_back(Point(x, y));
			}
		}
	}

}


DigitRecognizer::DigitRecognizer(Settings::LightSetting lightSetting): horizontalRect(Point(0, 0), Point(40, 10)), verticalRect(Point(0, 0), Point(10, 30))
{

	rng = RNG(12345);

    lowerBound = lightSetting.hsvLowerBound;
    upperBound = lightSetting.hsvUpperBound;

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

	areaThreshold = 0.6;

	float hXLen, hYLen, vXLen, vYLen;
	float xOffset, yOffset;
	hXLen = 40;
	hYLen = 60.0 / 7.0;
	vYLen = 60 / 2;
	vXLen = 40.0 / 3.0;
	Rect zero = Rect(Point2f(0, 0), Point2f(hXLen, hYLen));
	Rect one = Rect(Point2f(0, 0), Point2f(vXLen, vYLen));

    segmentRects = {
		Rect(Point(0, 0), Point(40, 20)), //0
		Rect(Point(0, 0), Point(20, 30)), //1
		Rect(Point(20, 0), Point(40, 30)),//2
		Rect(Point(0, 20), Point(40, 40)),//3
		Rect(Point(0, 30), Point(20, 60)),//4
		Rect(Point(20, 30), Point(40, 60)),//5
		Rect(Point(0, 40), Point(40, 60)),//6
		/*
		zero, // 0
		one, // 1
		one + Point(40 - vXLen, 0), // 2
		zero + Point(0, (60.0 - hYLen) / 2.0), // 3
		one + Point(0, 60 / 2), // 4
		one + Point(40 - vXLen, 60 / 2), // 5
		zero + Point(0, 60 - hYLen) //6
		*/.
		/*
    	Rect(Point(0, 0), Point(35, 5)), // 0
    	Rect(Point(6, 5), Point(11, 27)), // 1
    	Rect(Point(32, 5), Point(37, 27)), //2
	    Rect(Point(5, 27), Point(35, 32)), // 3
	    Rect(Point(3, 32), Point(8, 58)), // 4
	    Rect(Point(27, 32), Point(32, 58)), //5
	    Rect(Point(5, 53), Point(30, 58)) // 6
		*/
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
#ifdef SHOW_IMAGE
	imshow("Canny", grayImg);
#endif


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
#ifdef SHOW_IMAGE
    imshow("hsvFrame", hsvFrame);
#endif

	for (int i = 0; i < digitBoundRects.size(); ++i)
	{
		Mat curImg = digitBoardImg(digitBoundRects.at(i));
		resize(curImg, curImg, Size(40, 60));
		digitImgs.push_back(curImg);
	}
	cout << "Digit: " << digitImgs.size() << endl;
	for (int i = 0; i < digitImgs.size(); ++i)
	{
		digitLabels.push_back(adaptiveRecognize(digitImgs.at(i)));
		cout << digitLabels.at(i);
	}
	cout << endl;
	
}

#ifdef ADJUST_HSV
struct DigitRecognizerUserData {
	Scalar* lowerBound;
	Scalar* upperBound;
	Mat hsvImg;
	DigitRecognizer* digitRecognizer;
};

void AdjustHSVImg(int v, void* d)
{
	DigitRecognizerUserData* data = (DigitRecognizerUserData*)d;
	Scalar& lowerBound = *(data->lowerBound);
	Mat resImg;
	lowerBound[2] = v;
	inRange(data->hsvImg, lowerBound, *(data->upperBound), resImg);
	imshow("AdjustHsvImg", resImg);
}
#endif

int DigitRecognizer::process(const Mat& img)
{
	Mat hsvImg;
	GaussianBlur(img, hsvImg, Size(9, 9), 0);
	dilate(hsvImg, hsvImg, getStructuringElement(0, Size(9, 9)));
	cvtColor(img, hsvImg, CV_BGR2HSV);
#ifdef BACK_PROJECTION
	Mat valueFrame, hsvFrame;
	hsvImg.copyTo(hsvFrame);
	valueFrame.create(hsvFrame.size(), hsvFrame.depth());
	int ch[] = {2, 0};
	mixChannels(&hsvFrame, 1, &valueFrame, 1, ch, 1);
	MatND hist;
	int histSize = 15;
	float valueRange[] = {100, 255};
	const float* ranges = {valueRange};
	calcHist(&valueFrame, 1, 0, Mat(), hist, 1, &histSize, &ranges, true, false);
	normalize(hist, hist, 0, 255, NORM_MINMAX, -1, Mat());
	Mat backProjection;
	calcBackProject(&valueFrame, 1, 0, hist, backProjection, &ranges, 1, true);
	//morphologyEx(backProjection, backProjection, MORPH_CLOSE, getStructuringElement(MORPH_RECT,Size(3,3)));
	inRange(backProjection, {5}, {255}, backProjection);
#ifdef SHOW_IMAGE
	imshow("BackProj", backProjection);
#endif
#endif
#ifdef ADJUST_HSV
	int V = lowerBound[2];
	DigitRecognizerUserData data;
	data.lowerBound = &lowerBound;
	data.upperBound = &upperBound;
	hsvImg.copyTo(data.hsvImg);
	data.digitRecognizer = this;
	namedWindow("AdjustHsvImg", WINDOW_NORMAL);
	createTrackbar("Adjust Hsv", "AdjustHsvImg", &V, 255, AdjustHSVImg, (void*)&data);
	AdjustHSVImg(lowerBound[2], (void*)&data);
	waitKey(0);
#endif
	inRange(hsvImg, lowerBound, upperBound, hsvImg);
#ifdef BACK_PROJECTION
	return fitDigitAndRecognize(backProjection);
#endif
	return fitDigitAndRecognize(hsvImg);
}

int DigitRecognizer::fitDigitAndRecognize(Mat& hsvImg)
{
	Mat hsvCopy;
	hsvImg.copyTo(hsvCopy);
	morphologyEx(hsvCopy, hsvCopy, MORPH_CLOSE, getStructuringElement(MORPH_RECT,Size(3,3)));
	vector<vector<Point> > contours;
	vector<Vec4i> hierarchy;
	findContours( hsvCopy, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE, Point(0, 0) );			
	sort(contours.begin(), contours.end(), [](const vector<Point> & a, const vector<Point> & b) {return a.size()>b.size();});
	vector<Point> curContoursPoly;
	approxPolyDP(contours.at(0), curContoursPoly, 3, true);
	Rect curBoundingRect = boundingRect(curContoursPoly);
	float ratio = (float)curBoundingRect.width / (float)curBoundingRect.height;
	int ret = 0;
	if (ratio < 0.5)
	{
		resize(hsvImg, hsvImg, Size(40, 60));
	}
	else{
		hsvImg = hsvImg(boundingRect(curContoursPoly));
		resize(hsvImg, hsvImg, Size(40, 60));
	} 
	ret = similarityRecognize(hsvImg);
#ifdef SHOW_IMAGE
	imshow("hsvImg", hsvImg);
#endif
	return ret;
}

DigitRecognizer::~DigitRecognizer()
{
	digitImgs.clear();
	digitLabels.clear();
}

int DigitRecognizer::similarityRecognize(const Mat& img)
{
#ifdef SHOW_IMAGE
	Mat temp;
	img.copyTo(temp);
#endif
	static int digitTemplates[10][7] = {
		{1, 1, 1, 0, 1, 1, 1},
		{0, 0, 1, 0, 0, 1, 0},
		{1, 0, 1, 1, 1, 0, 1},
		{1, 0, 1, 1, 0, 1, 1},
		{0, 1, 1, 1, 0, 1, 0},
		{1, 1, 0, 1, 0, 1, 1},
		{1, 1, 0, 1, 1, 1, 1},
		{1, 0, 1, 0, 0, 1, 0},
		{1, 1, 1, 1, 1, 1, 1},
		{1, 1, 1, 1, 0, 1, 1}
	};
	struct Candidate{
		float difference;
		int index;
	} candidates[10];
	for (int i = 0; i < 10; i++)
	{
		candidates[i].difference = 0;
		candidates[i].index = i;
	}
	for (int i = 0; i < segmentRects.size(); ++i)
	{
		Mat curImg = img(segmentRects.at(i));
		int total = countNonZero(curImg);
		float curRatio = (float)total/ (float) segmentRects.at(i).area();
#ifdef SHOW_IMAGE
		rectangle(temp, segmentRects.at(i), Scalar(255, 255, 255));
#endif
		for (int j = 0; j < 10; ++j)
		{
			candidates[j].difference += abs(curRatio - digitTemplates[j][i]);
		}
	}
	sort(candidates, candidates + 10, [](const Candidate& a, const Candidate& b) { return a.difference < b.difference; } );
#ifdef SHOW_IMAGE
	imshow("temp", temp);
#endif
	return candidates[0].index;
}

int DigitRecognizer::recognize(const Mat& img)
{
	int ret = 0;
#ifdef SHOW_IMAGE
	Mat temp;
	img.copyTo(temp);
#endif


	for (int i = 0; i < segmentRects.size(); ++i)
	{
		ret <<= 1;
		Mat curImg = img(segmentRects.at(i));
		int total = countNonZero(curImg);
		if ((float)total/ (float) segmentRects.at(i).area() > 0.5)
		{
			ret += 1;
		}
#ifdef SHOW_IMAGE
		rectangle(temp, segmentRects.at(i), Scalar(255, 255, 255));
#endif
	}
#ifdef SHOW_IMAGE
	imshow("temp", temp);
#endif
	try
	{
		ret = segmentTable.at(ret);
	}
	catch (out_of_range e)
	{

		return -1;
	}
	return ret;
}

int DigitRecognizer::adaptiveRecognize(const Mat& img)
{
	int ret = 0;
#ifdef SHOW_IMAGE
	Mat temp;
	img.copyTo(temp);
#endif

	const int step = 1;
	for (int i = 0; i < 3; ++i)
	{
		for (int j = 0; j < 10; j+=step)
		{
			Rect curRect = horizontalRect + Point(0, 20 * i + j);
			Mat curImg = img(curRect);
			int total = countNonZero(curImg);
			if ((float)total / (float)curRect.area() > areaThreshold)
			{
#ifdef SHOW_IMAGE
				rectangle(temp, curRect, Scalar(255, 255, 255));
#endif
				ret += (1 << (6 - 3 * i));
				break;
			}
		}
	}
	for (int i = 0; i < 2; ++i)
	{
		for (int j = 0; j < 10; j+=step)
		{
			Rect curRect = verticalRect + Point(20 * i + j, 0);
			Mat curImg = img(curRect);
			int total = countNonZero(curImg);
			if ((float)total / (float)curRect.area() > areaThreshold)
			{
#ifdef SHOW_IMAGE
				rectangle(temp, curRect, Scalar(255, 255, 255));
#endif
				ret += (1 << (5 - i));
				break;
			}
		}
	}
	for (int i = 0; i < 2; ++i)
	{
		for (int j = 0; j < 10; j+=step)
		{
			Rect curRect = verticalRect + Point(20 * i + j, 30);
			Mat curImg = img(curRect);
			int total = countNonZero(curImg);
			if ((float)total / (float)curRect.area() > areaThreshold)
			{
#ifdef SHOW_IMAGE
				rectangle(temp, curRect, Scalar(255, 255, 255));
#endif
				ret += (1 << (2 - i));
				break;
			}
		}
	}
#ifdef SHOW_IMAGE
	imshow("temp", temp);
#endif
	try
	{
		ret = segmentTable.at(ret);
	}
	catch (out_of_range e)
	{

		return -1;
	}
	return ret;
}

void DigitRecognizer::clear()
{
	digitImgs.clear();
}
