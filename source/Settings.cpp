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
	fin["MnistThreshold"] >> Settings::runeSetting.MnistThreshold;
	fin["RuneSType"] >> Settings::runeSetting.RuneSType;
    fin["CellRatio"] >> Settings::runeSetting.CellRatio;
    fin["CellWidth"] >> Settings::runeSetting.CellWidth;
    fin["CellHeight"] >> Settings::runeSetting.CellHeight;
    fin["DigitRatio"] >> Settings::runeSetting.DigitRatio;
    fin["DigitWidth"] >> Settings::runeSetting.DigitWidth;
    fin["DigitHeight"] >>  Settings::runeSetting.DigitHeight;
	fin["OneHeight"] >> Settings::runeSetting.OneHeight;
	fin["MinDistance"] >> Settings::runeSetting.MinDistance;
	fin["MaxDistance"] >> Settings::runeSetting.MaxDistance;
	fin["ContourThreshold"]  >> Settings::runeSetting.ContourThreshold;
    fin["ExposureTime"] >> Settings::cameraSetting.ExposureTime;
	fin["CameraMatrix"] >> Settings::cameraSetting.CameraMatrix;
	fin["DistortionMatrix"] >> Settings::cameraSetting.DistortionMatrix;
    fin["HsvLowerBound"] >> Settings::lightSetting.hsvLowerBound;
	fin["HsvUpperBound"] >> Settings::lightSetting.hsvUpperBound;
    fin["VoterSaveTime"] >> Settings::voteSetting.saveTime;
    fin["ScaleX"] >> Settings::gimbalSetting.ScaleX;
    fin["ScaleY"] >> Settings::gimbalSetting.ScaleY;
    fin["ScaleZ"] >> Settings::gimbalSetting.ScaleZ;
    fin["GimbalX"] >> Settings::gimbalSetting.GimbalX;
    fin["GimbalY"] >> Settings::gimbalSetting.GimbalY;
    fin["GimbalZ"] >> Settings::gimbalSetting.GimbalZ;
	fin["CameraTheta"] >> Settings::gimbalSetting.CameraTheta;
	fin["ShootingSpeed"] >> Settings::gimbalSetting.ShootingSpeed;
    fin.release();
}

void Settings::save() {
    cv::FileStorage fout(Settings::filename, cv::FileStorage::WRITE);
    // fout.writeComment(string("Rune-related settings\n"));
	fout << "MnistThreshold" << Settings::runeSetting.MnistThreshold;
	fout << "RuneSType" << Settings::runeSetting.RuneSType;
    fout << "CellRatio" << Settings::runeSetting.CellRatio;
    fout << "CellWidth" << Settings::runeSetting.CellWidth;
    fout << "CellHeight" << Settings::runeSetting.CellHeight;
    fout << "DigitRatio" << Settings::runeSetting.DigitRatio;
    fout << "DigitWidth" << Settings::runeSetting.DigitWidth;
    fout << "DigitHeight" << Settings::runeSetting.DigitHeight;
	fout << "OneHeight" << Settings::runeSetting.OneHeight;
	fout << "MinDistance" << Settings::runeSetting.MinDistance;
	fout << "MaxDistance" << Settings::runeSetting.MaxDistance;
	fout << "ContourThreshold" << Settings::runeSetting.ContourThreshold;
    fout << "VoterSaveTime" << Settings::voteSetting.saveTime;
    // fout.writeComment(string("Camera-related settings\n"));
    fout << "ExposureTime" << Settings::cameraSetting.ExposureTime;
    fout << "HsvLowerBound" << Settings::lightSetting.hsvLowerBound;
    fout << "HsvUpperBound" << Settings::lightSetting.hsvUpperBound;
	fout << "CameraMatrix" << Settings::cameraSetting.CameraMatrix;
	fout << "DistortionMatrix" << Settings::cameraSetting.DistortionMatrix;

    fout << "ScaleX" << Settings::gimbalSetting.ScaleX;
    fout << "ScaleY" << Settings::gimbalSetting.ScaleY;
    fout << "ScaleZ" << Settings::gimbalSetting.ScaleZ;
    fout << "GimbalX" << Settings::gimbalSetting.GimbalX;
    fout << "GimbalY" << Settings::gimbalSetting.GimbalY;
    fout << "GimbalZ" << Settings::gimbalSetting.GimbalZ;
	fout << "CameraTheta" << Settings::gimbalSetting.CameraTheta;
	fout << "ShootingSpeed" << Settings::gimbalSetting.ShootingSpeed;
}
