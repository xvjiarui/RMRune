#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <string>
// #include <highgui.h>
// #include <opencv2/highgui.h>
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
#include <opencv2/opencv.hpp>
#include <opencv2/gpu/gpu.hpp>

// #define RESET_SETTINGS

int main(int argc, char** argv )
{
	string path = "./Settings/Settings.xml";
	string camPath = "./Settings/Camera/";
    Mat original_img;
	if (gpu::getCudaEnabledDeviceCount())
		gpu::setDevice(0);
	else
		cout << "gg" << endl;
	//cv::gpu::GpuMat dst;

	if (argc == 1)
	{
		cout << "c:Camera v:Video i:Image" << endl;
		return -1;
	}

	if (argv[1][0] == 'i')
	{
		if (argc < 3)
		{
			cerr << "Please specify the image path!" << endl;
			return -1;
		}
		camPath += (argc == 4 ? argv[3] : "1");
		camPath += ".xml";
		Settings settings(path, camPath);
		settings.load();
		original_img = imread(argv[2]);
		resize(original_img, original_img, Size(640, 480), 0, 0, INTER_CUBIC);
		namedWindow("Original Image", WINDOW_AUTOSIZE );
		imshow("Original Image", original_img);
		RuneDetector runeDetector(settings);
		runeDetector.getTarget(original_img, RuneDetector::RUNE_B);
		waitKey(0);
	}
	else
	{
		if (argv[1][0] == 'v')
		{
			camPath += (argc == 4 ? argv[3] : "1");
		}
		else
		{
			camPath += (argc == 3 ? argv[2] : "1");
		}
		camPath += ".xml";
		Settings settings(path, camPath);
		settings.load();
		ImgCP imgCP(&settings, argv[1][0], argv[2], 0);
		cout << "imgCP" << endl;
		std::thread t1(&ImgCP::ImageProducer, imgCP); // pass by reference
		std::thread t2(&ImgCP::ImageConsumer, imgCP);
		t1.join();
		t2.join();
	}


    return 0;
}

