#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <string>
#include <highgui.h>

#include "MnistRecognizer.h"
#include "DigitRecognizer.h"
#include "RuneDetector.hpp"
#include "RMVideoCapture.hpp"

int main(int argc, char** argv )
{
    RMVideoCapture cap("/dev/video0", 3);
    cap.setVideoFormat(1280, 720, 1);
    int exp_t = 64;
    cap.setExposureTime(0, exp_t);//settings->exposure_time);
    cap.startStream();
    cap.info();
	MnistRecognizer mnistRecognizer("LeNet-model");
	DigitRecognizer digitRecognizer;
    while(1){
        Mat original_img;
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
		//resize(original_img, original_img, Size(480, 640));
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
    if (argc != 3)
    {
        cout << "Please specify the image and recognizer module!" << endl;
        return -1;
    }
    original_img = imread(argv[2]);
    if ( !original_img.data )
    {
        printf("No orignal image data \n");
        return -1;
    }
    resize(original_img, original_img, Size(480, 640));
    namedWindow("Original Image", WINDOW_AUTOSIZE );
    imshow("Original Image", original_img);
    float ratio = 0.5;
    RuneDetector runeDetector(127 * ratio, 71 * ratio,  true);
    runeDetector.getTarget(original_img, RuneDetector::RUNE_B);
    for (int i = 0; i < 9; i++){
        imshow(to_string(i), runeDetector.getSudokuImgs(i));
    }
    waitKey(0);
    return 0;
}

