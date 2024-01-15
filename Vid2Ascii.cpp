/*
Steps:

1. Ask for Input:
   - Prompt the user to provide the input video file (e.g., in mp4 format).

2. Check File Existence and Format:
   - Verify that the provided file exists and has the correct format (e.g., .mp4).

3. Retrieve System Information:
   - Utilize platform-specific APIs or libraries to obtain information about the system, including:
     - Number of CPU cores.
     - Available RAM.
     - Current CPU usage.
     - Maybe use Poco or Boost

4. Frame Splitting:
   - Break down the input video into individual frames. Use a suitable library or tool for video decoding (e.g., OpenCV, FFmpeg) to extract frames from the video file.

5. Frame Conversion to ASCII:
   - Might have to use OpenCV for conversion
   - For each frame, convert the image data to ASCII art. This process involves mapping pixel values to corresponding ASCII characters based on intensity. Fine-tune the mapping to achieve the desired visual representation.

6. Batch Processing of Frames:
   - Organize the frames into batches for parallel processing. Determine the optimal batch size based on experimentation and system capabilities.

7. Smart CPU Allocation:
   - Dynamically adjust the program's CPU usage based on the current system load and available resources.
     - Set a default CPU limit (e.g., 10%).
     - Monitor system metrics (CPU usage, available RAM) in real-time.
     - Adjust the CPU limit dynamically to optimize resource utilization.

8. Parallel ASCII Conversion:
   - Utilize threading or asynchronous processing to concurrently convert frames within each batch. Ensure synchronization mechanisms to manage shared resources during parallel processing.

9. Load Balancing:
   - Implement load balancing strategies to evenly distribute the workload among CPU cores. This is crucial for optimizing the efficiency of parallel ASCII conversion.

10. Smart Batch Allocation Based on Cores:
    - Adjust the batch size dynamically based on the number of CPU cores.
      - Calculate an optimal batch size relative to the available CPU cores.
      - Experiment with different batch sizes to find the most efficient configuration.
      - Monitor and adapt the batch size dynamically during execution.

11. Optimize ASCII Conversion Algorithm:
    - Continuously optimize the ASCII conversion algorithm to enhance the efficiency of the mapping process. Consider parallel algorithms or vectorization techniques to improve performance.

12. Real-time Monitoring and Feedback:
    - Provide real-time feedback to the user regarding the progress of frame conversion. Display information such as the current frame being processed, batch progress, and overall conversion status.

13. User Configuration Options:
    - Allow users to configure key parameters, such as CPU limit, initial batch size, and other resource-related settings.
      - Provide flexibility for users to customize the program based on their preferences and system characteristics.

14. Error Handling and Graceful Exit:
    - Implement robust error handling mechanisms.
    - Gracefully handle scenarios where the program cannot adhere to specified resource limits.
    - Provide informative messages and allow users to adjust settings.

15. Testing and Optimization:
    - Conduct extensive real-world testing on different video lengths, system configurations, and workloads.
    - Profile and optimize the program based on observed performance characteristics.
    - Iterate on the implementation to achieve the best balance between efficiency and resource utilization.

*/

#include <iostream> 
#include <string>
#include <regex>
#include <filesystem>
#include <fstream>
#include <streambuf>
#include <opencv2/opencv.hpp>
#include <ctime>
#include <sstream>
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

class Logger : public std::streambuf {
public:

    Logger(std::ostream& logStream)
        : logStream(logStream) {}

    void log(const std::string& message) {
        logStream << message << std::endl;
    }

private:
    std::ostream& logStream;
};


int main() {
    LogTime logTime;

    std::string time = logTime.getTime();
    std::ofstream logFile(time+"_log.txt");
    Logger Log(logFile);

    bool valid = false;

    while (!valid) {
        std::string file_path;
        std::cout << "Please provide file path to a .mp4: ";
        std::cin >> file_path;


        std::regex pattern("\\.mp4");

        if (std::regex_search(file_path, pattern) && fs::exists(file_path)) {
            valid = true;
            std::string time = logTime.getTime();
            Log.log(time);
            Log.log("Valid set to true");
        } else {
            std::cout << "Invalid file path, please check that the file exists and is a mp4 file" << std::endl;
        }
    }

    cv::VideoCapture cap("file_path");
    if (!cap.isOpened()) {
        std::string time = logTime.getTime();
        Log.log(time);
        std::cerr << "Unable to open .mp4, please check video integrity.\nPress any key to exit...";
        cv::waitKey(0);
        return -1;
    }

    double fps = cap.get(cv::CAP_PROP_FPS);

    std::string dir = "frames/";
    std::error_code ec;

    if (!fs::is_directory(dir)) {
        std::string time = logTime.getTime();
        Log.log(time);
        fs::create_directory(dir, ec);
        Log.log("Created directory sucessfully");
    }

    if(ec) {
        std::string time = logTime.getTime();
        Log.log(time);
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
    
    cap.release();

    return 0;
}