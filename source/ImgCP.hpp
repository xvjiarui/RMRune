#pragma once
#include "opencv2/opencv.hpp"
#include "Settings.hpp"
#include "AngleSolver.hpp"
#include "RuneDetector.hpp"

#define BUFFER_SIZE 1

class ImgCP {
	public:
		ImgCP(Settings* _settings, const char* _videopath, int fd_car, RuneDetector* _runeDetector):videoPath(_videopath)
		{
			settings = _settings;
			fd2car = fd_car;
			runeDetector = _runeDetector;
			angleSolver = NULL;
			pIdx = cIdx = 0;
		}
		void ImageProducer();
		void ImageConsumer();
	private:
		Settings* settings;
		const char* videoPath;
		int fd2car;
		RuneDetector* runeDetector;
		AngleSolverFactory angleSolverFactory;
		AngleSolver* angleSolver;
		cv::Mat data[BUFFER_SIZE];
		unsigned int pIdx;
		unsigned int cIdx;
};
