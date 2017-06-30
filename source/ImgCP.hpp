#pragma once
#include "opencv2/opencv.hpp"
#include "Settings.hpp"
#include "RuneDetector.hpp"
#include "AngleSolver.hpp"

class ImgCP {
	public:
		ImgCP(Settings* _settings, const char* _videoPath, int fd_car, RuneDetector* _runeDetector)
		{
			settings = _settings;
			videoPath = _videoPath;
			fd2car = fd_car;
			runeDetector = _runeDetector;
			CellActualWidth = settings->runeSetting.CellWidth * settings->runeSetting.CellRatio;
			CellActualHeight = settings->runeSetting.CellHeight * settings->runeSetting.CellRatio;
			angleSolver = new AngleSolver(settings->cameraSetting.CameraMatrix, settings->cameraSetting.DistortionMatrix,
						CellActualWidth, CellActualHeight, 0.4);
			angleSolverFactory.setSolver(angleSolver);
			angleSolverFactory.setTargetSize(CellActualWidth, CellActualHeight, AngleSolverFactory::TARGET_RUNE);
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
		float CellActualWidth, CellActualHeight;
};
