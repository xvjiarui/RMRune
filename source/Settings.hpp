#pragma once
#include "opencv2/core/core.hpp"
#include <string>
#include "define.hpp"

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
			int OneHeight;
            float DigitRatio;
            short RuneSType; //0 1 2
			int MnistThreshold;
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

        struct VoteSetting
        {
            int saveTime;
        } voteSetting;

      private:
        std::string filename;
};
