#pragma once
#include "opencv2/opencv.hpp"
#include "define.hpp"
#include "Settings.hpp"
#include "RuneDetector.hpp"
#include "AngleSolver.hpp"
#include "RuneDetector.hpp"

class ImgCP {
	public:
		ImgCP(Settings* _settings, char mode, const char* _videoPath, int fd_car)
		{
			settings = _settings;
			fd2car = fd_car;
			videoPath =_videoPath;
			isVideoMode = mode == 'v';
		}
		void ImageProducer();
		void ImageConsumer();
	private:
		Settings* settings;
		const char* videoPath;
		int fd2car;
		bool isVideoMode;
};
