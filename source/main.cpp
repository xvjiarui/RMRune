#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <string>
#include <highgui.h>

#include "MnistRecognizer.h"
#include "DigitRecognizer.h"
#include "RuneDetector.hpp"


int H_L = 0;
int S_L = 0;
int V_L = 0;
int H_H = 255;
int S_H = 255;
int V_H = 255;
// void LowerBound(int, void* )
// {
//     Mat temp;
//     inRange(show, Scalar(H_L, S_L, V_L) , Scalar(H_H, S_H, V_H), temp);  
//     imshow("temp", temp);
//     cout << "H:" << H_L << " " << H_H << endl;
//     cout << "S:" << S_L << " " << S_H << endl;
//     cout << "V:" << V_L << " " << V_H << endl;
// }

int main(int argc, char** argv )
{
    Mat original_img;
    if (argc != 3)
    {
        cout << "Please specify the image and recognizer module!" << endl;
        return -1;
    }
    original_img = imread(argv[2]);
    MnistRecognizer mnistRecognizer("LeNet-model");
    DigitRecognizer digitRecognizer;
    if ( !original_img.data )
    {
        printf("No orignal image data \n");
        return -1;
    }
    resize(original_img, original_img, Size(480, 640));
    namedWindow("Original Image", WINDOW_AUTOSIZE );
    imshow("Original Image", original_img);
    float ratio = 0.5;
    RuneDetector a(127 * ratio, 71 * ratio,  true);
    a.getTarget(original_img, RuneDetector::RUNE_B);
    for (int i = 0; i < 1; i++){
        imshow(to_string(i), a.testCell[i]);
    }
    waitKey(0);
    return 0;
    if (strcmp(argv[1], "1") == 0)
    {
        digitRecognizer.predict(original_img);
        for (int i = 0; i < digitRecognizer.digitLabels.size(); ++i)
        {
            cout << digitRecognizer.digitLabels.at(i) << flush;
        }

        // namedWindow("Lower_H", WINDOW_AUTOSIZE);
        // namedWindow("Lower_S", WINDOW_AUTOSIZE);
        // namedWindow("Lower_V", WINDOW_AUTOSIZE);
        // namedWindow("Higher_H", WINDOW_AUTOSIZE);
        // namedWindow("Higher_S", WINDOW_AUTOSIZE);
        // namedWindow("Higher_V", WINDOW_AUTOSIZE);
        // createTrackbar( "Lower_H", "Lower_H", &H_L, 255, LowerBound); 
        // createTrackbar( "Lower_S", "Lower_S", &S_L, 255, LowerBound); 
        // createTrackbar( "Lower_V", "Lower_V", &V_L, 255, LowerBound); 
        // createTrackbar( "Higher_H", "Higher_H", &H_H, 255, LowerBound); 
        // createTrackbar( "Higher_S", "Higher_S", &S_H, 255, LowerBound); 
        // createTrackbar( "Higher_V", "Higher_V", &V_H, 255, LowerBound); 
        // LowerBound(0, 0);
    }
    else
    {

        mnistRecognizer.predict(original_img);

        for (int i = 0; i < mnistRecognizer.mnistImgs.size(); i++) {
            if ( !mnistRecognizer.mnistImgs.at(i).data) {
                printf("No single image data \n");
                return -1;
            }
            namedWindow(to_string(i),  WINDOW_AUTOSIZE );
            imshow(to_string(i), mnistRecognizer.mnistImgs.at(i));
            cout << mnistRecognizer.getLabel(i) << endl;
        }
    } 

    waitKey(0);

    return 0;
}

