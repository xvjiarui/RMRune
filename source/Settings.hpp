#pragma once
#include "opencv2/core/core.hpp"
#include <string>

class Settings {
    public:
        Settings(const std::string& filename);
        ~Settings();
        void save();
        void load();
        void SetFileName(const std::string& fn);

        struct RuneSetting {
            float CellRatio;
            int CellWidth;
            int CellHeight;
        } runesetting;

        struct CameraSetting {
            int ExposureTime;
			cv::Mat CameraMatrix;
			cv::Mat DistortionMatrix;
        } camerasetting;
    private:
        std::string filename;
};
