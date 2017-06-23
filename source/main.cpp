#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <string>
#include <highgui.h>

#include "MnistRecognizer.h"
#include "DigitRecognizer.h"
#include "RuneDetector.hpp"
#define USING_TEST_VIDEO
#ifndef USING_TEST_VIDEO
    #include "RMVideoCapture.hpp"
#endif

// For Debug and Test 
int H_L = 0;
int S_L = 0;
int V_L = 0;
int H_U = 255;
int S_U = 255;
int V_U = 255;

Mat show;
void boundFilter(int, void* )
{
    Mat temp;
    inRange(show, Scalar(H_L, S_L, V_L) , Scalar(H_U, S_U, V_U), temp);  
    imshow("temp", temp);
    cout << "H:" << H_L << " " << H_U << endl;
    cout << "S:" << S_L << " " << S_U << endl;
    cout << "V:" << V_L << " " << V_U << endl;
}

int main(int argc, char** argv )
{
    if (argc != 3)
    {
        cout << "Please specify the image and recognizer module!" << endl;
        return -1;
    }
    #ifndef USING_TEST_VIDEO
        RMVideoCapture cap("/dev/video0", 3);
        cap.setVideoFormat(1280, 720, 1);
        int exp_t = 64;
        cap.setExposureTime(0, exp_t);//settings->exposure_time);
        cap.startStream();
        cap.info();
    #else
        cout << "USING_TEST_VIDEO" <<endl;
        VideoCapture cap(argv[2]);
    #endif
    if (!cap.isOpened())
    {
        cout << "Could not open video!" << endl;
        return -1;
    }
    cout << "Opened" <<endl;
	MnistRecognizer mnistRecognizer("LeNet-model");
	DigitRecognizer digitRecognizer;
    while(1){
        Mat original_img;
        cap >> original_img;
        char key = waitKey(20);
		
        #ifndef USING_TEST_VIDEO
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
        #endif
		namedWindow("Original Image", WINDOW_AUTOSIZE );
		imshow("Original Image", original_img);
		float ratio = 1;
		RuneDetector runeDetector(127 * ratio, 71 * ratio,  true);
		try {
		runeDetector.getTarget(original_img, RuneDetector::RUNE_B);
		}
		catch (cv::Exception)
		{
			continue;
		}
    }

    Mat original_img;
    original_img = imread(argv[2]);
    if ( !original_img.data )
    {
        printf("No orignal image data \n");
        return -1;
    }
    resize(original_img, original_img, Size(480, 640));
    namedWindow("Original Image", WINDOW_AUTOSIZE );
    imshow("Original Image", original_img);
    if (strcmp(argv[1], "DEBUG") == 0)
    {
        show = original_img;
        GaussianBlur(show, show, Size(9, 9) , 0);
        namedWindow("Lower_H", WINDOW_AUTOSIZE);
        namedWindow("Lower_S", WINDOW_AUTOSIZE);
        namedWindow("Lower_V", WINDOW_AUTOSIZE);
        namedWindow("Upper_H", WINDOW_AUTOSIZE);
        namedWindow("Upper_S", WINDOW_AUTOSIZE);
        namedWindow("Upper_V", WINDOW_AUTOSIZE);
        createTrackbar( "Lower_H", "Lower_H", &H_L, 255, boundFilter); 
        createTrackbar( "Lower_S", "Lower_S", &S_L, 255, boundFilter); 
        createTrackbar( "Lower_V", "Lower_V", &V_L, 255, boundFilter); 
        createTrackbar( "Upper_H", "Upper_H", &H_U, 255, boundFilter); 
        createTrackbar( "Upper_S", "Upper_S", &S_U, 255, boundFilter); 
        createTrackbar( "Upper_V", "Upper_V", &V_U, 255, boundFilter); 
        boundFilter(0, 0);
        waitKey(0);
    }
    float ratio = 0.5;
    RuneDetector runeDetector(127 * ratio, 71 * ratio,  true);
    runeDetector.getTarget(original_img, RuneDetector::RUNE_B);
    for (int i = 0; i < 9; i++){
        imshow(to_string(i), runeDetector.getSudokuImgs(i));
    }
    waitKey(0);
    return 0;
}

