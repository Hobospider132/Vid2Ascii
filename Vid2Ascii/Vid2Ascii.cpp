#include <iostream>
#include <string>
#include <regex>
#include <filesystem>
#include <fstream>
#include <opencv2/opencv.hpp>
#include <chrono>
#include <thread>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/ioctl.h>
#include <unistd.h>
#endif

void clearConsole() {
#ifdef _WIN32
    COORD topLeft = { 0, 0 };
    HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO screen;
    DWORD written;

    GetConsoleScreenBufferInfo(consoleHandle, &screen);
    FillConsoleOutputCharacterA(consoleHandle, ' ', screen.dwSize.X * screen.dwSize.Y, topLeft, &written);
    FillConsoleOutputAttribute(consoleHandle, FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_BLUE,
        screen.dwSize.X * screen.dwSize.Y, topLeft, &written);
    SetConsoleCursorPosition(consoleHandle, topLeft);
#else
    std::cout << "\033[2J\033[H";
#endif
}

void displayTextFile(const std::string& fileName) {
    std::ifstream inputFile(fileName);
    if (inputFile.is_open()) {
        std::string fileContent((std::istreambuf_iterator<char>(inputFile)),
            std::istreambuf_iterator<char>());
        std::cout << fileContent;
        inputFile.close();
    }
    else {
        std::cerr << "Error opening file: " << fileName << std::endl;
    }
}


void setConsoleSize(int width, int height) {
#ifdef _WIN32
    HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);

    COORD newSize;
    newSize.X = static_cast<SHORT>(width);
    newSize.Y = static_cast<SHORT>(height);

    SetConsoleScreenBufferSize(consoleHandle, newSize);

    SMALL_RECT windowRect;
    windowRect.Left = 0;
    windowRect.Top = 0;
    windowRect.Right = newSize.X - 1;
    windowRect.Bottom = newSize.Y - 1;

    SetConsoleWindowInfo(consoleHandle, TRUE, &windowRect);
#else 
    std::cout << "\033[8;" << height << ";" << width << "t";
#endif
}

namespace fs = std::filesystem;

std::pair<int, int> getConsoleWindowSize() {
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    return { csbi.srWindow.Right - csbi.srWindow.Left + 1, csbi.srWindow.Bottom - csbi.srWindow.Top + 1 };
#else
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    return { w.ws_col, w.ws_row };
#endif
}

std::string getTime() {
    std::chrono::system_clock::time_point currentTime = std::chrono::system_clock::now();

    std::time_t currentTime_t = std::chrono::system_clock::to_time_t(currentTime);

    std::stringstream ss;

    struct tm localTime;
    if (localtime_s(&localTime, &currentTime_t) == 0) {
        ss << std::put_time(&localTime, "%d-%m-%Y %H.%M.%S");
    }
    else {
        // Gonna be super awkward in the logs if this happens
        ss << "Error getting local time";
    }
    return ss.str();
}


void progressBar(int progressPercentage) {
    int barWidth = 70;
    int pos = barWidth * progressPercentage / 100;

    std::cout << "Converting frames...  [";
    for (int i = 0; i < barWidth; ++i) {
        if (i < pos) {
            std::cout << "=";
        }
        else if (i == pos) {
            std::cout << ">";
        }
        else {
            std::cout << " ";
        }
    }

    std::cout << "] " << progressPercentage << " %\r";
    std::cout.flush();
}

void vid2ascii(const cv::Mat& frame, const std::string& fileName) {
    std::ofstream frameFile(fileName);
    const std::string asciiChars = "&@%#*+=-:. ";
    if (frameFile.is_open()) {
        for (int i = 0; i < frame.rows; ++i) {
            for (int j = 0; j < frame.cols; ++j) {
                uchar intensity = frame.at<uchar>(i, j);
                int index = static_cast<int>(intensity / 255.0 * (asciiChars.length() - 1));
                char asciiChar = asciiChars[index];
                frameFile << asciiChar;
            }
            frameFile << std::endl;
        }

        frameFile.close();
    }
    else {
        std::cerr << "Error creating text file: " << fileName << std::endl;
    }
}

void blendFrames(const cv::Mat& currentFrame, const cv::Mat& previousFrame, double alpha, cv::Mat& blendedFrame) {
    cv::addWeighted(currentFrame, alpha, previousFrame, 1.0 - alpha, 0.0, blendedFrame);
}


int main() {
    std::string time = getTime();
    std::string dir = "log/";
    std::error_code ec;

    bool tempLogFile = true;

    if (!fs::is_directory(dir)) {
        try {
            fs::create_directory(dir, ec);
        }
        catch (const std::filesystem::filesystem_error& ex) {
            std::cerr << "Error creating directory: " << ex.what() << std::endl;
            tempLogFile = false;
        }
    }

    std::ofstream logFile(dir + getTime() + ".log");
    logFile << getTime() + " ";
    logFile << "Successfully created log file" << std::endl;

    if (tempLogFile == true) {
        logFile << time + " ";
        logFile << "Created log directory successfully" << std::endl;
    }
    else {
        logFile << time + " ";
        logFile << "Error log creating directory frames/" << std::endl;
    }

    bool valid = false;
    std::string file_path;

    while (!valid) {
        std::cout << "Please provide file path to a .mp4: ";
        std::getline(std::cin, file_path);

        std::regex pattern("\\.mp4");
        std::replace(file_path.begin(), file_path.end(), '\\', '/');

        if (std::regex_search(file_path, pattern) && fs::exists(file_path)) {
            valid = true;
            logFile << getTime() + " ";
            logFile << "Valid set to true" << std::endl;
        }
        else {
            std::cout << "Invalid file path, please check that the file exists and is an mp4 file" << std::endl;
        }
    }

    dir = "frames/";

    if (!fs::is_directory(dir)) {
        try {
            fs::create_directory(dir, ec);
            logFile << "Created directory successfully" << std::endl;
        }
        catch (const std::filesystem::filesystem_error& ex) {
            logFile << getTime() + " ";
            logFile << "Error creating directory frames/" << std::endl;
            std::cerr << "Error creating directory: " << ex.what() << std::endl;
        }
    }

    cv::VideoCapture cap;
    cap.open(file_path);

    if (!cap.isOpened()) {
        logFile << getTime() + " ";
        logFile << "Error: unable to open file" << file_path << std::endl;
        std::cerr << "Unable to open .mp4, please check video integrity." << std::endl;
        std::cout << "Press any key to exit...";
        cv::waitKey(0);
        return -1;
    }

    clearConsole();

    cv::Mat frame;
    cv::Mat prevFrame;
    bool firstFrame = true;

    int totalFrames = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_COUNT));
    int frameNumber = 0;

    while (cap.read(frame)) {
        cv::resize(frame, frame, cv::Size(), 0.3, 0.15);
        cv::cvtColor(frame, frame, cv::COLOR_BGR2GRAY);

        cv::Mat blendedFrame;
        if (firstFrame) {
            blendedFrame = frame.clone();
            firstFrame = false;
        }
        else {
            blendFrames(frame, prevFrame, 0.5, blendedFrame);  
        }

        std::string frameName = dir + "frame_" + std::to_string(frameNumber) + ".txt";
        vid2ascii(blendedFrame, frameName);

        float progress = static_cast<float>(frameNumber) / totalFrames;
        int progressPercentage = static_cast<int>(std::round(progress * 100.0));

        progressBar(progressPercentage);

        frameNumber++;

        prevFrame = frame.clone();  
    }


    std::cout << "Converted video to: " << frameNumber << " frames" << std::endl;
    logFile << getTime() + " ";
    logFile << "Converted " << frameNumber << " frames" << std::endl;

    // Reset cap and frame number to convert each frame to ASCII
    clearConsole();
    cap.set(cv::CAP_PROP_POS_FRAMES, 0);

    auto consoleSize = getConsoleWindowSize();
    int consoleCols = consoleSize.first;
    int consoleRows = consoleSize.second;

    double scaleX = static_cast<double>(consoleCols) / frame.cols;
    double scaleY = static_cast<double>(consoleRows) / frame.rows;

    std::string frameDirectory = "frames/";
    double framesPerSecond = cap.get(cv::CAP_PROP_FPS);

    for (int frameNumber = 0; frameNumber < totalFrames; ++frameNumber) {
        std::string frameName = frameDirectory + "frame_" + std::to_string(frameNumber) + ".txt";
        displayTextFile(frameName);

        // Calculate sleep time in milliseconds for a smoother transition
        int sleepTime = static_cast<int>(1000.0 / framesPerSecond);
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepTime));

    }

    cap.release();
    logFile.close();

    for (int frameNumber = 0; frameNumber < totalFrames; ++frameNumber) {
        std::string frameName = frameDirectory + "frame_" + std::to_string(frameNumber) + ".txt";
        fs::remove(frameName);
    }

    return 0;
}
