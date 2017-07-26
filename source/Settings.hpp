#pragma once
#include "opencv2/core/core.hpp"
#include <string>
#include "define.hpp"

class Settings {
    public:
        Settings(const std::string& filename, const std::string& camFilename);
        ~Settings();
        void save();
        void load();
        void SetFileName(const std::string& fn, const std::string& camFn);
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
			float MinDistance;
			float MaxDistance;
			int ContourThreshold;
            int forceRuneType;
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
			int kmeansClasses;
			int kmeansItrTimes;
        } lightSetting;

        struct VoteSetting
        {
            int saveTime;
            int SRSaveTime;
        } voteSetting;

        struct GimbalSetting
        {
            float ScaleX;
            float ScaleY;
            float ScaleZ;
            float GimbalX;
            float GimbalY;
            float GimbalZ;
			float CameraTheta;
			float ShootingSpeed;
        } gimbalSetting;

      private:
        std::string filename;
        std::string camFilename;
        bool fileExist(const std::string& filename);
};
