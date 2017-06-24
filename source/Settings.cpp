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
    fin["ExposureTime"] >> Settings::camerasetting.ExposureTime;
    fin["CellRatio"] >> Settings::runesetting.CellRatio;
    fin["CellWidth"] >> Settings::runesetting.CellWidth;
    fin["CellHeight"] >> Settings::runesetting.CellHeight;
    fin.release();
}

void Settings::save() {
    cv::FileStorage fout(Settings::filename, cv::FileStorage::WRITE);
    // fout.writeComment(string("Rune-related settings\n"));
    fout << "CellRatio" << Settings::runesetting.CellRatio;
    fout << "CellWidth" << Settings::runesetting.CellWidth;
    fout << "CellHeight" << Settings::runesetting.CellHeight;
    // fout.writeComment(string("Camera-related settings\n"));
    fout << "ExposureTime" << Settings::camerasetting.ExposureTime;
}