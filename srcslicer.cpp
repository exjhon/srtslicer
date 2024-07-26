#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <iomanip>
#include <algorithm>
#include <cstdlib>

#ifdef _WIN32
#include <windows.h>
#include <stringapiset.h> // For WideCharToMultiByte
#else
#include <unistd.h>
#endif

struct Subtitle {
    int index;
    std::string startTime;
    std::string endTime;
    std::string text;
};

// Function to convert UTF-8 to system encoding on Windows
std::string utf8ToSystem(const std::string& utf8Str) {
#ifdef _WIN32
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), static_cast<int>(utf8Str.size()), NULL, 0);
    std::wstring wideStr(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), static_cast<int>(utf8Str.size()), &wideStr[0], size_needed);

    size_needed = WideCharToMultiByte(CP_ACP, 0, wideStr.c_str(), static_cast<int>(wideStr.size()), NULL, 0, NULL, NULL);
    std::string systemStr(size_needed, 0);
    WideCharToMultiByte(CP_ACP, 0, wideStr.c_str(), static_cast<int>(wideStr.size()), &systemStr[0], size_needed, NULL, NULL);

    return systemStr;
#else
    return utf8Str;
#endif
}

// Function to remove illegal characters from text
std::string sanitizeFilename(const std::string& text) {
    std::string sanitized = text;
    // Define the illegal characters
    const std::string illegalChars[] = { "?", ":", "*", "\"" };

    for (const auto& ch : illegalChars) {
        size_t pos = 0;
        while ((pos = sanitized.find(ch, pos)) != std::string::npos) {
            sanitized.replace(pos, ch.length(), ""); // Remove the illegal character
        }
    }

    return sanitized;
}

std::vector<Subtitle> parseSrt(const std::string& filePath) {
    std::vector<Subtitle> subtitles;
    std::ifstream file(filePath);
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue; // Skip empty lines

        Subtitle subtitle;
        try {
            subtitle.index = std::stoi(line);
        }
        catch (const std::invalid_argument&) {
            std::cerr << "Invalid subtitle index: " << line << std::endl;
            continue;
        }
        std::getline(file, line);
        std::istringstream timeStream(line);
        std::getline(timeStream, subtitle.startTime, ' ');
        timeStream.ignore(4, ' '); // skip " --> "
        std::getline(timeStream, subtitle.endTime);

        // Replace ',' with '.' in timestamps
        std::replace(subtitle.startTime.begin(), subtitle.startTime.end(), ',', '.');
        std::replace(subtitle.endTime.begin(), subtitle.endTime.end(), ',', '.');

        std::string text;
        while (std::getline(file, line) && !line.empty()) {
            text += line + " ";
        }
        subtitle.text = sanitizeFilename(text); // Sanitize the subtitle text
        subtitles.push_back(subtitle);
    }
    return subtitles;
}

std::string escapeShellArg(const std::string& arg) {
    std::string escapedArg = "\"";
    for (char c : arg) {
        if (c == '\"') {
            escapedArg += "\\\"";
        }
        else {
            escapedArg += c;
        }
    }
    escapedArg += "\"";
    return escapedArg;
}

// Function to convert timestamp to milliseconds
std::string timeToMilliseconds(const std::string& timeStr) {
    int hours, minutes, seconds, milliseconds;
    char dummy; // For consuming the ':' and ',' characters
    std::stringstream timeStream(timeStr);
    timeStream >> hours >> dummy >> minutes >> dummy >> seconds >> dummy >> milliseconds;

    int totalMilliseconds = ((hours * 3600) + (minutes * 60) + seconds) * 1000 + milliseconds;
    std::ostringstream oss;
    oss << std::setw(9) << std::setfill('0') << totalMilliseconds; // Ensure 9 digits with leading zeros
    return oss.str();
}

void splitWav(const std::string& wavPath, const std::vector<Subtitle>& subtitles) {
    std::string outputDir = wavPath.substr(0, wavPath.find_last_of("/\\"));
    std::string baseFileName = wavPath.substr(wavPath.find_last_of("/\\") + 1);
    baseFileName = baseFileName.substr(0, baseFileName.find_last_of('.')); // Remove the extension
    for (const auto& subtitle : subtitles) {
        std::string startTimeMs = timeToMilliseconds(subtitle.startTime);
        std::string endTimeMs = timeToMilliseconds(subtitle.endTime);
        std::string outputFileName = outputDir + "/" + baseFileName + "_" + std::to_string(subtitle.index) + "_" + startTimeMs + "_" + endTimeMs + "_" + subtitle.text + ".wav";
        outputFileName = utf8ToSystem(outputFileName); // Convert UTF-8 to system encoding if necessary
        std::string command = "ffmpeg -i " + escapeShellArg(wavPath) +
            " -ss " + subtitle.startTime +
            " -to " + subtitle.endTime +
            " -c copy " + escapeShellArg(outputFileName);
        std::cout << "Executing command: " << command << std::endl;
        int result = system(command.c_str());
        if (result != 0) {
            std::cerr << "Command failed with error code: " << result << std::endl;
        }
    }
}

int main() {
    std::string wavPath;
    std::string srtPath;

    std::cout << "请输入.wav文件路径: ";
    std::getline(std::cin, wavPath);
    std::cout << "请输入.srt文件路径: ";
    std::getline(std::cin, srtPath);

    std::vector<Subtitle> subtitles = parseSrt(srtPath);
    splitWav(wavPath, subtitles);

    std::cout << "音频切分完成！" << std::endl;
    return 0;
}
