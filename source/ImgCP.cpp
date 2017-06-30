#include "ImgCP.hpp"
#include "RuneDetector.hpp"
#include "RMVideoCapture.hpp"
#include <highgui.h>
#include <string>

using namespace std;
using namespace cv;
#define VIDEO_MODE

#define BUFFER_SIZE 1
volatile unsigned int pIdx = 0;
volatile unsigned int cIdx = 0;
struct ImageData {
	Mat img;
	unsigned int frame;
};
ImageData data[BUFFER_SIZE];
void ImgCP::ImageProducer()
{
#ifdef VIDEO_MODE
	if (videoPath == NULL)
	{
		cout << "excuse me?" << endl;
		return;
	}
	VideoCapture cap(videoPath);
	if (!cap.isOpened())
	{
		cout << "not open" << endl;
		return;
	}
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
		cap >> data[pIdx % BUFFER_SIZE].img;
#ifdef VIDEO_MODE
		data[pIdx % BUFFER_SIZE].frame++;
#else
		data[pIdx % BUFFER_SIZE].frame = cap.getFrameCount();
#endif
		++pIdx;
	}
}

void ImgCP::ImageConsumer()
{
	int startTime = 0;
	int endTime = 0;
	Settings& settings = *ImgCP::settings;
	RuneDetector runeDetector(settings);
	AngleSolverFactory angleSolverFactory;
	float CellActualWidth, CellActualHeight;
	CellActualWidth = settings.runeSetting.CellWidth * settings.runeSetting.CellRatio;
	CellActualHeight = settings.runeSetting.CellHeight * settings.runeSetting.CellRatio;
	AngleSolver angleSolver(settings.cameraSetting.CameraMatrix, settings.cameraSetting.DistortionMatrix,
				CellActualWidth, CellActualHeight, 0.4);
	angleSolverFactory.setSolver(&angleSolver);
	angleSolverFactory.setTargetSize(CellActualWidth, CellActualHeight, AngleSolverFactory::TARGET_RUNE);
	while(1)
	{
		while (pIdx - cIdx == 0);
		Mat original_img; 
		data[cIdx % BUFFER_SIZE].img.copyTo(original_img);
		startTime = getTickCount();
		unsigned int frameNum = data[cIdx % BUFFER_SIZE].frame;
		++cIdx;
		try {
			int targetIdx = runeDetector.getTarget(original_img, RuneDetector::RUNE_B).second;
			if (targetIdx == -1)
				continue;
			cout << "targetIdx:" << targetIdx << endl;
			RotatedRect targetRect = runeDetector.getRotateRect(targetIdx);
			double angle_x, angle_y;
			angleSolverFactory.getAngle(targetRect, AngleSolverFactory::TARGET_RUNE, angle_x, angle_y, 20, 0);
			cout << "test angle:" << angle_x << ' ' << angle_y << endl;
			endTime = getTickCount();
		    cout << "Frame time: " << (endTime - startTime) * 1000.0 / getTickFrequency() << endl;
		}
		catch (cv::Exception)
		{
			continue;
		}
		catch (exception)
		{
			continue;
		}
		cout << endl;
	}
}
		
