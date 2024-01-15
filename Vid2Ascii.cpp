/*
    TODO:

    [ ] Find out why log file does not get created
    [ ] Find out why cv cap isn't working
*/

#include <iostream>
#include <string>
#include <regex>
#include <filesystem>
#include <fstream>
#include <opencv2/opencv.hpp>
#include <ctime>
#include <iomanip>

namespace fs = std::filesystem;

class LogTime {
public:
    std::string getTime() const {
        std::chrono::system_clock::time_point currentTime = std::chrono::system_clock::now();

        std::time_t currentTime_t = std::chrono::system_clock::to_time_t(currentTime);

        std::stringstream ss;

        struct tm localTime;
        if (localtime_s(&localTime, &currentTime_t) == 0) {
            ss << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S");
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

    std::string time = logTime.getTime();
    std::string fileName = time + "_log.txt";
    std::cout << fileName;
    std::ofstream logFile(fileName);

    bool valid = false;

    std::string file_path;

    while (!valid) {
        std::cout << "Please provide file path to a .mp4: ";
        std::cin >> file_path;

        std::regex pattern("\\.mp4");

        if (std::regex_search(file_path, pattern) && fs::exists(file_path)) {
            valid = true;
            std::string time = logTime.getTime();
            logFile << time + " ";
            logFile << "Valid set to true" << std::endl;
        } else {
            std::cout << "Invalid file path, please check that the file exists and is a mp4 file" << std::endl;
        }
    }
    cv::VideoCapture cap(file_path);
    if (!cap.isOpened()) {
        std::string time = logTime.getTime();
        logFile << time + " ";
        std::cerr << "Unable to open .mp4, please check video integrity.\nPress any key to exit...";
        cv::waitKey(0);
        logFile << "Error: " + std::to_string(cv::getTickCount()) << std::endl;
        return -1;
    }

    double fps = cap.get(cv::CAP_PROP_FPS); 

    std::string dir = "frames/";
    std::error_code ec;

    if (!fs::is_directory(dir)) {
        std::string time = logTime.getTime();
        logFile << time + " ";
        fs::create_directory(dir, ec);
        logFile << "Created directory sucessfully" << std::endl;
    }

    if(ec) {
        std::string time = logTime.getTime();
        logFile << time + " ";
        std::cerr << "Error creating directory: " << ec.message() << std::endl;
        cv::waitKey(0); 
        return -1;
    }

    int frameNumber = 0;
    cv::Mat frame;
    
    while (cap.read(frame)) {
        cv::cvtColor(frame, frame, cv::COLOR_BGR2GRAY);
        std::string frameName = dir + "frame_" + std::to_string(frameNumber) + ".png";
        cv::imwrite(frameName, frame);
        frameNumber++;
    }
    
    cap.release(); */
    logFile.close();

    return 0;
}
