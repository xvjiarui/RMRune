/*******************************************************************************************************************
Copyright 2017 Dajiang Innovations Technology Co., Ltd (DJI)

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
documentation files(the "Software"), to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense, and / or sell copies of the Software, and
to permit persons to whom the Software is furnished to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included in all copies or substantial portions of
the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.
*******************************************************************************************************************/

#include "RuneDetector.hpp"
#include <algorithm>

#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/calib3d/calib3d.hpp>
#include <opencv2/features2d/features2d.hpp>

#include <iostream>
#include <string>
#include <thread>
#include <cstdio>
#include <unordered_map>
#include "Voter.hpp"

using namespace cv;
using namespace std;

cv::Point2f MatDotPoint(cv::Mat M, const cv::Point2f &p)
{
	cv::Mat_<double> src(3 /*rows*/, 1 /* cols */);

	src(0, 0) = p.x;
	src(1, 0) = p.y;
	src(2, 0) = 1.0;

	cv::Mat_<double> dst = M * src; //USE MATRIX ALGEBRA
	return cv::Point2f(dst(0, 0), dst(1, 0));
}

pair<int, int> RuneDetector::getTarget(const cv::Mat &image, RuneType rune_type)
{
	cvtColor(image, src, CV_BGR2GRAY);
	Mat binary;
	//threshold(src, binary, contour_threshold, 255, THRESH_BINARY);
	GaussianBlur(src, binary, Size(13, 13), 0);
	morphologyEx(binary, binary, MORPH_CLOSE, getStructuringElement(MORPH_RECT,Size(3,3)));
	adaptiveThreshold(binary, binary, 255, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY, 5, 0);
//threshold(src, binary, 200, 255, THRESH_BINARY);
#ifdef SHOW_IMAGE
	imshow("image", image);
	imshow("binary", binary);
	waitKey(5);
#endif
	vector<vector<Point2i>> contours;
	vector<Vec4i> hierarchy;
	findContours(binary, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE);
#if defined(FINDRATIO)
	Mat show(image.size(), CV_8UC3, Scalar(0, 0, 0));
	vector<RotatedRect> contourRects;
	vector<pair<int, float>> contourRatios;
	RotatedRect rect;
	for (int i = 0, RectCount = 0; i < contours.size(); ++i)
	{
		rect = minAreaRect(contours[i]);
		if (rect.size.width == 0 || rect.size.height == 0)
			continue;
		rect = adjustRotatedRect(rect);
		contourRects.push_back(rect);
		contourRatios.push_back(make_pair(RectCount++, (float)rect.size.width / (float)rect.size.height));
	}
	sort(contourRatios.begin(), contourRatios.end(), [](const pair<int, float> &a, const pair<int, float> &b) { return a.second < b.second; });
	imshow("captured window", image);
	for (int i = 0, c1, c2, c3; i < contourRatios.size(); i++)
	{
		RotatedRect &rect = contourRects.at(contourRatios.at(i).first);
		// drawContours(show, contours, contourRatios.at(i).first, CV_RGB(rand() % 255, rand() % 255, rand() % 255), 3, CV_FILLED);
		c1 = rand() % 255;
		c2 = rand() % 255;
		c3 = rand() % 255;
		rectangle(show, contourRects.at(contourRatios.at(i).first).boundingRect(), Scalar(c1, c2, c3));
		putText(show, to_string(i), rect.center, FONT_HERSHEY_SIMPLEX, 0.5, Scalar(c1, c2, c3), 2);
		cout << i << ": " << contourRatios.at(i).second << ' ' << rect.size.width << ' ' << rect.size.height << ' ' << rect.angle  << ' ' << rect.center << endl;
		if ((i && i % 10 == 0) || (i == contourRatios.size() - 1))
		{
			imshow("contours", show);
			if (waitKey(0) == 'n')
			{
				break;
			}
			show = Mat(image.size(), CV_8UC3, Scalar(0, 0, 0));
		}
	}
	cout << "Next Frame" << endl;
	return make_pair(-1, -1);
#endif
	sudoku_rects.clear();
	digit_rects.clear();
	one_digit_rects.clear();
	if (checkSudoku(contours, sudoku_rects, rune_type))
	{
		if (rune_type == RUNE_B)
		{
			sudoku_imgs.clear();
			pair<int, int> idx = chooseMnistTarget(image, sudoku_rects);
			return idx;
		}
		else
		{
			if (use_perspective == true)
			{
				pair<int, int> idx = chooseTargetPerspective(src, sudoku_rects);
				return idx;
			}
			else
			{
				pair<int, int> idx = chooseTarget(src, sudoku_rects);
				return idx;
			}
		}
	}
	cout << "Fail" << endl;
	return make_pair(-1, -1);
}

bool RuneDetector::checkSudoku(const vector<vector<Point2i>> &contours, vector<RotatedRect> &sudoku_rects, RuneType rune_type)
{
	if (contours.size() < 9)
		return false;

	float ratio = 28.0 / 16.0;
	int sudoku = 0;

	float DigitWHRatio = (float)digit_width / (float)digit_height;
	float OneWHRatio = (float)digit_width / (float)one_height;

	float low_threshold = 0.6;
	float high_threshold = 1.2;
	vector<Point2f> centers;
	float digit_avg_width = 0;
	float digit_avg_height = 0;
	for (size_t i = 0; i < contours.size(); i++)
	{
		RotatedRect rect = minAreaRect(contours[i]);
		rect = adjustRotatedRect(rect);
		const Size2f &s = rect.size;
		float ratio_cur = s.width / s.height;

		// warning: temporary disable
		if (ratio_cur > 0.8 * ratio && ratio_cur < 1.2 * ratio &&
			s.width > low_threshold * sudoku_width && s.width < high_threshold * sudoku_width &&
			s.height > low_threshold * sudoku_height && s.height < high_threshold * sudoku_height &&
			((rect.angle > -10 && rect.angle < 10) || rect.angle < -170 || rect.angle > 170))
		{

			sudoku_rects.push_back(rect);
			centers.push_back(rect.center);
			++sudoku;
		}
		else if (ratio_cur > 0.6 * DigitWHRatio && ratio_cur < 1.4 * DigitWHRatio &&
				 s.width > 0.6 * digit_width && s.width < 1.4 * digit_width &&
				 s.height > 0.6 * digit_height && s.height < 1.4 * digit_height &&
				 (rect.angle > 60 || rect.angle < -60))
		{
			digit_rects.push_back(rect);
			digit_avg_width += s.height;
			digit_avg_height += s.width;
		}
		/*
		else if (ratio_cur > 0.6 * OneWHRatio && ratio_cur < 1.4 * OneWHRatio &&
				 s.width > 0.6 * digit_width && s.width < 1.4 * digit_width &&
				 s.height > 0.6 * one_height && s.height < 1.4 * one_height &&
				 (rect.angle > 60 || rect.angle < -60))
		{
			one_digit_rects.push_back(rect);
		}
		*/
	}
	/* 
	if (one_digit_rects.size() == 1)
	{
		digit_avg_width /= digit_rects.size();
		RotatedRect curRotatedRect = one_digit_rects.at(0);
		curRotatedRect.size.height = digit_avg_width;
		curRotatedRect.center -= Point2f(0.3 * digit_avg_width, 0);
		digit_rects.push_back(curRotatedRect);
	}
	*/
	oneIndex = -1;
	oneConfirmed = false;
	if (digit_rects.size() == 4)
	{
		digit_avg_width /= digit_rects.size();
		sort(digit_rects.begin(), digit_rects.end(), [](const RotatedRect& a, const RotatedRect& b) { return a.center.x < b.center.x;});
		float targetX = 0;
		RotatedRect curRotatedRect;
		float avgDistance = 0;
		int maxDistance = -1;
		vector<float> distance;
		for (int i = 0; i < digit_rects.size() - 1; i++)
		{
			float curDistance = digit_rects.at(i + 1).center.x - digit_rects.at(i).center.x;
			distance.push_back(curDistance);
			if (curDistance > maxDistance)
			{
				maxDistance = curDistance;
				oneIndex = i + 1;
			}
			avgDistance += curDistance;
		}
		avgDistance /= 3.0;
		float var = 0;
		for_each(distance.begin(), distance.end(), [&](const float& a){
			var += (a - avgDistance) * (a - avgDistance);
		});
		if (var > 225)
		{
			oneConfirmed = true;
			targetX = digit_rects.at(oneIndex - 1).center.x + avgDistance;
		}
		else
		{
			oneIndex = 0;
			targetX = digit_rects.at(0).center.x - avgDistance;
		}
		curRotatedRect = digit_rects.at(0);
		curRotatedRect.center.x  = targetX;
		digit_rects.push_back(curRotatedRect);
	}
#ifdef SHOW_DEBUG_DETAILS
	cout << sudoku << ' ' << digit_rects.size() << ' ' << one_digit_rects.size() << endl;
#endif

	if (sudoku > 15 || sudoku < 9)
	{
		cout << "Sudoku gg." << endl;
		return false;
	}
	if (rune_type == RUNE_B && (digit_rects.size() > 10 || digit_rects.size() < 4))
	{
		cout << "digits gg." << endl;
		return false;
	}

	if (sudoku > 9)
	{
		float **dist_map = new float *[sudoku];
		for (int i = 0; i < sudoku; i++)
		{
			dist_map[i] = new float[sudoku];
			for (int j = 0; j < sudoku; j++)
			{
				dist_map[i][j] = 0;
			}
		}
		// calculate distance of each cell center
		for (int i = 0; i < sudoku; ++i)
		{
			for (int j = i + 1; j < sudoku; ++j)
			{
				float d = sqrt((centers[i].x - centers[j].x) * (centers[i].x - centers[j].x) + (centers[i].y - centers[j].y) * (centers[i].y - centers[j].y));
				dist_map[i][j] = d;
				dist_map[j][i] = d;
			}
		}

		// choose the minimun distance cell as center cell
		int center_idx = 0;
		float min_dist = 100000000;
		for (int i = 0; i < sudoku; ++i)
		{
			float cur_d = 0;
			for (int j = 0; j < sudoku; ++j)
			{
				cur_d += dist_map[i][j];
			}
			if (cur_d < min_dist)
			{
				min_dist = cur_d;
				center_idx = i;
			}
		}

		// sort distance between each cell and the center cell
		vector<pair<float, int>> dist_center;
		for (int i = 0; i < sudoku; ++i)
		{
			dist_center.push_back(make_pair(dist_map[center_idx][i], i));
		}
		std::sort(dist_center.begin(), dist_center.end(), [](const pair<float, int> &p1, const pair<float, int> &p2) { return p1.first < p2.first; });

		for (int i = 0; i < sudoku; i++)
		{
			delete[] dist_map[i];
		}
		delete[] dist_map;

		// choose the nearest 9 cell as suduku
		vector<RotatedRect> sudoku_rects_temp;
		for (int i = 0; i < 9; ++i)
		{
			sudoku_rects_temp.push_back(sudoku_rects[dist_center[i].second]);
		}
		sudoku_rects_temp.swap(sudoku_rects);
	}
	if (sudoku_rects.size() != 9)
	{
		cout << "Sudoku gg." << endl;
		return false;
	}

	sort(sudoku_rects.begin(), sudoku_rects.end(), [](const RotatedRect& r1, const RotatedRect& r2) { return r1.center.y < r2.center.y;});
	sort(sudoku_rects.begin() + 0, sudoku_rects.begin() + 3, [](const RotatedRect& r1, const RotatedRect& r2) { return r1.center.x < r2.center.x;});
	sort(sudoku_rects.begin() + 3, sudoku_rects.begin() + 6, [](const RotatedRect& r1, const RotatedRect& r2) { return r1.center.x < r2.center.x;});
	sort(sudoku_rects.begin() + 6, sudoku_rects.begin() + 9, [](const RotatedRect& r1, const RotatedRect& r2) { return r1.center.x < r2.center.x;});
	if (digit_rects.size() > 5)
	{
		float **dist_map = new float *[digit_rects.size()];
		for (int i = 0; i < digit_rects.size(); i++)
		{
			dist_map[i] = new float[digit_rects.size()];
			for (int j = 0; j < digit_rects.size(); j++)
			{
				dist_map[i][j] = 0;
			}
		}
		// calculate distance of each cell center
		for (int i = 0; i < digit_rects.size(); ++i)
		{
			for (int j = i + 1; j < digit_rects.size(); ++j)
			{
				float d = sqrt((digit_rects.at(i).center.x - digit_rects.at(j).center.x) * (digit_rects.at(i).center.x - digit_rects.at(j).center.x) + ((digit_rects.at(i).center.y - digit_rects.at(j).center.y) * (digit_rects.at(i).center.y - digit_rects.at(j).center.y)));
				dist_map[i][j] = d;
				dist_map[j][i] = d;
			}
		}

		// choose the minimun distance cell as center cell
		int center_idx = 0;
		float min_dist = 100000000;
		for (int i = 0; i < digit_rects.size(); ++i)
		{
			float cur_d = 0;
			for (int j = 0; j < digit_rects.size(); ++j)
			{
				cur_d += dist_map[i][j];
			}
			if (cur_d < min_dist)
			{
				min_dist = cur_d;
				center_idx = i;
			}
		}
#ifdef SHOW_DEBUG_DETAILS
		cout << center_idx << endl;
#endif

		// sort distance between each cell and the center cell
		vector<pair<float, int>> dist_center;
		for (int i = 0; i < digit_rects.size(); ++i)
		{
			dist_center.push_back(make_pair(dist_map[center_idx][i], i));
		}
		std::sort(dist_center.begin(), dist_center.end(), [](const pair<float, int> &p1, const pair<float, int> &p2) { return p1.first < p2.first; });

		for (int i = 0; i < digit_rects.size(); i++)
		{
			delete[] dist_map[i];
		}
		delete[] dist_map;

		vector<RotatedRect> digit_rects_temp;
		for (int i = 0; i < 5; ++i)
		{
			digit_rects_temp.push_back(digit_rects[dist_center[i].second]);
		}
		digit_rects_temp.swap(digit_rects);
	}
	if (rune_type == RUNE_B && digit_rects.size() != 5)
	{
		cout << "Digit gg." << endl;
		return false;
	}
	return true;
}
#ifndef NoORB
int RuneDetector::findTargetORB(cv::Mat *cells)
{
	Mat descriptor[9];
	vector<vector<KeyPoint>> keypoints;
	keypoints.resize(9);
	ORB orb(100, 1, 1, 10, 0, 2, 1, 17);
	BFMatcher matcher(NORM_HAMMING, 0);
	int match_count[9][9] = {{0}};

	for (size_t i = 0; i < 9; i++)
	{
		vector<KeyPoint> &kp = keypoints[i];
		Mat &desp = descriptor[i];
		orb(cells[i], Mat(), kp, desp);
		//FAST(cell[idx], kp, 10);
		//Ptr<xfeatures2d::BriefDescriptorExtractor> brief = cv::xfeatures2d::BriefDescriptorExtractor::create();
		//brief->compute(cell[idx], kp, desp);

		if (desp.rows < 2)
			return -1;

		// feature matching
		for (size_t k = 0; k < i; k++)
		{
			vector<vector<DMatch>> matches;
			matcher.knnMatch(desp, descriptor[k], matches, 2);
			int cnt = 0;
			for (size_t n = 0; n < matches.size(); n++)
			{
				vector<DMatch> &m = matches[n];
				DMatch &dm1 = m[0];
				DMatch &dm2 = m[1];
				if (dm1.distance / dm2.distance < 0.8)
				{
					cnt++;
				}
			}
			match_count[i][k] = cnt;
			match_count[k][i] = cnt;
		}
	}

	// choose the minimun match cell as the target
	float avg_cnt[9] = {};
	int min_idx = -1;
	float min_cnt = 65535;
	for (size_t i = 0; i < 9; i++)
	{
		for (size_t j = 0; j < 9; j++)
		{
			avg_cnt[i] += match_count[i][j];
		}
		if (avg_cnt[i] < min_cnt)
		{
			min_cnt = avg_cnt[i];
			min_idx = i;
		}
	}
	return min_idx;
}
#endif

int RuneDetector::findTargetCanny(cv::Mat *cells)
{
	int min_count_idx = -1;
	int w_3 = cells[0].cols / 2.8;
	int w_23 = cells[0].cols * 2 / 3.0;
	double mid_ratio = 0.0;

	for (size_t i = 0; i < 9; i++)
	{
		int mid_area_count = 0;
		int black_count = 0;
		Mat edge;
		Canny(cells[i], edge, 20, 50);
		uchar *ptr = (uchar *)edge.data;

		for (size_t j = 0; j < cells[i].rows; ++j)
		{
			for (size_t k = 0; k < cells[i].cols; ++k, ++ptr)
			{
				int v = *ptr;
				if (v == 255)
					++black_count;

				if (k >= w_3 && k <= w_23)
					++mid_area_count;
			}
		}

		//cout << black_count << "  ";
		double cur_ratio = (double)mid_area_count / black_count;
		//cout << cur_ratio << "  ";
		if (mid_ratio < cur_ratio)
		{
			mid_ratio = cur_ratio;
			min_count_idx = i;
		}

	}
	return min_count_idx;
}

int RuneDetector::findTargetEdge(cv::Mat *cells)
{
	int grad_threshold = 10;
	int min_count = 65535;
	int min_count_idx = -1;
	for (size_t i = 0; i < 9; i++)
	{
		Mat gradX, gradY;
		cv::Sobel(cells[i], gradX, CV_16S, 1, 0);
		cv::Sobel(cells[i], gradY, CV_16S, 0, 1);

		int large_grad_count = 0;
		short *ptr_x = (short *)gradX.data;
		short *ptr_y = (short *)gradY.data;

		for (size_t j = 0; j < gradX.rows; ++j)
		{
			for (size_t k = 0; k < gradX.cols; ++k, ++ptr_x, ++ptr_y)
			{
				int x = abs(*ptr_x);
				int y = abs(*ptr_y);
				if (x > grad_threshold || y > grad_threshold)
					++large_grad_count;
			}
		}

		if (min_count > large_grad_count)
		{
			min_count = large_grad_count;
			min_count_idx = i;
		}
	}
	return min_count_idx;
}

#ifdef ADJUST_THRESHOLD
struct ThresholdStruct
{
	Mat* data;
	MnistRecognizer* mnistRecognizer;
};

void AdjustThreshold(int t, void* d)
{
	ThresholdStruct* data = (ThresholdStruct*)d;
	Mat temp;
	threshold(*(data->data), temp, t, 255, THRESH_BINARY);
	imshow("AdjustThreshold", temp);
	resize(temp, temp, Size(28, 28));
	cout << t << ":" <<  data->mnistRecognizer->recognize(temp) << endl;
}
#endif

pair<int, int> RuneDetector::chooseMnistTarget(const Mat &inputImg, const vector<RotatedRect> &sudoku_rects)
{
	// get 9(cell) X 4(corner) corner, and 9 cell's center
	
	Mat image;
	cvtColor(inputImg, image, CV_BGR2GRAY);
	vector<Point2fWithIdx> centers;
	vector<Point2f> corner;
	Point2f center_avg;
	for (size_t i = 0; i < sudoku_rects.size(); i++)
	{
		const RotatedRect &rect = sudoku_rects[i];
		Point2f vecs[4];
		rect.points(vecs);
		for (size_t j = 0; j < 4; j++)
		{
			corner.push_back(vecs[j]);
		}
		centers.push_back(Point2fWithIdx(rect.center, i));
		center_avg += rect.center;
	}

	center_avg = Point2f(center_avg.x / sudoku_rects.size(), center_avg.y / sudoku_rects.size());

	// arange sudoku cell to following order
	// 0  1  2
	// 3  4  5
	// 6  7  8
	
	sort(centers.begin(), centers.end(), [](const Point2fWithIdx &p1, const Point2fWithIdx &p2) { return p1.p.y < p2.p.y; });
	sort(centers.begin() + 0, centers.begin() + 3, [](const Point2fWithIdx &p1, const Point2fWithIdx &p2) { return p1.p.x < p2.p.x; });
	sort(centers.begin() + 3, centers.begin() + 6, [](const Point2fWithIdx &p1, const Point2fWithIdx &p2) { return p1.p.x < p2.p.x; });
	sort(centers.begin() + 6, centers.begin() + 9, [](const Point2fWithIdx &p1, const Point2fWithIdx &p2) { return p1.p.x < p2.p.x; });

	// get position of [0,2,6,8] corner
	int corner_idx[] = {0, 2, 6, 8};
	vector<Point2f> corner_0268;
	for (size_t i = 0; i < 4; i++)
	{
		size_t k = centers[corner_idx[i]].idx * 4;
		for (size_t j = 0; j < 4; j++)
		{
			corner_0268.push_back(corner[k + j]);
		}
	}

	
	// find approx corner of sudoku
	RotatedRect rect = minAreaRect(corner_0268);
	Point2f vertices[4];
	rect.points(vertices);
	Point2f lu, ld, ru, rd;
	sort(vertices, vertices + 4, [](const Point2f &p1, const Point2f &p2) { return p1.x < p2.x; });
	if (vertices[0].y < vertices[1].y)
	{
		lu = vertices[0];
		ld = vertices[1];
	}
	else
	{
		lu = vertices[1];
		ld = vertices[0];
	}
	if (vertices[2].y < vertices[3].y)
	{
		ru = vertices[2];
		rd = vertices[3];
	}
	else
	{
		ru = vertices[3];
		rd = vertices[2];
	}

	// find actual corner of sudoku
	Point2f _lu, _ld, _ru, _rd;
	float mlu = 10000.0, mld = 10000.0, mru = 10000.0, mrd = 10000.0;
	for (size_t i = 0; i < corner_0268.size(); i++)
	{
		const Point2f &p = corner_0268[i];
		float v1 = (p - lu).dot((p - lu));
		float v2 = (p - ld).dot((p - ld));
		float v3 = (p - ru).dot((p - ru));
		float v4 = (p - rd).dot((p - rd));
		if (v1 < mlu)
		{
			mlu = v1;
			_lu = p;
		}
		if (v2 < mld)
		{
			mld = v2;
			_ld = p;
		}
		if (v3 < mru)
		{
			mru = v3;
			_ru = p;
		}
		if (v4 < mrd)
		{
			mrd = v4;
			_rd = p;
		}
	}

	// applies a perspective transformation to an image
	float _width = max((_lu - _ru).dot(_lu - _ru), (_ld - _rd).dot(_ld - _rd));
	float _height = max((_lu - _ld).dot(_lu - _ld), (_rd - _ru).dot(_rd - _ru));
	_width = sqrtf(_width);
	_height = sqrtf(_height);

	vector<Point2f> src_p;
	src_p.push_back(_lu);
	src_p.push_back(_ld);
	src_p.push_back(_ru);
	src_p.push_back(_rd);

	vector<Point2f> dst_p;
	dst_p.push_back(Point2f(0.0, 0.0));
	dst_p.push_back(Point2f(0.0, _height));
	dst_p.push_back(Point2f(_width, 0.0));
	dst_p.push_back(Point2f(_width, _height));

	Mat perspective_mat = getPerspectiveTransform(src_p, dst_p);
	Mat image_persp;
	warpPerspective(image, image_persp, perspective_mat, Size(_width, _height));
#ifdef SHOW_IMAGE
	imshow("image_persp", image_persp);
#endif

	dst_p.clear();
	dst_p.push_back(_lu + Point2f(0.0, 0.0));
	dst_p.push_back(_lu + Point2f(0.0, _height));
	dst_p.push_back(_lu + Point2f(_width, 0.0));
	dst_p.push_back(_lu + Point2f(_width, _height));
	perspective_mat = getPerspectiveTransform(src_p, dst_p);
	Point2f perspective_center = MatDotPoint(perspective_mat, center_avg);
#ifdef ADAPTIVE_THRESHOLD_IN_DIGIT
	Mat binary, perspective_image;
	cvtColor(inputImg, binary, CV_BGR2GRAY);
	GaussianBlur(binary, binary, Size(9, 9), 0);
	morphologyEx(binary, binary, MORPH_CLOSE, getStructuringElement(MORPH_RECT,Size(3,3)));
	adaptiveThreshold(binary, binary, 255, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY, 5, 0);
	warpPerspective(binary, perspective_image, perspective_mat, inputImg.size());
#else
	Mat perspective_image;
	warpPerspective(inputImg, perspective_image, perspective_mat, inputImg.size());
#endif
	vector<Rect> digitBoundingRect;
	sort(digit_rects.begin(), digit_rects.end(), [](const RotatedRect &a, const RotatedRect &b) { return a.center.x < b.center.x; });
	vector<Mat> digit_images;
	vector<vector<pair<double, int> > > digit_scores(5);
	cout << "[" << endl;
	for (int i = 0, writeI = 0; i < digit_rects.size(); i++)
	{
		Point2f pts[4];
		vector<Point2f> vpts, t;
		digit_rects[i].points(pts);
		for_each(pts, (pts + 4), [&](const Point2f &a) { vpts.push_back(a); });
		perspectiveTransform(vpts, t, perspective_mat);
		digit_images.push_back(perspective_image(boundingRect(t)));
#ifdef SHOW_IMAGE
		imshow("showtime", digit_images.at(i));
#endif
		if (i == oneIndex && oneConfirmed)
		{
			digit_scores.at(writeI).clear();
			digit_scores.at(writeI).push_back(make_pair<double, int>(100, 1));
		}
		else
		{
			digit_scores.at(writeI) = digitRecognizer.process_primary(digit_images.at(i));
		}
		if (i == oneIndex && !digit_scores.at(writeI).size())
		{
			digit_scores.back().clear();
			digit_scores.back().push_back(make_pair<double, int>(100, 1));
		}
		else
		{
			/*
			if (digit_scores.size())
			{
				digit_results.at(writeI) = curDigit;
			}
			else
			{
				digit_results.at(writeI) = 1;
			}
			*/
			writeI++;
		}
	}
	for (auto &a : digit_scores)
	{
		if (!a.size())
		{
			cout << endl;
			return make_pair(-1, -1);
		}
		cout << a.at(0).second;
	}
	cout << endl;
	getUniqueResult(digit_scores, digit_results);
	for_each(digit_results.begin(), digit_results.end(), [](const int& a) { cout << a;});
	cout << "]";
#ifdef SHOW_DEBUG_DETAILS
	cout << endl;
#else
	cout << ' ';
#endif

	static Voter<vector<int>> digitVoter(voteSetting.saveTime);
	static vector<int> lastDigitResult;
	static int curShootIdx = 0;

	digitVoter.PushElement(digit_results);

	// calculate the average width and hieght of each cell
	const double *pdata = (double *)perspective_mat.data;
	float height_avg = 0.0, width_avg = 0.0;
	for (size_t i = 0; i < sudoku_rects.size(); ++i)
	{
		vector<Point2f> vec_p;
		for (size_t j = 0; j < 4; j++)
		{
			const Point2f &p = corner[i * 4 + j];
			float x = pdata[0] * p.x + pdata[1] * p.y + pdata[2];
			float y = pdata[3] * p.x + pdata[4] * p.y + pdata[5];
			float s = pdata[6] * p.x + pdata[7] * p.y + pdata[8];
			vec_p.push_back(Point2f(x / s, y / s));
		}
		Rect2f r = boundingRect(vec_p);
		height_avg += r.height;
		width_avg += r.width;
	}
	height_avg /= 9.0;
	width_avg /= 9.0;

	if (height_avg > _height / 3)
		height_avg = 0.25 * _height;
	if (width_avg > _width / 3)
		width_avg = 0.25 * _width;

	// get image of every cell, then compute ORB feature and match feature;
	int cell_width = 0.98 * width_avg;
	int cell_height = height_avg;
	int half_w_gap = (width_avg - cell_width) / 2, half_h_gap = (height_avg - cell_height) / 2;
	half_w_gap = 0;
	half_h_gap = 0;
	int offset_x = 0;
	int offset_y = 0;
	int width_start[] = {half_w_gap, int((_width - cell_width) / 2), int(_width - cell_width - half_w_gap)};
	int height_start[] = {half_h_gap, int((_height - cell_height) / 2), int(_height - cell_height - half_h_gap)};

	vector<vector<pair<double, int>>> results(9);
	vector<thread> mnistThreads;

#ifdef ADJUST_THRESHOLD
	bool goToNextFrame = false;
#endif

	for (size_t i = 0; i < 3; i++)
	{
		for (size_t j = 0; j < 3; j++)
		{
			size_t idx = i * 3 + j;
			Rect cell_roi(width_start[j] + offset_x, height_start[i] + offset_y, cell_width, cell_height);
			Mat temp;
			image_persp(cell_roi).copyTo(temp);
#ifdef ADJUST_THRESHOLD
			if (!goToNextFrame)
			{
				ThresholdStruct data;
				data.data = &temp;
				data.mnistRecognizer = &mnistRecognizer[0];
				namedWindow("AdjustThreshold", WINDOW_NORMAL);
				createTrackbar("Adjust Threshold", "AdjustThreshold", &mnist_threshold, 255, AdjustThreshold, (void*)&data);
				AdjustThreshold(mnist_threshold,(void*)&data);
				if (waitKey(0) == 'n'){
					cout << "Next frame" << endl;
					goToNextFrame = true;
				}
			}
#endif
			threshold(temp, temp, mnist_threshold, 255, THRESH_BINARY);
			resize(temp, temp, Size(28, 28));
			sudoku_imgs.push_back(temp);
		}
	}
	for (size_t i = 0; i < sudoku_imgs.size(); i++)
	{

		mnistThreads.push_back(thread([&, i]() {
			results.at(i) = mnistRecognizer[i].recognize_primary(sudoku_imgs.at(i));
		}));
	}

	for (auto &t : mnistThreads)
	{
		t.join();
	}

/*

	cout << "(";
	for (int i = 0; i < FinalResults.size(); i++)
	{
		//sudoku_indexs.at(FinalResults.at(i)->second) = i;
		cout << FinalResults.at(i)->second;
		mnistResult.push_back(FinalResults.at(i)->second);
	}
	cout << ")" << endl;
	*/
	vector<int> mnistResult;
	getUniqueResult(results, mnistResult);
	cout << "(";
	for_each(mnistResult.begin(), mnistResult.end(), [](const int& a) { cout << a;});
	cout << ")" << endl;

	static Voter<vector<int>> mnistVoter(voteSetting.saveTime);
	static vector<int> lastMnistResult;

	mnistVoter.PushElement(mnistResult);

#ifdef SHOW_DEBUG_DETAILS
	cout << "curShootIdx: " << curShootIdx << endl;
#endif

#ifndef NO_VOTING
	if (digitVoter.GetBestElement(digit_results) && mnistVoter.GetBestElement(mnistResult))
	{
#ifdef OPTIMIZE_VOTING
		if (mnistResult == lastMnistResult)
		{
			cout << "pass" << endl;
			return make_pair(-1, -1);
		}
#endif
		if (lastDigitResult != digit_results)
		{
			curShootIdx = 0;
		}
		else
		{
			if (++curShootIdx >= 5)
				curShootIdx = 0;
		}
		lastDigitResult = digit_results;
		digitVoter.RemoveOldElements(/*voteSetting.saveTime*/);
		lastMnistResult = mnistResult;
		mnistVoter.RemoveOldElements(/*voteSetting.saveTime*/);
	}
	else
	{
		return make_pair(-1, -1);
	}

	cout << "\"";
	for (int i = 0; i < lastMnistResult.size(); i++)
	{
		sudoku_indexs.at(lastMnistResult.at(i)) = i;
		cout << lastMnistResult.at(i);
	}
	cout << "\"" << endl;

	cout << "{";
	for (int i = 0; i < lastDigitResult.size(); i++)
	{
		cout << getSudokuIndex(lastDigitResult.at(i));
	}
	cout << "}" << endl;
	cout << "----------------------------" << endl;
	cout << curShootIdx << ' ' << lastDigitResult.at(curShootIdx) << ' ' << getSudokuIndex(lastDigitResult.at(curShootIdx)) << endl;
	cout << "----------------------------" << endl;

	return make_pair(1, getSudokuIndex(lastDigitResult.at(curShootIdx)));
#endif
	return make_pair(1, getSudokuIndex(digit_results.at(0)));
}

void RuneDetector::getUniqueResult(vector<vector<pair<double, int> > >& results, vector<int>& uniqueResults)
{
	vector<vector<pair<double, int>>::iterator> FinalResults;
	deque<int> check_index;

	for (int i = 0; i < results.size(); i++)
	{
		FinalResults.push_back(results.at(i).begin());
		check_index.push_back(i);
	}

	for (auto idxitr1 = check_index.begin(); idxitr1 != check_index.end(); idxitr1++)
	{
		for (int j = 0; j < results.size(); j++)
		{
			if (*idxitr1 == j)
				continue;
			int i = *idxitr1;
			if (FinalResults.at(i)->second == FinalResults.at(j)->second)
			{
				if ((FinalResults.at(i)) + 1 != results.at(i).end() && (*(FinalResults.at(i))).first < (*(FinalResults.at(j))).first)
				{
					(FinalResults.at(i))++;
					check_index.insert(idxitr1 + 1, *idxitr1);
					break; //recompare from beginning
				}
				else
				{
					if (FinalResults.at(j) + 1 == results.at(j).end())
					{
						(FinalResults.at(i))++;
						check_index.insert(idxitr1 + 1, *idxitr1);
						break; //recompare from beginning
					}
					(FinalResults.at(j))++;
					check_index.insert(idxitr1 + 1, j);
				}
			}
		}
	}
	uniqueResults.clear();
	for (auto &itr : FinalResults)
	{
		uniqueResults.push_back(itr->second);
	}
}

pair<int, int> RuneDetector::chooseTargetPerspective(const Mat &image, const vector<RotatedRect> &sudoku_rects)
{
	// get 9(cell) X 4(corner) corner, and 9 cell's center
	vector<Point2fWithIdx> centers;
	vector<Point2f> corner;
	for (size_t i = 0; i < sudoku_rects.size(); i++)
	{
		const RotatedRect &rect = sudoku_rects[i];
		Point2f vecs[4];
		rect.points(vecs);
		for (size_t j = 0; j < 4; j++)
		{
			corner.push_back(vecs[j]);
		}
		centers.push_back(Point2fWithIdx(rect.center, i));
	}

	// arange sudoku cell to following order
	// 0  1  2
	// 3  4  5
	// 6  7  8
	sort(centers.begin(), centers.end(), [](const Point2fWithIdx &p1, const Point2fWithIdx &p2) { return p1.p.y < p2.p.y; });
	sort(centers.begin() + 0, centers.begin() + 3, [](const Point2fWithIdx &p1, const Point2fWithIdx &p2) { return p1.p.x < p2.p.x; });
	sort(centers.begin() + 3, centers.begin() + 6, [](const Point2fWithIdx &p1, const Point2fWithIdx &p2) { return p1.p.x < p2.p.x; });
	sort(centers.begin() + 6, centers.begin() + 9, [](const Point2fWithIdx &p1, const Point2fWithIdx &p2) { return p1.p.x < p2.p.x; });

	// get position of [0,2,6,8] corner
	int corner_idx[] = {0, 2, 6, 8};
	vector<Point2f> corner_0268;
	for (size_t i = 0; i < 4; i++)
	{
		size_t k = centers[corner_idx[i]].idx * 4;
		for (size_t j = 0; j < 4; j++)
		{
			corner_0268.push_back(corner[k + j]);
		}
	}

	// find approx corner of sudoku
	RotatedRect rect = minAreaRect(corner_0268);
	Point2f vertices[4];
	rect.points(vertices);
	Point2f lu, ld, ru, rd;
	sort(vertices, vertices + 4, [](const Point2f &p1, const Point2f &p2) { return p1.x < p2.x; });
	if (vertices[0].y < vertices[1].y)
	{
		lu = vertices[0];
		ld = vertices[1];
	}
	else
	{
		lu = vertices[1];
		ld = vertices[0];
	}
	if (vertices[2].y < vertices[3].y)
	{
		ru = vertices[2];
		rd = vertices[3];
	}
	else
	{
		ru = vertices[3];
		rd = vertices[2];
	}

	// find actual corner of sudoku
	Point2f _lu, _ld, _ru, _rd;
	float mlu = 10000.0, mld = 10000.0, mru = 10000.0, mrd = 10000.0;
	for (size_t i = 0; i < corner_0268.size(); i++)
	{
		const Point2f &p = corner_0268[i];
		float v1 = (p - lu).dot((p - lu));
		float v2 = (p - ld).dot((p - ld));
		float v3 = (p - ru).dot((p - ru));
		float v4 = (p - rd).dot((p - rd));
		if (v1 < mlu)
		{
			mlu = v1;
			_lu = p;
		}
		if (v2 < mld)
		{
			mld = v2;
			_ld = p;
		}
		if (v3 < mru)
		{
			mru = v3;
			_ru = p;
		}
		if (v4 < mrd)
		{
			mrd = v4;
			_rd = p;
		}
	}

	// applies a perspective transformation to an image
	float _width = max((_lu - _ru).dot(_lu - _ru), (_ld - _rd).dot(_ld - _rd));
	float _height = max((_lu - _ld).dot(_lu - _ld), (_rd - _ru).dot(_rd - _ru));
	_width = sqrtf(_width);
	_height = sqrtf(_height);

	vector<Point2f> src_p;
	src_p.push_back(_lu);
	src_p.push_back(_ld);
	src_p.push_back(_ru);
	src_p.push_back(_rd);

	vector<Point2f> dst_p;
	dst_p.push_back(Point2f(0.0, 0.0));
	dst_p.push_back(Point2f(0.0, _height));
	dst_p.push_back(Point2f(_width, 0.0));
	dst_p.push_back(Point2f(_width, _height));

	Mat perspective_mat = getPerspectiveTransform(src_p, dst_p);

	Mat image_persp;
	warpPerspective(image, image_persp, perspective_mat, Size(_width, _height));
	// calculate the average width and hieght of each cell
	const double *pdata = (double *)perspective_mat.data;
	float height_avg = 0.0, width_avg = 0.0;
	for (size_t i = 0; i < sudoku_rects.size(); ++i)
	{
		vector<Point2f> vec_p;
		for (size_t j = 0; j < 4; j++)
		{
			const Point2f &p = corner[i * 4 + j];
			float x = pdata[0] * p.x + pdata[1] * p.y + pdata[2];
			float y = pdata[3] * p.x + pdata[4] * p.y + pdata[5];
			float s = pdata[6] * p.x + pdata[7] * p.y + pdata[8];
			vec_p.push_back(Point2f(x / s, y / s));
		}
		Rect2f r = boundingRect(vec_p);
		height_avg += r.height;
		width_avg += r.width;
	}
	height_avg /= 9.0;
	width_avg /= 9.0;

	if (height_avg > _height / 3)
		height_avg = 0.25 * _height;
	if (width_avg > _width / 3)
		width_avg = 0.25 * _width;

	// get image of every cell, then compute ORB feature and match feature;
	int cell_width = 0.48 * width_avg + 0.5;
	int cell_height = 0.50 * height_avg + 0.5;
	int half_w_gap = (width_avg - cell_width) / 2, half_h_gap = (height_avg - cell_height) / 2;
	int offset_x = 0.05 * cell_width + 0.5;
	int offset_y = 0.05 * cell_height + 0.5;
	int width_start[] = {half_w_gap, int((_width - cell_width) / 2), int(_width - cell_width - half_w_gap)};
	int height_start[] = {half_h_gap, int((_height - cell_height) / 2), int(_height - cell_height - half_h_gap)};

	Mat cell[9];
	for (size_t i = 0; i < 3; i++)
	{
		for (size_t j = 0; j < 3; j++)
		{
			size_t idx = i * 3 + j;
			Rect cell_roi(width_start[j] + offset_x, height_start[i] + offset_y, cell_width, cell_height);
			cout << cell_roi << endl;
			image_persp(cell_roi).copyTo(cell[idx]);
		}
	}

	int idx = -1;
	
#ifndef NoORB
	if (type == RUNE_ORB)
		idx = findTargetORB(cell);
	else
#endif
	if (type == RUNE_GRAD)
		idx = findTargetEdge(cell);
	else if (type == RUNE_CANNY)
		idx = findTargetCanny(cell);

	//int idxx = findTargetCanny(cell);
	//cout << "Canny: " << idxx << "\tEDGE: " << idx << endl;
	static Voter<int> indexVoter(voteSetting.SRSaveTime);	
	static int lastIndexResult = -1;
	indexVoter.PushElement(idx);
	if (indexVoter.GetBestElement(idx))
	{
		if(lastIndexResult == idx)
		{
			cout << "Pass" << endl;
			return make_pair(-1, -1);
		}
		lastIndexResult = idx;
		indexVoter.RemoveOldElements();
	}
	else return make_pair(-1, -1);
	// return idx < 0 ? make_pair(-1, -1) : make_pair((int)centers[idx].idx, idx);
	return idx < 0 ? make_pair(-1, -1) : make_pair(idx, (int)centers[idx].idx);
}

pair<int, int> RuneDetector::chooseTarget(const Mat &image, const vector<RotatedRect> &sudoku_rects)
{
	// get 9(cell) X 4(corner) corner, and 9 cell's center
	vector<Point2fWithIdx> centers;
	vector<Point2f> corner;
	for (size_t i = 0; i < sudoku_rects.size(); i++)
	{
		const RotatedRect &rect = sudoku_rects[i];
		Point2f vecs[4];
		rect.points(vecs);
		for (size_t j = 0; j < 4; j++)
		{
			corner.push_back(vecs[j]);
		}
		centers.push_back(Point2fWithIdx(rect.center, i));
	}

	// arange sudoku cell to following order
	// 0  1  2
	// 3  4  5
	// 6  7  8
	sort(centers.begin(), centers.end(), [](const Point2fWithIdx &p1, const Point2fWithIdx &p2) { return p1.p.y < p2.p.y; });
	sort(centers.begin() + 0, centers.begin() + 3, [](const Point2fWithIdx &p1, const Point2fWithIdx &p2) { return p1.p.x < p2.p.x; });
	sort(centers.begin() + 3, centers.begin() + 6, [](const Point2fWithIdx &p1, const Point2fWithIdx &p2) { return p1.p.x < p2.p.x; });
	sort(centers.begin() + 6, centers.begin() + 9, [](const Point2fWithIdx &p1, const Point2fWithIdx &p2) { return p1.p.x < p2.p.x; });

	Mat cell[9];

	for (size_t i = 0; i < 3; i++)
	{
		for (size_t j = 0; j < 3; j++)
		{
			size_t idx = i * 3 + j;
			Rect cell_roi = sudoku_rects[centers[idx].idx].boundingRect();
			int margin_x = 0.25 * cell_roi.width;
			int margin_y = 0.25 * cell_roi.height;
			Rect scale_roi = Rect(cell_roi.x + margin_x, cell_roi.y + margin_y, cell_roi.width - 2 * margin_x, cell_roi.height - 2 * margin_y);
			image(scale_roi).copyTo(cell[idx]);
		}
	}

	int idx = -1;
	
#ifndef NoORB
	if (type == RUNE_ORB)
	    idx = findTargetORB(cell);
	else
#endif
	if (type == RUNE_GRAD)
		idx = findTargetEdge(cell);
	else if (type == RUNE_CANNY)
		idx = findTargetCanny(cell);

	//int idxx = findTargetCanny(cell);
	//cout << "Canny: " << idxx << "\tEDGE: " << idx << endl;
	static Voter<int> indexVoter(voteSetting.SRSaveTime);	
	static int lastIndexResult = -1;
	indexVoter.PushElement(idx);
	if (indexVoter.GetBestElement(idx))
	{
		if(lastIndexResult == idx)
		{
			cout << "Pass" << endl;
			return make_pair(-1, -1);
		}
		lastIndexResult = idx;
		indexVoter.RemoveOldElements();
	}
	else return make_pair(-1, -1);
	// return idx < 0 ? make_pair(-1, -1) : make_pair((int)centers[idx].idx, idx);
	return idx < 0 ? make_pair(-1, -1) : make_pair(idx, (int)centers[idx].idx);
}

RotatedRect RuneDetector::adjustRotatedRect(const RotatedRect &rect)
{
	const Size2f &s = rect.size;
	if (s.width > s.height)
		return rect;
	return RotatedRect(rect.center, Size2f(s.height, s.width), rect.angle + 90.0);
}
