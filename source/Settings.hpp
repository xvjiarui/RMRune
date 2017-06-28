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
        struct RuneSetting
        {
            float CellRatio;
            int CellWidth;
            int CellHeight;
            int DigitWidth;
            int DigitHeight;
            float DigitRatio;
            short RuneSType; //0 1 2
        } runeSetting;
        struct CameraSetting
        {
            int ExposureTime;
            cv::Mat CameraMatrix;
            cv::Mat DistortionMatrix;
        } cameraSetting;
        struct LightSetting
        {
            cv::Scalar hsvLowerBound;
            cv::Scalar hsvUpperBound;
        } lightSetting;

      private:
        std::string filename;
};
