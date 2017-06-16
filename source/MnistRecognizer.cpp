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

MnistRecognizer::~MnistRecognizer()
{
	if (mnistImgs)
	{
		delete mnistImgs;
	}
	if (mnistCenters)
	{
		delete mnistCenters;
	}
	if (mnistLabels)
	{
		delete mnistLabels;
	}
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
	for (int i = 0; i < 10; i++)
		cout << "Score: " << scores[i].first << "     Predicted Label: " << scores[i].second << endl;
	return scores[0].second;
}

void MnistRecognizer::preprocess()
{
	Mat img = imgCopy;
	GaussianBlur(img, img, Size(9, 9) , 0);
	cvtColor(img, img, CV_BGR2GRAY);
	Canny(img, img, thresh, thresh * 2, 3);

	Mat structuringElement = getStructuringElement(MORPH_RECT, Size(8, 8));
	dilate(img, img, structuringElement);

	vector<vector<Point> > contours;
	vector<Vec4i> hierarchy;
	findContours( img, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE, Point(0, 0) );

	vector<vector<Point> > contours_polys( contours.size() );
	vector<Rect> boundRects( contours.size() );
	vector<Rect> croppedBoundRects( contours.size());

	mnistCenters = new vector<Point>(croppedBoundRects.size());
	for ( int i = 0; i < contours.size(); i++ ) {
		approxPolyDP( contours[i], contours_polys[i], 3, true );
		boundRects[i] = boundingRect( Mat(contours_polys[i]) );
		croppedBoundRects[i] = cropRect(boundRects[i], offset_x, offset_y, -offset_x, -offset_y);
		mnistCenters->at(i) = Point(croppedBoundRects[i].x, croppedBoundRects[i].y);
	}

	/* Check Rect Height-Width Ratio */
	for (int i = 0; i < boundRects.size(); i++) {
		int height = boundRects[i].height;
		int width = boundRects[i].width;
		cout << "Rect " << i << "Ratio = " << (float)height / (float)width << endl;
		cout << "Centre " << i << " (" << boundRects[i].x << "," << boundRects[i].y << ")" << endl;
	}

	for (int i = 0; i < contours.size(); i++) {
		rectangle( img, croppedBoundRects[i], Scalar(255, 255, 255), -1, 8, 0 );
	}

	cvtColor(img, img, CV_GRAY2BGR);
	bitwise_and(imgCopy, img, img);

	/* Convert one whole image to 9 small images */
	mnistImgs = new vector<Mat>(croppedBoundRects.size());
	for (int i = 0; i < mnistImgs->size(); i++) {
		img(croppedBoundRects[i]).copyTo(mnistImgs->at(i));
		resize(mnistImgs->at(i), mnistImgs->at(i), Size(28, 28));
		cout << "Size " << i << ": " << mnistImgs->at(i).size() << boundRects[i].size()  << endl;
		cvtColor(mnistImgs->at(i), mnistImgs->at(i), CV_BGR2GRAY);
		threshold(mnistImgs->at(i), mnistImgs->at(i), 120, 255, 3);
	}
}

void MnistRecognizer::predict(const Mat& img)
{
	imgCopy = img;
	preprocess();
	mnistLabels = new vector<int>(mnistImgs->size());
	for (int i = 0; i < mnistImgs->size(); ++i)
	{
		mnistLabels->at(i) = recognize(mnistImgs->at(i));
	}
}

