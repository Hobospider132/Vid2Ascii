#include <iostream>
#include <string>
#include <regex>
#include <filesystem>
#include <fstream>
#include <opencv2/opencv.hpp>
#include <ctime>
#include <iomanip>
#include <limits>

namespace fs = std::filesystem;


class LogTime {
public:
    std::string getTime() const {
        std::chrono::system_clock::time_point currentTime = std::chrono::system_clock::now();

        std::time_t currentTime_t = std::chrono::system_clock::to_time_t(currentTime);

        std::stringstream ss;

        struct tm localTime;
        if (localtime_s(&localTime, &currentTime_t) == 0) {
            ss << std::put_time(&localTime, "%d-%m-%Y %H,%M,%S");
        }
        else {
            // Gonna be super awkward in the logs if this happens
            ss << "Error getting local time";
        }
        return ss.str();
    }
};

int main() {
    LogTime logTime;

    std::string fileName = logTime.getTime() + ".log";
    std::ofstream logFile(fileName);
    logFile << logTime.getTime() + " ";
    logFile << "Sucessfully created log file" << std::endl;

    bool valid = false;

    std::string file_path;

    while (!valid) {
        std::cout << "Please provide file path to a .mp4: ";
        std::getline(std::cin, file_path);

        std::regex pattern("\\.mp4");
        std::replace(file_path.begin(), file_path.end(), '\\', '/');

        if (std::regex_search(file_path, pattern) && fs::exists(file_path)) {
            valid = true;
            logFile << logTime.getTime() + " ";
            logFile << "Valid set to true" << std::endl;
        }
        else {
            std::cout << "Invalid file path, please check that the file exists and is a mp4 file" << std::endl;
        }
    } 

    std::string dir = "frames/";
    std::error_code ec;

    if (!fs::is_directory(dir)) {
        try {
            fs::create_directory(dir, ec);
            logFile << "Created directory successfully" << std::endl;
        }
        catch (const std::filesystem::filesystem_error& ex) {
            logFile << logTime.getTime() + " ";
            logFile << "error creating directory" << std::endl;
            std::cerr << "Error creating directory: " << ex.what() << std::endl;
            std::cout << "Press any key to exit...";
            cv::waitKey(0);
            return -1;
        }
    }

    cv::VideoCapture cap;
    cap.open("test.mp4");
    cv::Mat frame;

    double fps = cap.get(cv::CAP_PROP_FPS);

    int frameNumber = 0;

    while (cap.read(frame)) {
        cv::cvtColor(frame, frame, cv::COLOR_BGR2GRAY);
        std::string frameName = dir + "frame_" + std::to_string(frameNumber) + ".png";
        cv::imwrite(frameName, frame);
        frameNumber++;
    }

    if (!cap.isOpened()) {
        logFile << logTime.getTime() + " ";
        logFile << "Error: unable to open file" << std::endl;
        std::cerr << "Unable to open .mp4, please check video integrity." << std::endl;
        std::cout << "Press any key to exit...";
        cv::waitKey(0);
        return -1;
    } 

    std::cout << "Converted video to: " << frameNumber << "frames" << std::endl;
    logFile << logTime.getTime() + " ";
    logFile << "Converted" << frameNumber << "frames" << std::endl;

    cap.release();
    logFile.close();

    return 0;
}