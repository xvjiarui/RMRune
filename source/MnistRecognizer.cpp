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
	convert_image(img, -1.0, 1.0, 32, 32, data);

	// recognize
	auto res = nn.predict(data);
	vector<pair<double, int>> scores;

	// sort & print
	for (int i = 0; i < 10; i++)
	{
		scores.emplace_back(rescale<tanh_layer>(res[i]), i);
	}

	// sort(scores.begin(), scores.end(), [](const pair<double, int>& a,const pair<double, int>& b){return a.first > b.first;});
	sort(scores.begin(), scores.end(), greater<pair<double, int> >());
	return scores;
}

