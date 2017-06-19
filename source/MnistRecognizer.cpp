#include <iostream>
#include <string>
#include "MnistRecognizer.h"

// rescale output to 0-100
template <typename Activation>
//static network<sequential> nn;
double rescale(double x)
{
	Activation a(1);
	return 100.0 * (x - a.scale().first) / (a.scale().second - a.scale().first);
}

void convert_image(cv::Mat img,
                   double minv,
                   double maxv,
                   int w,
                   int h,
                   vec_t &data)
{
	if (img.data == nullptr) return; // cannot open, or it's not an image

	cv::Mat_<uint8_t> resized;
	cv::resize(img, resized, cv::Size(w, h));

	// mnist dataset is "white on black", so negate required
	std::transform(resized.begin(), resized.end(), std::back_inserter(data),
	[ = ](uint8_t c) { return (255 - c) * (maxv - minv) / 255.0 + minv; });
}

Rect cropRect(Rect rect,
              int x_offset_tl, int y_offset_tl,
              int x_offset_br, int y_offset_br)
{
	return Rect( Point(rect.tl().x + x_offset_tl, rect.tl().y + y_offset_tl), Point(rect.br().x + x_offset_br, rect.br().y + y_offset_br) );
}

MnistRecognizer::MnistRecognizer(const string& dictionary)
{
	offset_x = 5;
	offset_y = 5;
	thresh = 182;

	rng = RNG(12345);
	Model_Path = "LeNet-model";
	nn.load(Model_Path);
}

void MnistRecognizer::predict(const Mat& img)
{
	img.copyTo(imgCopy);
	preprocess();
}

Point MnistRecognizer::getCenter(int label)
{
	return mnistCenters.at(label);
}

int MnistRecognizer::getLabel(int index)
{
	return mnistLabels.at(index);
}

void MnistRecognizer::clear()
{
	mnistImgs.clear();
	mnistCenters.clear();
	mnistLabels.clear();
}

MnistRecognizer::~MnistRecognizer()
{
}

int MnistRecognizer::recognize(const Mat& img)
{
	vec_t data;
	convert_image(img, -1.0, 1.0, 32, 32, data);

	// recognize
	auto res = nn.predict(data);
	vector<pair<double, int>> scores;

	// sort & print
	for (int i = 0; i < 10; i++)
	{
		scores.emplace_back(rescale<tanh_layer>(res[i]), i);
	}

	sort(scores.begin(), scores.end(), greater<pair<double, int>>());
	cout << endl << endl;
	for (int i = 0; i < scores.size(); i++)
		cout << "Score: " << scores.at(i).first << "     Predicted Label: " << scores.at(i).second << endl;
	return scores.at(0).second;
}

void MnistRecognizer::preprocess()
{
	clear();
	Mat img;
	GaussianBlur(imgCopy, img, Size(9, 9) , 0);
	cvtColor(img, img, CV_BGR2GRAY);
	Canny(img, img, thresh, thresh * 2, 3);

	dilate(img, img, getStructuringElement(MORPH_RECT, Size(8, 8)));

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
		croppedBoundRects.push_back(cropRect(boundRects.at(i), offset_x, offset_y, -offset_x, -offset_y));
	}

	/* Check Rect Height-Width Ratio */
	for (int i = 0; i < boundRects.size(); i++) {
		int height = boundRects.at(i).height;
		int width = boundRects.at(i).width;
		cout << "Rect " << i << "Ratio = " << (float)height / (float)width << endl;
		cout << "Centre " << i << " (" << boundRects.at(i).x << "," << boundRects.at(i).y << ")" << endl;
	}


	/* Convert one whole image to 9 small images */
	GaussianBlur(imgCopy, img, Size(9, 9), 0);
	for (int i = 0; i < croppedBoundRects.size(); i++) {
		Mat curMnistImg;
		img(croppedBoundRects.at(i)).copyTo(curMnistImg);
		resize(curMnistImg, curMnistImg, Size(28, 28));
		cvtColor(curMnistImg, curMnistImg, CV_BGR2GRAY);
		threshold(curMnistImg, curMnistImg, 120, 255, THRESH_TOZERO);
		mnistImgs.push_back(curMnistImg);
		mnistLabels.insert(pair<int, int> (i, recognize(curMnistImg)));
		mnistCenters.insert(pair<int, Point> (mnistLabels.at(i), 
			Point(croppedBoundRects.at(i).x + croppedBoundRects.at(i).width/2, croppedBoundRects.at(i).y + croppedBoundRects.at(i).height)));
		cout << "Size " << i << ": " << curMnistImg.size() << boundRects.at(i).size()  << endl;
	}
}

