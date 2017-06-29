#include "ImgCP.hpp"
#include "RuneDetector.hpp"
#include "RMVideoCapture.hpp"
#include <highgui.h>
#include <string>

using namespace std;
using namespace cv;

void ImgCP::ImageProducer()
{
#ifdef VIDEO_MODE
	if (videoPath == NULL)
	{
		cout << "excuse me?" << endl;
		return;
	}
	VideoCapture cap(videoPath);
#else
	RMVideoCapture cap("/dev/video0", 3);                                                         
	cap.setVideoFormat(1280, 720, 1);
	cap.setExposureTime(0, settings->cameraSetting.ExposureTime);//settings->exposure_time);
	cap.startStream();
	cap.info();
#endif
	while(1)
	{
		while (pIdx - cIdx >= BUFFER_SIZE);
		cap >> data[pIdx % BUFFER_SIZE];
		pIdx++;
	}
}

void ImgCP::ImageConsumer()
{
	while(1)
	{
		while (pIdx - cIdx == 0);
		Mat& original_img = data[cIdx % BUFFER_SIZE];
		RuneDetector& runeDetector = *ImgCP::runeDetector;
		Settings& settings = *ImgCP::settings;
		try {
			int targetIdx = runeDetector.getTarget(original_img, RuneDetector::RUNE_B).second;
			if (targetIdx == -1)
				continue;
			targetIdx = 4;
			cout << "targetIdx:" << targetIdx << endl;
			RotatedRect targetRect = runeDetector.getRotateRect(targetIdx);
			float CellActualWidth, CellActualHeight;
			CellActualWidth = settings.runeSetting.CellWidth * settings.runeSetting.CellRatio;
			CellActualHeight = settings.runeSetting.CellHeight * settings.runeSetting.CellRatio;
			angleSolver = new AngleSolver(settings.cameraSetting.CameraMatrix, settings.cameraSetting.DistortionMatrix,
						CellActualWidth, CellActualHeight, 0.4);
			angleSolverFactory.setSolver(angleSolver);
			angleSolverFactory.setTargetSize(CellActualWidth, CellActualHeight, AngleSolverFactory::TARGET_RUNE);
			double angle_x, angle_y;
			angleSolverFactory.getAngle(targetRect, AngleSolverFactory::TARGET_RUNE, angle_x, angle_y, 20, 0);
			cout << "test angle:" << angle_x << ' ' << angle_y << endl;
			delete angleSolver;
		}
		catch (cv::Exception)
		{
			delete angleSolver;
			continue;
		}
		catch (exception)
		{
			delete angleSolver;
			continue;
		}
		cout << endl;
		cIdx++;
	}
}
		
