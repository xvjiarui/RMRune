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
	rng = RNG(12345);
	Model_Path = dictionary;
	nn.load(Model_Path);
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
	return recognize_primary(img).at(0).second;
}
vector<pair<double, int> > MnistRecognizer::recognize_primary(const Mat& img)
{
	vec_t data;
	/*
	Mat kmeanImg, img;
	kmeanPreprocess(inputImg).copyTo(kmeanImg);
	fitMnist(kmeanImg, img);
	*/
	convert_image(img, -1.0, 1.0, 32, 32, data);

	// recognize
	auto res = nn.predict(data);
	vector<pair<double, int>> scores;

	// sort & print
	for (int i = 1; i < 10; i++)
	{
		scores.emplace_back(rescale<tanh_layer>(res[i]), i);
	}

	// sort(scores.begin(), scores.end(), [](const pair<double, int>& a,const pair<double, int>& b){return a.first > b.first;});
	sort(scores.begin(), scores.end(), greater<pair<double, int> >());
	return scores;
}

Mat MnistRecognizer::kmeanPreprocess(const Mat& img)
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
	kmeans(pixels, 2, labels, TermCriteria(CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 5, 0), 5, KMEANS_PP_CENTERS);
	Mat redImg;
	img.copyTo(redImg);
	int redClass = labels.at<int>(rows * cols);
	pdata = redImg.ptr<float>(0);
	for (int i = 0; i < rows; i++)
	{
		for (int j = 0; j < cols; j++)
		{
			if (labels.at<int>(i * cols + j) == redClass)
			{
				redImg.at<Vec3b>(i, j) = Vec3b(255, 255, 255);
			}
			else redImg.at<Vec3b>(i, j) = Vec3b(0, 0, 0);
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

bool MnistRecognizer::fitMnist(const Mat& inputImg, Mat& resImg)
{
	Mat inputCopy;
	inputImg.copyTo(inputCopy);
	// dilate(inputCopy, inputCopy, getStructuringElement(MORPH_RECT,Size(3,3)), Point(-1, -1), 3);
	//morphologyEx(inputCopy, inputCopy, MORPH_CLOSE, getStructuringElement(MORPH_RECT,Size(3,3)));
	//adaptiveThreshold(redImg, redImg, 255, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY, 3, 0);
	vector<vector<Point> > contours;
	vector<Vec4i> hierarchy;
	findContours( inputCopy, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE, Point(0, 0) );
	if (!contours.size()) return false;
	sort(contours.begin(), contours.end(), [](const vector<Point> & a, const vector<Point> & b) {return a.size() > b.size();});
	vector<Point> curContoursPoly;
	approxPolyDP(contours.at(0), curContoursPoly, 1, true);
	Mat white;
	Mat ones = Mat::ones(inputImg.rows, inputImg.cols, CV_8UC1) * 255;
	ones.copyTo(white);
	Rect curBoundingRect = boundingRect(curContoursPoly);
	Mat mnistCore;
	inputImg(curBoundingRect).copyTo(mnistCore);
	mnistCore.copyTo(white(curBoundingRect));
	white.copyTo(resImg);
	return true;
}
