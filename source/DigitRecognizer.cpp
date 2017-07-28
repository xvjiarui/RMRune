#include <iostream>
#include <string>
#include <stdexcept>
#include "DigitRecognizer.h"
#include "RuneDetector.hpp"
#include <opencv2/opencv.hpp>
#include <opencv2/gpu/gpu.hpp>

void binaryMat2points(const Mat & img, vector<Point> & pts)
{
	for (int x = 0; x < img.cols; x++)
	{
		for (int y = 0; y < img.rows; y++)
		{
			if (img.at<char>(x, y) > 0)
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
	DigitRecognizer::lightSetting = lightSetting;

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

	areaThreshold = 0.5;

	float hXLen, hYLen, vXLen, vYLen;
	float hXLenRatio = 1.0 / 2.0;
	float hYLenRatio = 1.0 / 7.0;
	hXLen = 40.0 * hXLenRatio;
	hYLen = 60.0 * hYLenRatio;
	vXLen = 40.0 * ((1 - hXLenRatio) / 2.0);
	vYLen = 60.0 * ((1 - 3 * hYLenRatio) / 2.0);
	Rect zero = Rect(Point2f(vXLen, 0), Point2f(vXLen + hXLen, hYLen));
	Rect one = Rect(Point2f(0, hYLen), Point2f(vXLen, hYLen + vYLen));

	segmentRects = {
		zero, // 0
		one, // 1
		one + Point(hXLen + vXLen, 0), // 2
		zero + Point(0, hYLen + vYLen), // 3
		one + Point(0, hYLen + vYLen), // 4
		one + Point(hXLen + vXLen, hYLen + vYLen), // 5
		zero + Point(0, 2 * (hYLen + vYLen)) // 6
		/*,
		Rect(Point(0, 0), Point(40, 20)), //0
		Rect(Point(0, 0), Point(20, 30)), //1
		Rect(Point(20, 0), Point(40, 30)),//2
		Rect(Point(0, 20), Point(40, 40)),//3
		Rect(Point(0, 30), Point(20, 60)),//4
		Rect(Point(20, 30), Point(40, 60)),//5
		Rect(Point(0, 40), Point(40, 60)),//6
		*/
		/*
		zero, // 0
		one, // 1
		one + Point(40 - vXLen, 0), // 2
		one + Point(hXLen, 0)
		zero + Point(0, (60.0 - hYLen) / 2.0), // 3
		one + Point(0, 60 / 2), // 4
		one + Point(40 - vXLen, 60 / 2), // 5
		zero + Point(0, 60 - hYLen) //6
		*/
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
	// detector = SurfFeatureDetector(400);

	float data[] = {1, 0.1, 0,  0, 1, 0};
	Mat affine(2, 3, CV_32FC1, data);

	digitTemplateImgs.resize(10);
	descriptors.resize(10);
	keyPoints.resize(10);
	for (int i = 0; i < digitTemplateImgs.size(); ++i)
	{
		const char digit = '0' + i;
		digitTemplateImgs.at(i) = imread(string( "template_digit/") + to_string(i) + string(".png"), IMREAD_GRAYSCALE);
		resize(digitTemplateImgs.at(i), digitTemplateImgs.at(i), Size(40, 60));
		warpAffine(digitTemplateImgs.at(i), digitTemplateImgs.at(i), affine, digitTemplateImgs.at(i).size());
		cout << "Image" << i << "success" << endl;
	}


	Mat temp;
	for(int i = 0; i< digitTemplateImgs.size(); i++)
	{
		digitTemplateImgs.at(i).copyTo(temp);
		trainData.push_back(temp.reshape(0, 1));
		trainLabel.push_back(i);
	}
	trainLabel.convertTo(trainLabel, CV_32FC1);
	trainData.convertTo(trainData, CV_32FC1, 1/255.0);
}

void DigitRecognizer::predict(const Mat& inputImg, const Rect2f & sudokuPanel)
{
	clear();

	Rect2f digitBoardRect = sudokuPanel;
	digitBoardRect.width = sudokuPanel.width / (280.0 * 3) * 104.5 * 5;
	digitBoardRect.height = sudokuPanel.height / (160.0 * 3) * 125.0;
	digitBoardRect -= Point2f(sudokuPanel.width * 0.25, sudokuPanel.height * 0.8);
	Mat img = inputImg(digitBoardRect);
	Mat grayImg;
	Mat digitBoardImg = img;
	cvtColor(img, grayImg, CV_BGR2GRAY);
	GaussianBlur(grayImg, grayImg, Size(9, 9), 0);
	Canny(grayImg, grayImg, 120, 120 * 2, 3);
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
		if (ratio < 0.5 * upperThreshold && ratio > 0.5 * lowerThreshold)
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

Mat DigitRecognizer::preprocess(const Mat& img)
{
	Mat hsvImg;
	GaussianBlur(img, hsvImg, Size(9, 9), 0);
	dilate(hsvImg, hsvImg, getStructuringElement(0, Size(9, 9)));
#ifdef ADAPTIVE_THRESHOLD_IN_DIGIT
	cvtColor(img, hsvImg, CV_GRAY2BGR);
	cvtColor(hsvImg, hsvImg, CV_BGR2HSV);
#else
	cvtColor(img, hsvImg, CV_BGR2HSV);
#endif
#ifdef BACK_PROJECTION
	Mat hsvFrame;
	hsvImg.copyTo(hsvFrame);
	int ch[] = {0, 2};
	Mat hist;
	int histSize[] = {15, 15};
	float hueRange[] = {0, 255};
	float valueRange[] = {100, 255};
	const float* ranges[] = {hueRange, valueRange};
	calcHist(&hsvFrame, 1, ch, Mat(), hist, 2, histSize, ranges, true, false);
	normalize(hist, hist, 0, 255, NORM_MINMAX, -1, Mat());
	Mat backProjection;
	calcBackProject(&hsvFrame, 1, ch, hist, backProjection, ranges, 1, true);
	//morphologyEx(backProjection, backProjection, MORPH_CLOSE, getStructuringElement(MORPH_RECT,Size(3,3)));
#ifdef SHOW_IMAGE
	imshow("Before binary", backProjection);
#endif
	inRange(backProjection, Scalar(5, 5), Scalar(255, 255), backProjection);
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
	return backProjection;
#endif
	return hsvImg;
}

Mat DigitRecognizer::kmeanPreprocess(const Mat& img)
{
	int rows = img.rows;
	int cols = img.cols;
	int channels = img.channels();
	Mat labels;
	Mat pixels(rows * cols + 1, 1, CV_32FC3); //extra one for red
	pixels.setTo(Scalar::all(0));

	float *pdata = pixels.ptr<float>(0);
	for (int i = 0; i < rows; ++i)
	{
		const uchar *idata = img.ptr<uchar>(i);
		for (int j = 0; j < cols * channels; ++j)
		{
			pdata[i * cols * channels + j] = saturate_cast<float>(idata[j]);
		}
	}
	pdata[rows * cols * channels] = 255;
	pdata[rows * cols * channels + 1] = 255;
	pdata[rows * cols * channels + 2] = 255;
	kmeans(pixels, lightSetting.kmeansClasses, labels, TermCriteria(CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, lightSetting.kmeansItrTimes, 0), 5, KMEANS_PP_CENTERS);
	Mat redImg;
	img.copyTo(redImg);
	int redClass = labels.at<int>(rows * cols);
	pdata = redImg.ptr<float>(0);
	for (int i = 0; i < rows; i++)
	{
		for (int j = 0; j < cols; j++)
		{
			if (labels.at<int>(i * cols + j) != redClass)
			{
				redImg.at<Vec3b>(i, j) = Vec3b(0, 0, 0);
			}
			else redImg.at<Vec3b>(i, j) = Vec3b(255, 255, 255);
		}
	}
	cvtColor(redImg, redImg, CV_BGR2GRAY);
	/*
	imshow("before", redImg);
	//imshow("after", redImg);
	waitKey(0);
	*/
	return redImg;
}
int DigitRecognizer::process(const Mat& img)
{
	Mat digitImg;
	if (fitDigit(preprocess(img), digitImg))
	{
		return similarityRecognize(digitImg);
	}
	return -1;
}

vector<pair<double, int> > DigitRecognizer::process_primary(const Mat& img)
{
	Mat digitImg;
	Mat kmeanImg;
	kmeanPreprocess(img).copyTo(kmeanImg);
	if (fitDigit(kmeanImg, digitImg))
	{
		return cannyRecognize_primary(digitImg);
	}
	return vector<pair<double, int> >();
}

vector<pair<double, int> > DigitRecognizer::process_primary_similarity(const Mat& img)
{
	Mat digitImg;
	Mat kmeanImg;
	kmeanPreprocess(img).copyTo(kmeanImg);
	if (fitDigit(kmeanImg, digitImg))
	{
		return similarityRecognize_primary(digitImg);
	}
	return vector<pair<double, int> >();
}


bool DigitRecognizer::fitDigit(const Mat& inputImg, Mat& resImg)
{
	Mat inputCopy;
	inputImg.copyTo(inputCopy);
	dilate(inputCopy, inputCopy, getStructuringElement(MORPH_RECT,Size(3,3)), Point(-1, -1), 3);
	//morphologyEx(inputCopy, inputCopy, MORPH_CLOSE, getStructuringElement(MORPH_RECT,Size(3,3)));
	//adaptiveThreshold(redImg, redImg, 255, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY, 3, 0);
	vector<vector<Point> > contours;
	vector<Vec4i> hierarchy;
	findContours( inputCopy, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE, Point(0, 0) );
	if (!contours.size()) return false;
	sort(contours.begin(), contours.end(), [](const vector<Point> & a, const vector<Point> & b) {return a.size() > b.size();});
	vector<Point> curContoursPoly;
	approxPolyDP(contours.at(0), curContoursPoly, 1, true);
	Rect curBoundingRect = boundingRect(curContoursPoly);
	float ratio = (float)curBoundingRect.width / (float)curBoundingRect.height;
	if (ratio < 0.5)
	{
		Mat tempImg = inputImg(boundingRect(curContoursPoly));
		Mat zeroImg = Mat::zeros(tempImg.rows, tempImg.rows / 60.0 * 40.0 - tempImg.cols, CV_8UC1);
		// inputCopy = Mat(40, 60, CV_8UC1);
		hconcat(zeroImg, tempImg, inputCopy);
		resize(inputCopy, inputCopy, Size(40, 60));
	}
	else {
		inputCopy = inputImg(boundingRect(curContoursPoly));
		resize(inputCopy, inputCopy, Size(40, 60));
	}
	float data[] = {1, 0.1, 0,  0, 1, 0};
	Mat affine(2, 3, CV_32FC1, data);
	warpAffine( inputCopy, inputCopy, affine, inputCopy.size());
	dilate(inputCopy, inputCopy, getStructuringElement(MORPH_RECT,Size(1,1)), Point(-1, -1), 3);
	int col;
	for (col = 39; col > -1; col--)
	{
		if (countNonZero(inputCopy.col(col)))
			break;
	}
	if (col != 39)
	{
		Rect cutRect(Point(0, 0), Point(col, inputCopy.rows)); 
		inputCopy(cutRect).copyTo(inputCopy);
		hconcat(Mat::zeros(inputCopy.rows, 39 - col, CV_8UC1), inputCopy, inputCopy);
	}
#ifdef SHOW_IMAGE
	imshow("inputImg", inputCopy);
#endif
	inputCopy.copyTo(resImg);
	return true;
}

/*
int DigitRecognizer::featureProcess(const Mat& inputImg)
{
	Mat inputCopy;
	inputImg.copyTo(inputCopy);
	vector<KeyPoint> curKeyPoints;
	detector.detect(inputCopy, curKeyPoints);
	Mat curDescriptors;
	extractor.compute( inputCopy, curKeyPoints, curDescriptors);
	FlannBasedMatcher matcher;
	vector<vector <DMatch> > matches(10);
	vector<pair<int, int> > scores(10); // <num, score>
	for (int i = 0; i < digitTemplateImgs.size(); ++i)
	{
		scores.at(i).first = i;
		scores.at(i).second = 0;
		static double maxDist = 0.0;
		static double minDist = 100.0;
		matcher.match(curDescriptors, descriptors.at(i), matches.at(i));
		for ( int j = 0; j < curDescriptors.rows; j++ )
		{
			double dist = matches.at(i).at(j).distance;
			if ( dist < minDist ) minDist = dist;
			if ( dist > maxDist ) maxDist = dist;
		}
		vector< DMatch > good_matches;

		for ( int j = 0; j < curDescriptors.rows; j++ )
		{
			if ( matches.at(i).at(j).distance <= max(5 * minDist, 0.02) )
			{
				good_matches.push_back( matches.at(i).at(j));
				scores.at(i).second++;
			}
		}

		//-- Draw only "good" matches
		Mat img_matches;
		drawMatches( inputCopy, curKeyPoints, digitTemplateImgs.at(i), keyPoints.at(i),
		             good_matches, img_matches, Scalar::all(-1), Scalar::all(-1),
		             vector<char>(), DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS );

		imshow( "Good Matches", img_matches );
		waitKey(0);
	}
	sort(scores.begin(), scores.end(), [] (const pair<int, int>& a, const pair<int, int>& b) { return a.second > b.second;});
	return scores.at(0).first;

}
*/
/*
vector<pair<double, int> > DigitRecognizer::fitDigitAndRecognize_primary(const Mat& hsvImg)
{
	ret = similarityRecognize(hsvCopy);
	return ret;
}
int DigitRecognizer::fitDigitAndRecognize(const Mat& hsvImg)
{
	Mat hsvCopy;
	hsvImg.copyTo(hsvCopy);
	morphologyEx(hsvCopy, hsvCopy, MORPH_CLOSE, getStructuringElement(MORPH_RECT,Size(3,3)));
	vector<vector<Point> > contours;
	findContours( hsvCopy, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE, Point(0, 0) );
	if (!contours.size()) return -1;
	sort(contours.begin(), contours.end(), [](const vector<Point> & a, const vector<Point> & b) {return a.size()>b.size();});
	vector<Point> curContoursPoly;
	approxPolyDP(contours.at(0), curContoursPoly, 3, true);
	Rect curBoundingRect = boundingRect(curContoursPoly);
	float ratio = (float)curBoundingRect.width / (float)curBoundingRect.height;
	int ret = 0;
	if (ratio < 0.5)
	{
		resize(hsvImg, hsvCopy, Size(40, 60));
	}
	else{
		hsvCopy = hsvImg(boundingRect(curContoursPoly));
		resize(hsvCopy, hsvCopy, Size(40, 60));
	}
	ret = similarityRecognize(hsvCopy);
#ifdef SHOW_IMAGE
	imshow("hsvImg", hsvCopy);
#endif
	return ret;
}
*/

DigitRecognizer::~DigitRecognizer()
{
	digitImgs.clear();
	digitLabels.clear();
}

int DigitRecognizer::similarityRecognize(const Mat& img)
{
	return similarityRecognize_primary(img)[0].second;
}

vector<pair<double, int> > DigitRecognizer::similarityRecognize_primary(const Mat& inputImg)
{
#ifdef SHOW_IMAGE
	Mat temp;
	img.copyTo(temp);
#endif
	Mat img;
	resize(inputImg, img, Size(40, 60));
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
	vector<pair<double, int> > candidates(10);
	for (int i = 0; i < candidates.size(); i++)
	{
		candidates[i].first = 0;
		candidates[i].second = i;
	}
	for (int i = 0; i < segmentRects.size(); ++i)
	{
		Mat curImg = img(segmentRects.at(i));
		int total = countNonZero(curImg);
		double curRatio = (double)total / (double)segmentRects.at(i).area();
#ifdef SHOW_IMAGE
		rectangle(temp, segmentRects.at(i), Scalar(255, 255, 255));
#endif
		for (int j = 0; j < 10; ++j)
		{
			candidates[j].first += abs(curRatio - digitTemplates[j][i]);
		}
	}
	sort(candidates.begin(), candidates.end(), [](const pair<double, int>& a, const pair<double, int>& b) { return a.first < b.first; } );
#ifdef SHOW_IMAGE
	imshow("temp", temp);
#endif
	return candidates;
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
		if ((float)total / (float) segmentRects.at(i).area() > 0.5)
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


int DigitRecognizer::knnRecognize(const Mat& img)
{
	CvKNearest knnClassifier(trainData, trainLabel);
	Mat newImg, sample(1, 40 * 60, CV_32FC1);
	Mat result(1, 1, CV_32FC1);
	resize(img, newImg, Size(40, 60));
	Mat tube = newImg.reshape(0, 1);
	tube.convertTo(tube, CV_32FC1, 1/255.0);
	knnClassifier.find_nearest(tube, 1, &result);
	return (int)result.at<float>(0, 0);
}

vector<pair<double, int> > DigitRecognizer::knnRecognize_primary(const Mat& img)
{
	CvKNearest knnClassifier(trainData, trainLabel);
	const int k = 1;
	Mat newImg;
	Mat result(1, 1, CV_32FC1);
	resize(img, newImg, Size(40, 60));
	Mat tube = newImg.reshape(0, 1);
	tube.convertTo(tube, CV_32FC1, 1/255.0);
	Mat result_score(10, k, CV_32FC1);
	Mat a(10, k, CV_32FC1);
	vector<pair<double, int> > vec_result_score;
	knnClassifier.find_nearest(tube, (const int)k, &result, 0, 0, &result_score);
	cout << "prediction: " << result.at<float>(0, 0) << endl;
	for (int i = 0; i < 10; i++)
	{
		float overallDist = 0;
		for (int j = 0; j < k; j++)
		{
			overallDist += result_score.at<float>(j, i);
		}
		vec_result_score.push_back(make_pair<double, int>((double)overallDist, (int)i));
	}
	sort(vec_result_score.begin(), vec_result_score.end(), [](const pair<double, int>& a, const pair<double, int>& b){ return a.first < b.first; });
	for_each(vec_result_score.begin(), vec_result_score.end(), [](const pair<double, int>& a){cout << a.first << ' ' << a.second << endl;});
	return vec_result_score;
}

vector<pair<double, int> > DigitRecognizer::cannyRecognize_primary(const Mat& inputImg)
{
	Mat img;
	resize(inputImg, img, Size(40, 60));
	morphologyEx(img, img, MORPH_CLOSE, getStructuringElement(MORPH_RECT,Size(3,3)));
	//Canny(img, img, 1, 2);
	//vertical
	const int midCol = 19;
	int col = midCol;
	int row;
	bool isConnected = false;
	int threshold = 100;
	vector<Point> vPoints, hPointsUpperLeft, hPointsLowerLeft, hPointsUpperRight, hPointsLowerRight;
	for (row = 0; row < 60; row++)
	{
		if (img.at<unsigned char>(Point(col, row)) > threshold)
		{
			if (!isConnected)
			{
				isConnected = true;
				vPoints.push_back(Point(col, row));
			}
		}
		else
		{
			isConnected = false;
		}
	}
	isConnected = false;
	row = 19;
	for (col = 0; col < 40; col++)
	{
		if (img.at<unsigned char>(Point(col, row)) > threshold)
		{
			if (!isConnected)
			{
				isConnected = true;
				if (col < midCol)
				{
					hPointsUpperLeft.push_back(Point(col, row));
				}
				else
				{
					hPointsUpperRight.push_back(Point(col, row));
				}
			}
		}
		else
		{
			isConnected = false;
		}
	}
	isConnected = false;
	row = 39;
	for (col = 0; col < 40; col++)
	{
		if (img.at<unsigned char>(Point(col, row)) > threshold)
		{
			if (!isConnected)
			{
				isConnected = true;
				if (col < midCol)
				{
					hPointsLowerLeft.push_back(Point(col, row));
				}
				else
				{
					hPointsLowerRight.push_back(Point(col, row));
				}
			}
		}
		else
		{
			isConnected = false;
		}
	}
	int digitLines[5];
	digitLines[0] = vPoints.size();
	digitLines[1] = hPointsUpperLeft.size();
	digitLines[2] = hPointsUpperRight.size();
	digitLines[3] = hPointsLowerLeft.size();
	digitLines[4] = hPointsLowerRight.size();
	static int templatePointsCount[9][5] = {
		{0, 0, 1, 0, 1},
		{3, 0, 1, 1, 0},
		{3, 0, 1, 0, 1},
		{1, 1, 1, 0, 1},
		{3, 1, 0, 0, 1},
		{3, 1, 0, 1, 1},
		{1, 0, 1, 0, 1},
		{3, 1, 1, 1, 1},
		{3, 1, 1, 0, 1}
	};
	vector<pair<double, int> > resVector;
	for (int i = 0; i < 9; i++)
	{
		int score = 0;
		for (int j = 0; j < 5; j++)
		{
			score += (digitLines[j] - templatePointsCount[i][j]) * (digitLines[j] - templatePointsCount[i][j]);
		}
		resVector.push_back(make_pair<double, int>((double)score, (int)(i + 1)));
	}
	sort(resVector.begin(), resVector.end(), [](const pair<double, int>& a, const pair<double, int>& b){ return a.first < b.first; });
	/*
	for (auto &a : resVector)
	{
		cout << a.first << ' ' << a.second << endl;
	}
	cout << "------------------------------------" << endl;
	*/
	return resVector;
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
		for (int j = 0; j < 10; j += step)
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
		for (int j = 0; j < 10; j += step)
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
		for (int j = 0; j < 10; j += step)
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
