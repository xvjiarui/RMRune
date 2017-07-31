#include "ImgCP.hpp"
#include "RuneDetector.hpp"
#include "RMVideoCapture.hpp"
#include "Voter.hpp"
// #include <highgui.h>
#include "manifoldGPIO.h"
#include <string>

using namespace std;
using namespace cv;
using manifoldGPIO::gpioExport;

volatile unsigned int pIdx = 0;
volatile unsigned int cIdx = 0;
struct ImageData {
	Mat img;
	unsigned int frame;
};
ImageData data[BUFFER_SIZE];
unsigned int runeGPIO = manifoldGPIO::low;
manifoldGPIO::GPIO runeToggleButton = manifoldGPIO::gpio157;
void serialSetup();
void serialStart();
void UART0_Close(int fd);
void setGimbalAngle(int index, float pitch, float yaw);
void sendGimbalAngle();
extern int fd;

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

#ifdef ADJUST_COORDINATE
struct CoordinateStruct {
	Mat *t_camera_ptz, *r_camera_ptz;
	AngleSolver* angleSolver;
	AngleSolverFactory* angleSolverFactory;
	double *ptz_camera_x, *ptz_camera_y, *ptz_camera_z;
	double *overlap_dist, *barrel_ptz_offset_y;
	RuneDetector* runeDetector;
	Settings* settings;
};

void CalAngle(void* d)
{
	CoordinateStruct* data = (CoordinateStruct*)d;
	double& ptz_camera_x = *(data->ptz_camera_x);
	double& ptz_camera_y = *(data->ptz_camera_y);
	double& ptz_camera_z = *(data->ptz_camera_z);
	double& overlap_dist = *(data->overlap_dist);
	double& barrel_ptz_offset_y = *(data->barrel_ptz_offset_y);
	Mat& t_camera_ptz = *(data->t_camera_ptz);
	Mat& r_camera_ptz = *(data->r_camera_ptz);
	AngleSolverFactory& angleSolverFactory = *(data->angleSolverFactory);
	AngleSolver& angleSolver = *(data->angleSolver);
	RuneDetector& runeDetector = *(data->runeDetector);
	Settings& settings = *(data->settings);

    //double theta = -atan((ptz_camera_y + barrel_ptz_offset_y)/overlap_dist);
	double theta = -3.1415926 / 180 * settings.gimbalSetting.CameraTheta;
    double r_data[] = {1,0,0,0,cos(theta),-sin(theta), 0, sin(theta), cos(theta)};
    double t_data[] = {ptz_camera_x, ptz_camera_y, ptz_camera_z}; // ptz org position in camera coodinate system
    Mat(3,1, CV_64FC1, t_data).copyTo(t_camera_ptz);
    Mat(3,3, CV_64FC1, r_data).copyTo(r_camera_ptz);
    angleSolver.setRelationPoseCameraPTZ(r_camera_ptz, t_camera_ptz, barrel_ptz_offset_y);
	angleSolverFactory.setSolver(&angleSolver);
	double angle_x, angle_y;
	// cout << ptz_camera_x << ' ' << ptz_camera_y << ' ' << ptz_camera_z;
	for (int i = 0; i < 9; i++)
	{
		if (!(i % 3))
		{
			cout << endl;
		}
		else
		{
			cout << '\t';
		}
		cout << runeDetector.getRotateRect(i).center;
	}
	for (int i = 0; i < 9; i++)
	{
		if (!(i % 3))
		{
			cout << endl;
		}
		else
		{
			cout << '\t';
		}
		if (angleSolverFactory.getAngle(runeDetector.getRotateRect(i), AngleSolverFactory::TARGET_RUNE, angle_x, angle_y, settings.gimbalSetting.ShootingSpeed, 0))
		{
			cout << "(" << angle_x << ", " << angle_y << ')';
		}
	}
	cout << "\n-------------------------------" << endl;
}

void AdjustX(int t, void* d)
{
	*(((CoordinateStruct*)d)->ptz_camera_x) = t - 150;
	CalAngle(d); 	
}
void AdjustY(int t, void* d)
{
	*(((CoordinateStruct*)d)->ptz_camera_y) = t - 150;
	CalAngle(d);
}
void AdjustZ(int t, void* d)
{
	*(((CoordinateStruct*)d)->ptz_camera_z) = t - 150;
	CalAngle(d);
}
#endif

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
				CellActualWidth, CellActualHeight, settings.gimbalSetting.ScaleZ, settings.gimbalSetting.ScaleY, settings.runeSetting.MinDistance, settings.runeSetting.MaxDistance);
    // parameter of PTZ and barrel
    double overlap_dist = 100000.0;
    double barrel_ptz_offset_y = 0;
    double ptz_camera_x = settings.gimbalSetting.GimbalX;
    double ptz_camera_y = settings.gimbalSetting.GimbalY;
    double ptz_camera_z = settings.gimbalSetting.GimbalZ;
	double theta = -3.1415926 / 180 * settings.gimbalSetting.CameraTheta;
    double r_data[] = {1,0,0,0,cos(theta),-sin(theta), 0, sin(theta), cos(theta)};
    double t_data[] = {ptz_camera_x, ptz_camera_y, ptz_camera_z}; // ptz org position in camera coodinate system
    Mat t_camera_ptz(3,1, CV_64FC1, t_data);
    Mat r_camera_ptz(3,3, CV_64FC1, r_data);
    angleSolver.setRelationPoseCameraPTZ(r_camera_ptz, t_camera_ptz, barrel_ptz_offset_y);
	angleSolverFactory.setSolver(&angleSolver);
	angleSolverFactory.setTargetSize(CellActualWidth, CellActualHeight, AngleSolverFactory::TARGET_RUNE);
	bool countTime = false;
#ifndef NO_COMMUNICATION
	//GPIO init
	//manifoldGPIO::gpioExport(runeToggleButton);
	//gpioExport(runeToggleButton);
	//manifoldGPIO::gpioSetDirection(runeToggleButton, manifoldGPIO::input);
	manifoldGPIO::gpioGetValue(runeToggleButton, &runeGPIO);
	//UART init
	serialSetup();
	serialStart();
	setGimbalAngle(0, 0, 0);
	cout << "sending... " << endl;
	sendGimbalAngle();
#endif

#ifdef ADJUST_COORDINATE
	CoordinateStruct pdata;
	pdata.t_camera_ptz = &t_camera_ptz;
	pdata.r_camera_ptz = &r_camera_ptz;
	pdata.angleSolver = &angleSolver;
	pdata.angleSolverFactory = &angleSolverFactory;
	pdata.ptz_camera_x = &ptz_camera_x;
	pdata.ptz_camera_y = &ptz_camera_y;
	pdata.ptz_camera_z = &ptz_camera_z;
	pdata.overlap_dist = &overlap_dist;
	pdata.barrel_ptz_offset_y = &barrel_ptz_offset_y;
	pdata.runeDetector = &runeDetector;
	pdata.settings = &settings;
#endif
	//double hardCodeAngleY[] = {10.0, 6.0, 1.6};

	while(1)
	{
		while (pIdx - cIdx == 0);
		Mat original_img; 
		data[cIdx % BUFFER_SIZE].img.copyTo(original_img);
		unsigned int frameNum = data[cIdx % BUFFER_SIZE].frame;
		++cIdx;
#ifdef ADJUST_COORDINATE
		RuneDetector::RuneType runeType;
		switch(settings.runeSetting.forceRuneType)
		{
			case 1:
				runeType = RuneDetector::RUNE_S;
				break;
			case 2:
				runeType = RuneDetector::RUNE_B;
				break;
			case 0:
			default:
				runeType = (runeGPIO == manifoldGPIO::low)? RuneDetector::RUNE_S : RuneDetector::RUNE_B;
		}
		int targetIdx = runeDetector.getTarget(original_img, runeType).second;
		if (targetIdx == -1)
		{
			cout << "Cannot detect 9 sudokus." << endl;
			continue;
		}
		cout << "--------------------------------\n" << targetIdx << "----------------------------------" << endl;
		int x = ptz_camera_x + 150;
		int y = ptz_camera_y + 150;
		int z = ptz_camera_z + 150;
		namedWindow("AdjustX", WINDOW_NORMAL);
		createTrackbar("Adjust X", "AdjustX", &x, 300, AdjustX, (void *)&pdata);
		imshow("AdjustX", original_img);
		namedWindow("AdjustY", WINDOW_NORMAL);
		createTrackbar("Adjust Y", "AdjustY", &y, 300, AdjustY, (void *)&pdata);
		imshow("AdjustY", original_img);
		namedWindow("AdjustZ", WINDOW_NORMAL);
		createTrackbar("Adjust Z", "AdjustZ", &z, 300, AdjustZ, (void *)&pdata);
		imshow("AdjustZ", original_img);
		while(waitKey(0) != 'n');
		continue;
#endif

		try {
#ifndef NO_COMMUNICATION
			manifoldGPIO::gpioGetValue(runeToggleButton, &runeGPIO);
#endif
			RuneDetector::RuneType runeType;
			switch (settings.runeSetting.forceRuneType)
			{
				case 1:
					runeType = RuneDetector::RUNE_S;
					break;
				case 2:
					runeType = RuneDetector::RUNE_B;
					break;
				case 0:
				default:
					runeType = (runeGPIO == manifoldGPIO::low) ? RuneDetector::RUNE_S : RuneDetector::RUNE_B;
			}
			//startTime = getTickCount();
			int targetIdx = runeDetector.getTarget(original_img, runeType).second;
			//endTime = getTickCount();
		    //cout << "Frame time: " << (endTime - startTime) * 1000.0 / getTickFrequency() << endl;
			if (targetIdx == -1)
			{
				countTime = true;
				continue;
			}
			for (int s = 0; s < 18; s++)
				cout << "target: " << targetIdx << endl;
			RotatedRect targetRect = runeDetector.getRotateRect(targetIdx);
			double angle_x, angle_y;
			if(angleSolverFactory.getAngle(targetRect, AngleSolverFactory::TARGET_RUNE, angle_x, angle_y, settings.gimbalSetting.ShootingSpeed, 0))
			{
				//angle_y += hardCodeAngleY[targetIdx / 3];
				//angle_y /= 2.0;
				cout << targetIdx << " " << targetRect.center << endl;
				cout << "Yaw: " << angle_x << "Pitch: " << angle_y << endl;
			}
#ifndef NO_COMMUNICATION
			setGimbalAngle(targetIdx, angle_x, angle_y);
			cout << "sending... " << endl;
			sendGimbalAngle();
#endif
			cout << "current angle:" << angle_x << ' ' << angle_y << ' ' << targetIdx << endl;
			//cin.get();
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
	}
	// close GPIO and UART
	//manifoldGPIO::gpioUnexport(runeToggleButton);
	UART0_Close(fd);
}
		
