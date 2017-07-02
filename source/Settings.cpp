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
    fin["CellRatio"] >> Settings::runeSetting.CellRatio;
    fin["CellWidth"] >> Settings::runeSetting.CellWidth;
    fin["CellHeight"] >> Settings::runeSetting.CellHeight;
    fin["DigitRatio"] >> Settings::runeSetting.DigitRatio;
    fin["DigitWidth"] >> Settings::runeSetting.DigitWidth;
    fin["DigitHeight"] >>  Settings::runeSetting.DigitHeight;
	fin["OneHeight"] >> Settings::runeSetting.OneHeight;
    fin["ExposureTime"] >> Settings::cameraSetting.ExposureTime;
	fin["CameraMatrix"] >> Settings::cameraSetting.CameraMatrix;
	fin["DistortionMatrix"] >> Settings::cameraSetting.DistortionMatrix;
    fin["HsvLowerBound"] >> Settings::lightSetting.hsvLowerBound;
    fin["HsvUpperBound"] >> Settings::lightSetting.hsvUpperBound;
    fin["VoterSaveTime"] >> Settings::voteSetting.saveTime;
    fin.release();
}

void Settings::save() {
    cv::FileStorage fout(Settings::filename, cv::FileStorage::WRITE);
    // fout.writeComment(string("Rune-related settings\n"));
    fout << "CellRatio" << Settings::runeSetting.CellRatio;
    fout << "CellWidth" << Settings::runeSetting.CellWidth;
    fout << "CellHeight" << Settings::runeSetting.CellHeight;
    fout << "DigitRatio" << Settings::runeSetting.DigitRatio;
    fout << "DigitWidth" << Settings::runeSetting.DigitWidth;
    fout << "DigitHeight" << Settings::runeSetting.DigitHeight;
	fout << "OneHeight" << Settings::runeSetting.OneHeight;
    fout << "VoterSaveTime" << Settings::voteSetting.saveTime;
    // fout.writeComment(string("Camera-related settings\n"));
    fout << "ExposureTime" << Settings::cameraSetting.ExposureTime;
    fout << "HsvLowerBound" << Settings::lightSetting.hsvLowerBound;
    fout << "HsvUpperBound" << Settings::lightSetting.hsvUpperBound;
	fout << "CameraMatrix" << Settings::cameraSetting.CameraMatrix;
	fout << "DistortionMatrix" << Settings::cameraSetting.DistortionMatrix;
}
