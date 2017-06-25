#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <string>
#include <highgui.h>

#include "MnistRecognizer.h"
#include "DigitRecognizer.h"
#include "RuneDetector.hpp"

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

#include "Settings.hpp"

//#define CAMERA_MODE
// #define VIDEO_MODE
 #define IMAGE_MODE

//#define RESET_SETTINGS
#ifdef CAMERA_MODE
    #include "RMVideoCapture.hpp"
#endif 

int main(int argc, char** argv )
{
    Settings settings("./Settings/Settings.xml");

    #ifdef RESET_SETTINGS
    settings.runesetting.CellWidth = 127;
    settings.runesetting.CellHeight = 71;
    settings.runesetting.CellRatio = 1;
    settings.camerasetting.ExposureTime = 64;
    settings.save();
    return 0;
    #endif

    settings.load();
    MnistRecognizer mnistRecognizer("LeNet-model");
	DigitRecognizer digitRecognizer;
    Mat original_img;
    RuneDetector runeDetector(settings.runesetting.CellWidth * settings.runesetting.CellRatio, 
                              settings.runesetting.CellHeight * settings.runesetting.CellRatio,  true);

    #ifdef IMAGE_MODE

    if (argc != 2)
    {
        cout << "Please specify the image path!" << endl;
        return -1;
    }
    original_img = imread(argv[1]);
    resize(original_img, original_img, Size(480, 640));
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
    cap.setExposureTime(0, settings.camerasetting.ExposureTime);//settings->exposure_time);
    cap.startStream();
    cap.info();

    #endif

    #ifndef IMAGE_MODE

    while(1)
    {
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
            runeDetector.getTarget(original_img, RuneDetector::RUNE_B);
        }
        catch (cv::Exception)
        {
            continue;
        }
        cout << endl;
    }

    #endif

    return 0;
}
