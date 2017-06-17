#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <string>

#include "MnistRecognizer.h"

int main(int argc, char** argv )
{
    Mat original_img;
    if (argc != 2)
    {
    	cout << "Please specify the image!" << endl;
    	return -1;
    }
    original_img = imread(argv[1]);
    MnistRecognizer mnistRecognizer("LeNet-model");
    if ( !original_img.data )
    {
        printf("No orignal image data \n");
        return -1;
    }
    resize(original_img, original_img, Size(480, 640));
    mnistRecognizer.predict(original_img);
    namedWindow("Original Image", WINDOW_AUTOSIZE );
    imshow("Original Image", original_img);

	for(int i = 0; i < mnistRecognizer.mnistImgs.size(); i++) {
		if ( !mnistRecognizer.mnistImgs.at(i).data) {
			printf("No single image data \n");
			return -1;
		}
		namedWindow(to_string(i),  WINDOW_AUTOSIZE );
		imshow(to_string(i), mnistRecognizer.mnistImgs.at(i));
		cout << mnistRecognizer.mnistLabels.at(i) << endl;
	}

    waitKey(0);

    return 0;
}

