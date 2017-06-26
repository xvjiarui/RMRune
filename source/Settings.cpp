#include "opencv2/core/core.hpp"
#include "Settings.hpp"
#include <string>

using namespace std;

Settings::Settings(const string& fn){
    SetFileName(fn);
}

Settings::~Settings() {

}

void Settings::SetFileName(const string& fn) {
    Settings::filename = fn;
}

void Settings::load() {
    cv::FileStorage fin(Settings::filename, cv::FileStorage::READ);
    fin["CellRatio"] >> Settings::runesetting.CellRatio;
    fin["CellWidth"] >> Settings::runesetting.CellWidth;
    fin["CellHeight"] >> Settings::runesetting.CellHeight;
    fin["DigitRatio"] >> Settings::runesetting.DigitRatio;
    fin["DigitWidth"] >> Settings::runesetting.DigitWidth;
    fin["DigitHeight"] >>  Settings::runesetting.DigitHeight;
    fin["ExposureTime"] >> Settings::camerasetting.ExposureTime;
	fin["CameraMatrix"] >> Settings::camerasetting.CameraMatrix;
	fin["DistortionMatrix"] >> Settings::camerasetting.DistortionMatrix;
    fin.release();
}

void Settings::save() {
    cv::FileStorage fout(Settings::filename, cv::FileStorage::WRITE);
    // fout.writeComment(string("Rune-related settings\n"));
    fout << "CellRatio" << Settings::runesetting.CellRatio;
    fout << "CellWidth" << Settings::runesetting.CellWidth;
    fout << "CellHeight" << Settings::runesetting.CellHeight;
    fout << "DigitRatio" << Settings::runesetting.DigitRatio;
    fout << "DigitWidth" << Settings::runesetting.DigitWidth;
    fout << "DigitHeight" << Settings::runesetting.DigitHeight;
    // fout.writeComment(string("Camera-related settings\n"));
    fout << "ExposureTime" << Settings::camerasetting.ExposureTime;
	fout << "CameraMatrix" << Settings::camerasetting.CameraMatrix;
	fout << "DistortionMatrix" << Settings::camerasetting.DistortionMatrix;
}
