#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <string>
#include <highgui.h>
#include <sys/time.h>

#include "Settings.hpp"
#include "MnistRecognizer.h"
#include "DigitRecognizer.h"
#include "RuneDetector.hpp"
#include "AngleSolver.hpp"

// // For Debug and Test 
// int H_L = 0;
// int S_L = 0;
// int V_L = 0;
// int H_U = 255;
// int S_U = 255;
// int V_U = 255;

// Mat show;
// void boundFilter(int, void* )
// {
//     Mat temp;
//     inRange(show, Scalar(H_L, S_L, V_L) , Scalar(H_U, S_U, V_U), temp);  
//     imshow("temp", temp);
//     cout << "H:" << H_L << " " << H_U << endl;
//     cout << "S:" << S_L << " " << S_U << endl;
//     cout << "V:" << V_L << " " << V_U << endl;
// }


#define CAMERA_MODE
// #define VIDEO_MODE
// #define IMAGE_MODE

//#define RESET_SETTINGS

#ifdef CAMERA_MODE
#include "RMVideoCapture.hpp"
#endif
int main(int argc, char** argv )
{
    Settings settings("./Settings/Settings.xml");

    #ifdef RESET_SETTINGS
    settings.load();
    settings.runeSetting.DigitHeight = 15;
    settings.runeSetting.DigitWidth = 27;
    settings.runeSetting.DigitRatio = 1;
    settings.runeSetting.RuneSType = 0;
    settings.save();
    return 0;
    #endif

    settings.load();
    Mat original_img;
    RuneDetector runeDetector(settings);

    #ifdef IMAGE_MODE

    if (argc != 2)
    {
        cout << "Please specify the image path!" << endl;
        return -1;
    }
    original_img = imread(argv[1]);
    namedWindow("Original Image", WINDOW_AUTOSIZE );
    imshow("Original Image", original_img);
    runeDetector.getTarget(original_img, RuneDetector::RUNE_B);
    waitKey(0);

    #elif defined(VIDEO_MODE)

    if (argc != 2)
    {
        cout << "Please specify the video path!" << endl;
        return -1;
    }
    VideoCapture cap(argv[1]);
    
    #else
    
    RMVideoCapture cap("/dev/video0", 3);
    cap.setVideoFormat(1280, 720, 1);
    cap.setExposureTime(0, settings.cameraSetting.ExposureTime);//settings->exposure_time);
    cap.startStream();
    cap.info();

    #endif

    #ifndef IMAGE_MODE

    while(1)
    {
		struct timeval tv;
		gettimeofday(&tv, NULL);
		suseconds_t startTime = tv.tv_usec;
        cap >> original_img;
        char key = waitKey(20);
        /*
        if (key == 'w'){
            exp_t += 1;
            cap.setExposureTime(0, exp_t);//settings->exposure_time);
            cout << "current exp t:\t" << exp_t << endl;
        }
        else if(key == 's'){
            exp_t -= 1;
            cap.setExposureTime(0, exp_t);
            cout << "current exp t:\t" << exp_t << endl;
        }
        */
		namedWindow("Original Image", WINDOW_AUTOSIZE );
		imshow("Original Image", original_img);
        try {
            int targetIdx = runeDetector.getTarget(original_img, RuneDetector::RUNE_B).second;
            if (targetIdx == -1)
                continue;
            RotatedRect targetRect = runeDetector.getRotatedRect(targetIdx);
            float CellActualWidth, CellActualHeight;
            CellActualWidth = settings.runeSetting.CellWidth * settings.runeSetting.CellRatio;
            CellActualHeight = settings.runeSetting.CellHeight * settings.runeSetting.CellRatio;
            AngleSolverFactory* angleSolverFactory;
            angleSolverFactory.setSolver(new AngleSolver(settings.cameraSetting.CameraMatrix, settings.cameraSetting.DistortionMatrix,
                                        CellActualWidth, CellActualHeight));
            angleSolverFactory.setTargetSize(CellActualWidth, CellActualHeight, TARGET_RUNE);
            double angle_x, angle_y;
            angleSolverFactory.getAngle(targetRect, angle_x, angle_y, 1);
            cout << 'test angle:' << angle_x << ' ' << angle_y << endl;
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
		gettimeofday(&tv, NULL);
		suseconds_t endTime = tv.tv_usec;
		cout << "Frame time: " << (endTime - startTime) / 1000 << endl;
    }

    #endif

    return 0;
}
