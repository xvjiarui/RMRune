#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <string>
#include <highgui.h>
#include <sys/time.h>
#include <thread>

#include "Settings.hpp"
#include "MnistRecognizer.h"
#include "DigitRecognizer.h"
#include "RuneDetector.hpp"
#include "AngleSolver.hpp"
#include "ImgCP.hpp"

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


//#define CAMERA_MODE
 #define VIDEO_MODE
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

    #ifdef IMAGE_MODE

    if (argc != 2)
    {
        cout << "Please specify the image path!" << endl;
        return -1;
    }
    original_img = imread(argv[1]);
    namedWindow("Original Image", WINDOW_AUTOSIZE );
    imshow("Original Image", original_img);
    RuneDetector runeDetector(settings);
    runeDetector.getTarget(original_img, RuneDetector::RUNE_B);
    waitKey(0);

    #else

    ImgCP imgCP(&settings, argv[1], 0);
    cout << "imgCP" << endl;
    std::thread t1(&ImgCP::ImageProducer, imgCP); // pass by reference
    std::thread t2(&ImgCP::ImageConsumer, imgCP);

    t1.join();
    t2.join();

    #endif


    return 0;
}

