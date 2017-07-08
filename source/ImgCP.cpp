#include "ImgCP.hpp"
#include "RuneDetector.hpp"
#include "RMVideoCapture.hpp"
#include "Voter.hpp"
#include <highgui.h>
#include <string>

using namespace std;
using namespace cv;

volatile unsigned int pIdx = 0;
volatile unsigned int cIdx = 0;
struct ImageData {
	Mat img;
	unsigned int frame;
};
ImageData data[BUFFER_SIZE];
void ImgCP::ImageProducer()
{
	if (isVideoMode)
	{
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
		cap.set(CV_CAP_PROP_FRAME_WIDTH,640);
		cap.set(CV_CAP_PROP_FRAME_HEIGHT,480);
		while(1)
		{
			while (pIdx - cIdx >= BUFFER_SIZE);
			Mat temp;
			cap >> temp; 
			resize(temp, temp, Size(640, 480), 0, 0, INTER_CUBIC);
			temp.copyTo(data[pIdx % BUFFER_SIZE].img);
			data[pIdx % BUFFER_SIZE].frame++;
			++pIdx;
		}
	}
	else
	{
		RMVideoCapture cap("/dev/video0", 3); 
		cap.setVideoFormat(640, 480, 1);
		cap.setExposureTime(0, settings->cameraSetting.ExposureTime);//settings->exposure_time);
		cap.startStream();
		cap.info();
		while(1)
		{
			while (pIdx - cIdx >= BUFFER_SIZE);
			cap >> data[pIdx % BUFFER_SIZE].img;
			data[pIdx % BUFFER_SIZE].frame = cap.getFrameCount();
			++pIdx;
		}
	}
}

void ImgCP::ImageConsumer()
{
	int startTime = 0;
	int endTime = 0;
	Settings& settings = *ImgCP::settings;
	RuneDetector runeDetector(settings);
	AngleSolverFactory angleSolverFactory;
	//float CellActualWidth = settings.runeSetting.CellWidth * settings.runeSetting.CellRatio;
	//float CellActualHeight = settings.runeSetting.CellHeight * settings.runeSetting.CellRatio;
	float CellActualWidth = 28;
	float CellActualHeight = 16;
	AngleSolver angleSolver(settings.cameraSetting.CameraMatrix, settings.cameraSetting.DistortionMatrix,
				CellActualWidth, CellActualHeight, settings.gimbalSetting.ScaleZ, settings.runeSetting.MinDistance, settings.runeSetting.MaxDistance);
    // parameter of PTZ and barrel
    const double overlap_dist = 100000.0;
    const double barrel_ptz_offset_y = 0;
    const double ptz_camera_x = settings.gimbalSetting.GimbalX;
    const double ptz_camera_y = settings.gimbalSetting.GimbalY;
    const double ptz_camera_z = settings.gimbalSetting.GimbalZ;
    double theta = -atan((ptz_camera_y + barrel_ptz_offset_y)/overlap_dist);
    double r_data[] = {1,0,0,0,cos(theta),-sin(theta), 0, sin(theta), cos(theta)};
    double t_data[] = {ptz_camera_x, ptz_camera_y, ptz_camera_z}; // ptz org position in camera coodinate system
    Mat t_camera_ptz(3,1, CV_64FC1, t_data);
    Mat r_camera_ptz(3,3, CV_64FC1, r_data);
    angleSolver.setRelationPoseCameraPTZ(r_camera_ptz, t_camera_ptz, barrel_ptz_offset_y);
	angleSolverFactory.setSolver(&angleSolver);
	angleSolverFactory.setTargetSize(CellActualWidth, CellActualHeight, AngleSolverFactory::TARGET_RUNE);
	bool countTime = false;
	while(1)
	{
		while (pIdx - cIdx == 0);
		Mat original_img; 
		data[cIdx % BUFFER_SIZE].img.copyTo(original_img);
		if (!countTime)
			startTime = getTickCount();
		unsigned int frameNum = data[cIdx % BUFFER_SIZE].frame;
		++cIdx;
		try {
			int targetIdx = runeDetector.getTarget(original_img, RuneDetector::RUNE_B).second;
			if (targetIdx == -1)
			{
				countTime = true;
				continue;
			}
			for(int i = 0; i < 9; i++)
			{
				RotatedRect targetRect = runeDetector.getRotateRect(i);
				double angle_x, angle_y;
				if(angleSolverFactory.getAngle(targetRect, AngleSolverFactory::TARGET_RUNE, angle_x, angle_y, 20, 0))
				{
					cout << i << " " << targetRect.center << endl;
					cout << "Yaw: " << angle_x << "Pitch: " << angle_y << endl;
				}
			}
			endTime = getTickCount();
		    cout << "Frame time: " << (endTime - startTime) * 1000.0 / getTickFrequency() << endl;
			countTime = false;
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
		
