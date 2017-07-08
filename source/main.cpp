#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <string>
#include <highgui.h>
#include <sys/time.h>
#include <thread>

#include "define.hpp"
#include "Settings.hpp"
#include "MnistRecognizer.h"
#include "DigitRecognizer.h"
#include "RuneDetector.hpp"
#include "AngleSolver.hpp"
#include "ImgCP.hpp"
#include "RMVideoCapture.hpp"

// #define RESET_SETTINGS


int main(int argc, char** argv )
{
    Settings settings("./Settings/Settings.xml");

    #ifdef RESET_SETTINGS
    settings.load();
    settings.voteSetting.saveTime = 5;
    settings.save();
    return 0;
    #endif

    settings.load();
    Mat original_img;

	if (argc == 1)
	{
		cout << "c:Camera v:Video i:Image" << endl;
		return -1;
	}

	if (argv[1][0] == 'i')
	{
		if (argc != 3)
		{
			cout << "Please specify the image path!" << endl;
			return -1;
		}
		original_img = imread(argv[2]);
		namedWindow("Original Image", WINDOW_AUTOSIZE );
		imshow("Original Image", original_img);
		RuneDetector runeDetector(settings);
		runeDetector.getTarget(original_img, RuneDetector::RUNE_B);
		waitKey(0);
	}
	else
	{
		ImgCP imgCP(&settings, argv[1][0], argv[2], 0);
		cout << "imgCP" << endl;
		std::thread t1(&ImgCP::ImageProducer, imgCP); // pass by reference
		std::thread t2(&ImgCP::ImageConsumer, imgCP);
		t1.join();
		t2.join();
	}


    return 0;
}

