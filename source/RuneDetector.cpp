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

#define SHOW_IMAGE

using namespace cv;
using namespace std;

cv::Point2f MatDotPoint(cv::Mat M, const cv::Point2f& p)
{
	cv::Mat_<double> src(3/*rows*/, 1 /* cols */);

	src(0, 0) = p.x;
	src(1, 0) = p.y;
	src(2, 0) = 1.0;

	cv::Mat_<double> dst = M * src; //USE MATRIX ALGEBRA
	return cv::Point2f(dst(0, 0), dst(1, 0));
}

pair<int, int> RuneDetector::getTarget(const cv::Mat & image, RuneType rune_type) {
	cvtColor(image, src, CV_BGR2GRAY);
	Mat binary;
	threshold(src, binary, 150, 255, THRESH_BINARY);
	//threshold(src, binary, 200, 255, THRESH_BINARY);
#ifdef SHOW_IMAGE
	imshow("binary", binary);
#endif
	vector<vector<Point2i>> contours;
	vector<Vec4i> hierarchy;
	findContours(binary, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE);
#ifdef SHOW_IMAGE
	Mat show(image.size(), CV_8UC3, Scalar(0,0,0));
	for (int i = 0; i < contours.size(); ++i) {
		drawContours(show, contours, i, CV_RGB(rand() % 255, rand() % 255, rand() % 255), 3, CV_FILLED);
	}
	imshow("contours", show);
#endif
	sudoku_rects.clear();
	if (checkSudoku(contours, sudoku_rects))
	{
		if (rune_type == RUNE_B)
		{
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

bool RuneDetector::checkSudoku(const vector<vector<Point2i>> & contours, vector<RotatedRect> & sudoku_rects) {
	if (contours.size() < 9)
		return false;

	float width = sudoku_width;
	float height = sudoku_height;
	float ratio = 28.0 / 16.0;
	int sudoku = 0;

	float DigitHWRatio = runesetting.DigitWidth / runesetting.DigitHeight;
	DigitHWRatio = 1.7;

	float low_threshold = 0.6;
	float high_threshold = 1.2;
	vector<Point2f> centers;
	for (size_t i = 0; i < contours.size(); i++) {
		RotatedRect rect = minAreaRect(contours[i]);
		rect = adjustRotatedRect(rect);
		const Size2f & s = rect.size;
		float ratio_cur = s.width / s.height;

// warning: temporary disable
		if (ratio_cur > 0.8 * ratio && ratio_cur < 1.2 * ratio &&
		        s.width > low_threshold * width && s.width < high_threshold * width &&
		        s.height > low_threshold * height && s.height < high_threshold * height &&
		        ((rect.angle > -10 && rect.angle < 10) || rect.angle < -170 || rect.angle > 170)) {

			sudoku_rects.push_back(rect);
			centers.push_back(rect.center);
			//vector<Point2i> poly;
			//approxPolyDP(contours[i], poly, 20, true);
			++sudoku;
		} 
		else if (ratio_cur > 0.8 * DigitHWRatio && ratio_cur < 1.2 * DigitHWRatio &&
					 s.width > 0.8  * runesetting.DigitWidth * runesetting.DigitRatio && s.width < 1.2 * runesetting.DigitWidth * runesetting.DigitRatio &&
					 s.height > 0.8 * runesetting.DigitHeight * runesetting.DigitRatio && s.height < 1.2 * runesetting.DigitHeight * runesetting.DigitRatio)
		        // ((rect.angle > -10 && rect.angle < 10) || rect.angle < -170 || rect.angle > 170))
		{
			digit_rects.push_back(rect);
		}
		
		// else {
		// 	//the debugging code to quickly find suitable ratio and width which is prepared for possible debug
		// 	Mat show(Size(480, 640), CV_8UC3, Scalar(0,0,0));
		// 	drawContours(show, contours, i, CV_RGB(rand() % 255, rand() % 255, rand() % 255), 3, CV_FILLED);
		// 	imshow("hi", show);
		// 	cout << ratio_cur << ' ' << s.width << ' ' << s.height << endl;
		// 	waitKey(0);
		// }
		
	}

	if (sudoku > 15 || sudoku < 9 || digit_rects.size() > 10 || digit_rects.size() < 4)
		return false;

	if (sudoku > 9) {
		float** dist_map = new float*[sudoku];
		for (int i = 0; i < sudoku; i++) {
			dist_map[i] = new float[sudoku];
			for (int j = 0; j < sudoku; j++) {
				dist_map[i][j] = 0;
			}
		}
		// calculate distance of each cell center
		for (int i = 0; i < sudoku; ++i) {
			for (int j = i + 1; j < sudoku; ++j) {
				float d = sqrt((centers[i].x - centers[j].x) * (centers[i].x - centers[j].x) + (centers[i].y - centers[j].y) * (centers[i].y - centers[j].y));
				dist_map[i][j] = d;
				dist_map[j][i] = d;
			}
		}

		// choose the minimun distance cell as center cell
		int center_idx = 0;
		float min_dist = 100000000;
		for (int i = 0; i < sudoku; ++i) {
			float cur_d = 0;
			for (int j = 0; j < sudoku; ++j) {
				cur_d += dist_map[i][j];
			}
			if (cur_d < min_dist) {
				min_dist = cur_d;
				center_idx = i;
			}
		}

		// sort distance between each cell and the center cell
		vector<pair<float, int> > dist_center;
		for (int i = 0; i < sudoku; ++i) {
			dist_center.push_back(make_pair(dist_map[center_idx][i], i));
		}
		std::sort(dist_center.begin(), dist_center.end(), [](const pair<float, int> & p1, const pair<float, int> & p2) { return p1.first < p2.first; });

		for (int i = 0; i < sudoku; i++) {
			delete [] dist_map[i];
		}
		delete [] dist_map;

		// choose the nearest 9 cell as suduku
		vector<RotatedRect> sudoku_rects_temp;
		for (int i = 0; i < 9; ++i) {
			sudoku_rects_temp.push_back(sudoku_rects[dist_center[i].second]);
		}
		sudoku_rects_temp.swap(sudoku_rects);
		
    }
    cout << "sudoku n: " << sudoku_rects.size()  << endl;

	if (digit_rects.size() > 5)
	{
		float** dist_map = new float*[digit_rects.size()];
		for (int i = 0; i < digit_rects.size(); i++) {
			dist_map[i] = new float[digit_rects.size()];
			for (int j = 0; j < digit_rects.size(); j++) {
				dist_map[i][j] = 0;
			}
		}
		// calculate distance of each cell center
		for (int i = 0; i < digit_rects.size(); ++i) {
			for (int j = i + 1; j < digit_rects.size(); ++j) {
				float d = sqrt((digit_rects.at(i).center.x - digit_rects.at(j).center.x) * (digit_rects.at(i).center.x - digit_rects.at(j).center.x) + ((digit_rects.at(i).center.y - digit_rects.at(j).center.y) * (digit_rects.at(i).center.y - digit_rects.at(j).center.y)));
				dist_map[i][j] = d;
				dist_map[j][i] = d;
			}
		}

		// choose the minimun distance cell as center cell
		int center_idx = 0;
		float min_dist = 100000000;
		for (int i = 0; i < digit_rects.size(); ++i) {
			float cur_d = 0;
			for (int j = 0; j < digit_rects.size(); ++j) {
				cur_d += dist_map[i][j];
			}
			if (cur_d < min_dist) {
				min_dist = cur_d;
				center_idx = i;
			}
		}
		cout << center_idx << endl;


		// sort distance between each cell and the center cell
		vector<pair<float, int> > dist_center;
		for (int i = 0; i < digit_rects.size(); ++i) {
			dist_center.push_back(make_pair(dist_map[center_idx][i], i));
		}
		std::sort(dist_center.begin(), dist_center.end(), [](const pair<float, int> & p1, const pair<float, int> & p2) { return p1.first < p2.first; });

		for (int i = 0; i < digit_rects.size(); i++) {
			delete [] dist_map[i];
		}
		delete [] dist_map;

		vector<RotatedRect> digit_rects_temp;
		for (int i = 0; i < 5; ++i) {
			digit_rects_temp.push_back(digit_rects[dist_center[i].second]);
		}
		digit_rects_temp.swap(digit_rects);
	}
	cout << "digit n : " << digit_rects.size() << ' ';
	return sudoku_rects.size() == 9 && (digit_rects.size() == 5 || digit_rects.size() == 4);
}
/*
int RuneDetector::findTargetORB(cv::Mat * cells) {
	Mat descriptor[9];
	vector<vector<KeyPoint> > keypoints;
	keypoints.resize(9);
	ORB orb(100, 1, 1, 10, 0, 2, 1, 17);
	BFMatcher matcher(NORM_HAMMING, 0);
	int match_count[9][9] = { { 0 } };

	for (size_t i = 0; i < 9; i++)	{
		vector<KeyPoint> & kp = keypoints[i];
		Mat & desp = descriptor[i];
		orb(cells[i], Mat(), kp, desp);
		//FAST(cell[idx], kp, 10);
		//Ptr<xfeatures2d::BriefDescriptorExtractor> brief = cv::xfeatures2d::BriefDescriptorExtractor::create();
		//brief->compute(cell[idx], kp, desp);

		if (desp.rows < 2)
			return -1;

		// feature matching
		for (size_t k = 0; k < i; k++) {
			vector<vector<DMatch> > matches;
			matcher.knnMatch(desp, descriptor[k], matches, 2);
			int cnt = 0;
			for (size_t n = 0; n < matches.size(); n++)	{
				vector<DMatch> & m = matches[n];
				DMatch & dm1 = m[0];
				DMatch & dm2 = m[1];
				if (dm1.distance / dm2.distance < 0.8) {
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
	for (size_t i = 0; i < 9; i++) {
		for (size_t j = 0; j < 9; j++) {
			avg_cnt[i] += match_count[i][j];
		}
		if (avg_cnt[i] < min_cnt) {
			min_cnt = avg_cnt[i];
			min_idx = i;
		}
	}
	return min_idx;
}
*/

// int RuneDetector::findTargetCanny(cv::Mat * cells){
//    int min_count = 65535;
//    int min_count_idx = -1;
//    for (size_t i = 0; i < 9; i++)	{
//        int black_count = 0;
//        Mat edge;
//        Canny(cells[i], edge, 20, 50);
//        uchar * ptr = (uchar *)edge.data;

//        for (size_t j = 0; j < cells[i].rows; ++j){
//            for (size_t k = 0; k < cells[i].cols; ++k, ++ptr)	{
//                int v = *ptr;
//                if (v == 255)
//                    ++black_count;
//            }
//        }

//        //cout << black_count << "  ";
//        if(min_count > black_count){
//            min_count = black_count;
//            min_count_idx = i;
//        }
//        //imshow(string("bin")+char(i+'0'), edge);

//    }
//    //cout << "\n";
//    return min_count_idx;
// }

int RuneDetector::findTargetCanny(cv::Mat * cells) {
	int min_count_idx = -1;
	int w_3 = cells[0].cols / 2.8;
	int w_23 = cells[0].cols * 2 / 3.0;
	double mid_ratio = 0.0;

	for (size_t i = 0; i < 9; i++)	{
		int mid_area_count = 0;
		int black_count = 0;
		Mat edge;
		Canny(cells[i], edge, 20, 50);
		uchar * ptr = (uchar *)edge.data;

		for (size_t j = 0; j < cells[i].rows; ++j) {
			for (size_t k = 0; k < cells[i].cols; ++k, ++ptr)	{
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
		if (mid_ratio <  cur_ratio) {
			mid_ratio = cur_ratio;
			min_count_idx = i;
		}

		//rectangle(edge, Rect(w_3,0,w_23-w_3,cells[i].rows), CV_RGB(255,255,255), 1);
		//imshow(string("bin")+char(i+'0'), edge);

	}
	//cout << "\n";
	return min_count_idx;
}

int RuneDetector::findTargetEdge(cv::Mat * cells) {
	int grad_threshold = 10;
	int min_count = 65535;
	int min_count_idx = -1;
	for (size_t i = 0; i < 9; i++)	{
		Mat gradX, gradY;
		cv::Sobel(cells[i], gradX, CV_16S, 1, 0);
		cv::Sobel(cells[i], gradY, CV_16S, 0, 1);

		int large_grad_count = 0;
		short * ptr_x = (short *)gradX.data;
		short * ptr_y = (short *)gradY.data;

		for (size_t j = 0; j < gradX.rows; ++j) {
			for (size_t k = 0; k < gradX.cols; ++k, ++ptr_x, ++ptr_y)	{
				int x = abs(*ptr_x);
				int y = abs(*ptr_y);
				if (x > grad_threshold || y > grad_threshold)
					++large_grad_count;
			}
		}

		if (min_count > large_grad_count) {
			min_count = large_grad_count;
			min_count_idx = i;
		}
		//imshow(string("bin")+char(i+'0'), cells[i]);
	}
	return min_count_idx;
}

pair <int, int> RuneDetector::chooseMnistTarget(const Mat & inputImg, const vector<RotatedRect> & sudoku_rects) {
// get 9(cell) X 4(corner) corner, and 9 cell's center
	Mat image;
	cvtColor(inputImg, image, CV_BGR2GRAY);
	vector<Point2fWithIdx> centers;
	vector<Point2f> corner;
	Point2f center_avg;
	for (size_t i = 0; i < sudoku_rects.size(); i++)	{
		const RotatedRect & rect = sudoku_rects[i];
		Point2f vecs[4];
		rect.points(vecs);
		for (size_t j = 0; j < 4; j++) {
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
	sort(centers.begin(), centers.end(), [](const Point2fWithIdx & p1, const Point2fWithIdx & p2) { return p1.p.y < p2.p.y; });
	sort(centers.begin() + 0, centers.begin() + 3, [](const Point2fWithIdx & p1, const Point2fWithIdx & p2) { return p1.p.x < p2.p.x; });
	sort(centers.begin() + 3, centers.begin() + 6, [](const Point2fWithIdx & p1, const Point2fWithIdx & p2) { return p1.p.x < p2.p.x; });
	sort(centers.begin() + 6, centers.begin() + 9, [](const Point2fWithIdx & p1, const Point2fWithIdx & p2) { return p1.p.x < p2.p.x; });

	// get position of [0,2,6,8] corner
	int corner_idx[] = { 0, 2, 6, 8 };
	vector<Point2f> corner_0268;
	for (size_t i = 0; i < 4; i++) {
		size_t k = centers[corner_idx[i]].idx * 4;
		for (size_t j = 0; j < 4; j++) {
			corner_0268.push_back(corner[k + j]);
		}
	}

	// find approx corner of sudoku
	RotatedRect rect = minAreaRect(corner_0268);
	Point2f vertices[4];
	rect.points(vertices);
	Point2f lu, ld, ru, rd;
	sort(vertices, vertices + 4, [](const Point2f & p1, const Point2f & p2) { return p1.x < p2.x; });
	if (vertices[0].y < vertices[1].y) {
		lu = vertices[0];
		ld = vertices[1];
	}
	else {
		lu = vertices[1];
		ld = vertices[0];
	}
	if (vertices[2].y < vertices[3].y)	{
		ru = vertices[2];
		rd = vertices[3];
	}
	else {
		ru = vertices[3];
		rd = vertices[2];
	}

	// find actual corner of sudoku
	Point2f _lu, _ld, _ru, _rd;
	float mlu = 10000.0, mld = 10000.0, mru = 10000.0, mrd = 10000.0;
	for (size_t i = 0; i < corner_0268.size(); i++) {
		const Point2f & p = corner_0268[i];
		float v1 = (p - lu).dot((p - lu));
		float v2 = (p - ld).dot((p - ld));
		float v3 = (p - ru).dot((p - ru));
		float v4 = (p - rd).dot((p - rd));
		if (v1 < mlu) {
			mlu = v1;
			_lu = p;
		}
		if (v2 < mld) {
			mld = v2;
			_ld = p;
		}
		if (v3 < mru) {
			mru = v3;
			_ru = p;
		}
		if (v4 < mrd) {
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
	imshow("image_persp", image_persp);

	
	dst_p.clear();
	dst_p.push_back(_lu + Point2f(0.0, 0.0));
	dst_p.push_back(_lu + Point2f(0.0, _height));
	dst_p.push_back(_lu + Point2f(_width, 0.0));
	dst_p.push_back(_lu + Point2f(_width, _height));
	perspective_mat = getPerspectiveTransform(src_p, dst_p);
	Point2f perspective_center = MatDotPoint(perspective_mat, center_avg);
	Mat perspective_image;
	warpPerspective(inputImg, perspective_image, perspective_mat, inputImg.size());
	/*
	Rect2f boardRect = Rect2f(perspective_center, Size2f(_width, _height));
	digitRecognizer.predict(perspective_image, boardRect);
	*/
	sort(digit_rects.begin(), digit_rects.end(), [](const RotatedRect& a, const RotatedRect& b) { return a.center.x < b.center.x;});
	vector<Mat> digit_images;
	for (int i = 0; i < digit_rects.size(); i++)
	{
		Point2f pts[4];
		vector<Point2f> vpts, t;
		digit_rects[i].points(pts);
		// for_each(pts, (pts + 4), [&](const Point2f& a){vpts.push_back(MatDotPoint(perspective_image, a));});
		 for_each(pts, (pts + 4), [&](const Point2f& a){vpts.push_back(a);});
		perspectiveTransform(vpts, t, perspective_mat);
		digit_images.push_back(perspective_image(boundingRect(t)));
		imshow("showtime", digit_images[i]);
		cout << digitRecognizer.process(digit_images.at(i)) << endl;
		waitKey(0);
	}


	// calculate the average width and hieght of each cell
	const double * pdata = (double *)perspective_mat.data;
	float height_avg = 0.0, width_avg = 0.0;
	for (size_t i = 0; i < sudoku_rects.size(); ++i) {
		vector<Point2f> vec_p;
		for (size_t j = 0; j < 4; j++) {
			const Point2f & p = corner[i * 4 + j];
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

    if(height_avg > _height / 3)
        height_avg = 0.25 * _height;
    if(width_avg > _width / 3)
        width_avg = 0.25 * _width;

	// get image of every cell, then compute ORB feature and match feature;
	int cell_width = 0.98 * width_avg;
	int cell_height = height_avg;
	int half_w_gap = (width_avg - cell_width) / 2, half_h_gap = (height_avg - cell_height) / 2;
	half_w_gap = 0; half_h_gap = 0;
	int offset_x = 0;
	int offset_y = 0;
	int width_start[] = { half_w_gap, int((_width - cell_width) / 2), int(_width - cell_width - half_w_gap) };
    int height_start[] = { half_h_gap, int((_height - cell_height) / 2), int(_height - cell_height - half_h_gap) };

	vector<vector<pair<double, int> > > results;
	Mat temp;

	for (size_t i = 0; i < 3; i++){
		for (size_t j = 0; j < 3; j++){
			size_t idx = i * 3 + j;
            Rect cell_roi(width_start[j]+offset_x, height_start[i]+offset_y, cell_width, cell_height);
			image_persp(cell_roi).copyTo(temp);
			threshold(temp, temp, 120, 255, THRESH_BINARY);
			resize(temp, temp, Size(28, 28));
			results.push_back(mnistRecognizer.recognize_primary(temp));
		}
	}

	vector<vector<pair<double, int> >::iterator> FinalResults;
	deque<int> check_index;

	for (int i = 0; i < results.size(); i++) {
		FinalResults.push_back(results.at(i).begin());
		check_index.push_back(i);
	}

	for (deque<int>::iterator idxitr1 = check_index.begin(); idxitr1 != check_index.end(); idxitr1++) {
		for (int j = 0; j < results.size(); j++) {
			if (*idxitr1 == j) continue;
			int i = *idxitr1;
			if ((*(FinalResults.at(i))).second == (*(FinalResults.at(j))).second) {
				if ((*(FinalResults.at(i))).first < (*(FinalResults.at(j))).first && (FinalResults.at(i)) + 1 != results.at(i).end()) {
					(FinalResults.at(i))++;
					check_index.insert(idxitr1 + 1, *idxitr1);
					break; //recompare from beginning
				}
				else {
					if (FinalResults.at(j) + 1 == results.at(j).end())
					{
						if ((FinalResults.at(i)) + 1 != results.at(i).end()) {
							(FinalResults.at(i))++;
							check_index.insert(idxitr1 + 1, *idxitr1);
							break; //recompare from beginning
						}
						else {
							//both conflict > 10 times -> should be impossible to solve
							return make_pair(-1, -1);
						}
					}
					(FinalResults.at(j))++;
					check_index.insert(idxitr1 + 1, j);
				}
			}
		}
	}

	cout << "(";
	for (int i = 0; i < FinalResults.size(); i++){
		cout << (*(FinalResults.at(i))).second << ' ';
	}
	cout << ")" << endl;
	
	return make_pair(0,0);
}

pair<int, int> RuneDetector::chooseTargetPerspective(const Mat & image, const vector<RotatedRect> & sudoku_rects) {
	// get 9(cell) X 4(corner) corner, and 9 cell's center
	vector<Point2fWithIdx> centers;
	vector<Point2f> corner;
	for (size_t i = 0; i < sudoku_rects.size(); i++)	{
		const RotatedRect & rect = sudoku_rects[i];
		Point2f vecs[4];
		rect.points(vecs);
		for (size_t j = 0; j < 4; j++) {
			corner.push_back(vecs[j]);
		}
		centers.push_back(Point2fWithIdx(rect.center, i));
	}

	// arange sudoku cell to following order
	// 0  1  2
	// 3  4  5
	// 6  7  8
	sort(centers.begin(), centers.end(), [](const Point2fWithIdx & p1, const Point2fWithIdx & p2) { return p1.p.y < p2.p.y; });
	sort(centers.begin() + 0, centers.begin() + 3, [](const Point2fWithIdx & p1, const Point2fWithIdx & p2) { return p1.p.x < p2.p.x; });
	sort(centers.begin() + 3, centers.begin() + 6, [](const Point2fWithIdx & p1, const Point2fWithIdx & p2) { return p1.p.x < p2.p.x; });
	sort(centers.begin() + 6, centers.begin() + 9, [](const Point2fWithIdx & p1, const Point2fWithIdx & p2) { return p1.p.x < p2.p.x; });

	// get position of [0,2,6,8] corner
	int corner_idx[] = { 0, 2, 6, 8 };
	vector<Point2f> corner_0268;
	for (size_t i = 0; i < 4; i++) {
		size_t k = centers[corner_idx[i]].idx * 4;
		for (size_t j = 0; j < 4; j++) {
			corner_0268.push_back(corner[k + j]);
		}
	}

	// find approx corner of sudoku
	RotatedRect rect = minAreaRect(corner_0268);
	Point2f vertices[4];
	rect.points(vertices);
	Point2f lu, ld, ru, rd;
	sort(vertices, vertices + 4, [](const Point2f & p1, const Point2f & p2) { return p1.x < p2.x; });
	if (vertices[0].y < vertices[1].y) {
		lu = vertices[0];
		ld = vertices[1];
	}
	else {
		lu = vertices[1];
		ld = vertices[0];
	}
	if (vertices[2].y < vertices[3].y)	{
		ru = vertices[2];
		rd = vertices[3];
	}
	else {
		ru = vertices[3];
		rd = vertices[2];
	}

	// find actual corner of sudoku
	Point2f _lu, _ld, _ru, _rd;
	float mlu = 10000.0, mld = 10000.0, mru = 10000.0, mrd = 10000.0;
	for (size_t i = 0; i < corner_0268.size(); i++) {
		const Point2f & p = corner_0268[i];
		float v1 = (p - lu).dot((p - lu));
		float v2 = (p - ld).dot((p - ld));
		float v3 = (p - ru).dot((p - ru));
		float v4 = (p - rd).dot((p - rd));
		if (v1 < mlu) {
			mlu = v1;
			_lu = p;
		}
		if (v2 < mld) {
			mld = v2;
			_ld = p;
		}
		if (v3 < mru) {
			mru = v3;
			_ru = p;
		}
		if (v4 < mrd) {
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
	const double * pdata = (double *)perspective_mat.data;
	float height_avg = 0.0, width_avg = 0.0;
	for (size_t i = 0; i < sudoku_rects.size(); ++i) {
		vector<Point2f> vec_p;
		for (size_t j = 0; j < 4; j++) {
			const Point2f & p = corner[i * 4 + j];
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
	int width_start[] = { half_w_gap, int((_width - cell_width) / 2), int(_width - cell_width - half_w_gap) };
	int height_start[] = { half_h_gap, int((_height - cell_height) / 2), int(_height - cell_height - half_h_gap) };

	Mat cell[9];
	for (size_t i = 0; i < 3; i++) {
		for (size_t j = 0; j < 3; j++) {
			size_t idx = i * 3 + j;
			Rect cell_roi(width_start[j] + offset_x, height_start[i] + offset_y, cell_width, cell_height);
			cout << cell_roi << endl;
			image_persp(cell_roi).copyTo(cell[idx]);
		}
	}

	int idx = -1;
	/*
	if (type == RUNE_ORB)
		idx = findTargetORB(cell);
		*/
	if (type == RUNE_GRAD)
		idx = findTargetEdge(cell);
	else if (type == RUNE_CANNY)
		idx = findTargetCanny(cell);

	//int idxx = findTargetCanny(cell);
	//cout << "Canny: " << idxx << "\tEDGE: " << idx << endl;
	return idx < 0 ? make_pair(-1, -1) : make_pair((int)centers[idx].idx, idx);
}

pair<int, int> RuneDetector::chooseTarget(const Mat & image, const vector<RotatedRect> & sudoku_rects) {
	// get 9(cell) X 4(corner) corner, and 9 cell's center
	vector<Point2fWithIdx> centers;
	vector<Point2f> corner;
	for (size_t i = 0; i < sudoku_rects.size(); i++)	{
		const RotatedRect & rect = sudoku_rects[i];
		Point2f vecs[4];
		rect.points(vecs);
		for (size_t j = 0; j < 4; j++) {
			corner.push_back(vecs[j]);
		}
		centers.push_back(Point2fWithIdx(rect.center, i));
	}

	// arange sudoku cell to following order
	// 0  1  2
	// 3  4  5
	// 6  7  8
	sort(centers.begin(), centers.end(), [](const Point2fWithIdx & p1, const Point2fWithIdx & p2) { return p1.p.y < p2.p.y; });
	sort(centers.begin() + 0, centers.begin() + 3, [](const Point2fWithIdx & p1, const Point2fWithIdx & p2) { return p1.p.x < p2.p.x; });
	sort(centers.begin() + 3, centers.begin() + 6, [](const Point2fWithIdx & p1, const Point2fWithIdx & p2) { return p1.p.x < p2.p.x; });
	sort(centers.begin() + 6, centers.begin() + 9, [](const Point2fWithIdx & p1, const Point2fWithIdx & p2) { return p1.p.x < p2.p.x; });

	Mat cell[9];

	for (size_t i = 0; i < 3; i++) {
		for (size_t j = 0; j < 3; j++) {
			size_t idx = i * 3 + j;
			Rect cell_roi = sudoku_rects[centers[idx].idx].boundingRect();
			int margin_x = 0.25 * cell_roi.width;
			int margin_y = 0.25 * cell_roi.height;
			Rect scale_roi = Rect(cell_roi.x + margin_x, cell_roi.y + margin_y, cell_roi.width - 2 * margin_x, cell_roi.height - 2 * margin_y);
			image(scale_roi).copyTo(cell[idx]);
		}
	}

	int idx = -1;
	/*
	if (type == RUNE_ORB)
	    idx = findTargetORB(cell);
		*/
	if (type == RUNE_GRAD)
		idx = findTargetEdge(cell);
	else if (type == RUNE_CANNY)
		idx = findTargetCanny(cell);

	//int idxx = findTargetCanny(cell);
	//cout << "Canny: " << idxx << "\tEDGE: " << idx << endl;
	return idx < 0 ? make_pair(-1, -1) : make_pair((int)centers[idx].idx, idx);
}

RotatedRect RuneDetector::adjustRotatedRect(const RotatedRect & rect) {
	const Size2f & s = rect.size;
	if (s.width > s.height)
		return rect;
	return RotatedRect(rect.center, Size2f(s.height, s.width), rect.angle + 90.0);
}

