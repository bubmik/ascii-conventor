#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <windows.h>
#include <thread>
#include <chrono>
#include <cmath>
#include <sstream>
#include <mmsystem.h>
#include <algorithm>

#pragma comment(lib, "winmm.lib")

class ASCIIVideoPlayer {
private:
    int frameRate;
    int consoleWidth;
    int consoleHeight;
    std::string videoPath;
    FILE* ffmpegPipe;
    std::thread audioThread;
    bool stopAudio;
    int targetWidth;
    int targetHeight;

    const char* ASCII_CHARS = " .:-=+*#%@";

    void setupFullscreenConsole() {
        HWND console = GetConsoleWindow();
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

        CONSOLE_FONT_INFOEX cfi;
        cfi.cbSize = sizeof(cfi);
        cfi.nFont = 0;
        cfi.dwFontSize.X = 1;
        cfi.dwFontSize.Y = 1;
        cfi.FontFamily = FF_DONTCARE;
        cfi.FontWeight = FW_NORMAL;
        wcscpy_s(cfi.FaceName, L"Terminal");
        SetCurrentConsoleFontEx(hConsole, FALSE, &cfi);

        Sleep(100);

        ShowWindow(console, SW_MAXIMIZE);
        Sleep(300);

        CONSOLE_SCREEN_BUFFER_INFO csbi;
        GetConsoleScreenBufferInfo(hConsole, &csbi);

        SHORT maxWidth = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        SHORT maxHeight = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;

        COORD bufferSize;
        bufferSize.X = maxWidth;
        bufferSize.Y = maxHeight;
        SetConsoleScreenBufferSize(hConsole, bufferSize);

        GetConsoleScreenBufferInfo(hConsole, &csbi);
        consoleWidth = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        consoleHeight = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;

        std::cout << "Fullscreen console: " << consoleWidth << "x" << consoleHeight << " chars\n";
    }

    void hideCursor() {
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_CURSOR_INFO cursorInfo;
        GetConsoleCursorInfo(hConsole, &cursorInfo);
        cursorInfo.bVisible = false;
        SetConsoleCursorInfo(hConsole, &cursorInfo);

        DWORD mode;
        GetConsoleMode(hConsole, &mode);
        mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(hConsole, mode);
    }

    void frameToAscii(const std::vector<unsigned char>& frameData, char* output) {
        float scaleX = (float)targetWidth / consoleWidth;
        float scaleY = (float)targetHeight / consoleHeight;

        int pos = 0;
        const int asciiLen = strlen(ASCII_CHARS) - 1;

        for (int y = 0; y < consoleHeight; y++) {
            int imgY = (int)(y * scaleY);
            if (imgY >= targetHeight) imgY = targetHeight - 1;

            for (int x = 0; x < consoleWidth; x++) {
                int imgX = (int)(x * scaleX);
                if (imgX >= targetWidth) imgX = targetWidth - 1;

                int idx = (imgY * targetWidth + imgX) * 3;
                int brightness = (frameData[idx] + frameData[idx + 1] + frameData[idx + 2]) / 3;
                output[pos++] = ASCII_CHARS[(brightness * asciiLen) / 255];
            }
            if (y < consoleHeight - 1) output[pos++] = '\n';
        }
        output[pos] = '\0';
    }

    void extractAudio() {
        std::cout << "Extracting audio...\n";
        std::string audioCmd = "ffmpeg -i \"" + videoPath + "\" -vn -acodec pcm_s16le -ar 44100 -ac 2 temp_audio.wav -y -hide_banner -loglevel error";
        system(audioCmd.c_str());
    }

    void playAudioThread() {
        Sleep(100);

        std::wstring wideAudio = L"temp_audio.wav";
        PlaySound(wideAudio.c_str(), NULL, SND_FILENAME | SND_ASYNC);

        while (!stopAudio) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        PlaySound(NULL, NULL, 0);
    }

    bool openVideoStream() {
        std::string probeCmd = "ffprobe -v error -select_streams v:0 -show_entries stream=width,height -of csv=s=x:p=0 \"" + videoPath + "\"";
        FILE* probe = _popen(probeCmd.c_str(), "r");
        int videoWidth = 1920, videoHeight = 1080;

        if (probe) {
            char buffer[128];
            if (fgets(buffer, sizeof(buffer), probe)) {
                sscanf_s(buffer, "%dx%d", &videoWidth, &videoHeight);
            }
            _pclose(probe);
        }

        std::cout << "Video: " << videoWidth << "x" << videoHeight << "\n";

        float videoAspect = (float)videoWidth / videoHeight;
        float consoleAspect = (float)consoleWidth / (consoleHeight * 0.5f);

        if (videoAspect > consoleAspect) {
            targetWidth = consoleWidth;
            targetHeight = (int)(targetWidth / videoAspect);
        }
        else {
            targetHeight = consoleHeight;
            targetWidth = (int)(targetHeight * videoAspect);
        }

        std::cout << "Target: " << targetWidth << "x" << targetHeight << " pixels\n";

        std::ostringstream cmd;
        cmd << "ffmpeg -i \"" << videoPath << "\" "
            << "-vf \"fps=" << frameRate << ",scale=" << targetWidth << ":" << targetHeight << "\" "
            << "-f rawvideo -pix_fmt rgb24 -an - 2>nul";

        ffmpegPipe = _popen(cmd.str().c_str(), "rb");

        return ffmpegPipe != nullptr;
    }

    void closeVideoStream() {
        if (ffmpegPipe) {
            _pclose(ffmpegPipe);
            ffmpegPipe = nullptr;
        }
    }

public:
    ASCIIVideoPlayer(const std::string& path, int fps)
        : videoPath(path), frameRate(fps), consoleWidth(80), consoleHeight(25),
        ffmpegPipe(nullptr), stopAudio(false), targetWidth(0), targetHeight(0) {
    }

    ~ASCIIVideoPlayer() {
        closeVideoStream();
        stopAudio = true;
        if (audioThread.joinable()) {
            audioThread.join();
        }
        system("del temp_audio.wav 2>nul");
    }

    void playStreaming() {
        std::cout << "Preparing fullscreen video with audio...\n\n";

        extractAudio();

        std::cout << "\nConfiguring fullscreen mode...\n";
        Sleep(1000);

        setupFullscreenConsole();

        std::cout << "Starting playback in 2 seconds...\n";
        std::cout << "Press ESC to exit\n";
        Sleep(2000);

        if (!openVideoStream()) {
            std::cerr << "Failed to open video stream!\n";
            return;
        }

        system("cls");
        hideCursor();

        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        COORD coord = { 0, 0 };

        DWORD mode;
        GetConsoleMode(hConsole, &mode);
        mode |= ENABLE_PROCESSED_OUTPUT;
        SetConsoleMode(hConsole, mode);

        int frameSize = targetWidth * targetHeight * 3;
        std::vector<unsigned char> frameBuffer(frameSize);
        int asciiBufferSize = (consoleWidth + 1) * consoleHeight + 1;
        char* asciiBuffer = new char[asciiBufferSize];

        int asciiLength = consoleWidth * consoleHeight + (consoleHeight - 1);

        int frameCount = 0;

        stopAudio = false;
        audioThread = std::thread(&ASCIIVideoPlayer::playAudioThread, this);

        auto videoStart = std::chrono::high_resolution_clock::now();
        auto frameDuration = std::chrono::microseconds(1000000 / frameRate);

        while (true) {
            auto frameStart = std::chrono::high_resolution_clock::now();

            if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
                break;
            }

            size_t bytesRead = fread(frameBuffer.data(), 1, frameSize, ffmpegPipe);
            if (bytesRead != frameSize) {
                break;
            }

            frameToAscii(frameBuffer, asciiBuffer);

            SetConsoleCursorPosition(hConsole, coord);
            DWORD written;
            WriteConsoleA(hConsole, asciiBuffer, asciiLength, &written, NULL);

            frameCount++;

            auto expectedTime = videoStart + (frameDuration * frameCount);
            auto now = std::chrono::high_resolution_clock::now();

            if (now < expectedTime) {
                std::this_thread::sleep_until(expectedTime);
            }
        }

        delete[] asciiBuffer;

        stopAudio = true;
        if (audioThread.joinable()) {
            audioThread.join();
        }

        closeVideoStream();

        std::cout << "\n\nPlayback finished! Frames: " << frameCount << "\n";
        system("del temp_audio.wav 2>nul");
    }
};

bool checkFFmpeg() {
    std::cout << "Checking for FFmpeg...\n";

    if (std::ifstream("ffmpeg\\bin\\ffmpeg.exe").good()) {
        std::cout << "Found: ffmpeg\\bin\\ffmpeg.exe (local)\n";
        SetEnvironmentVariableA("PATH", "ffmpeg\\bin;%PATH%");
        return true;
    }

    if (std::ifstream("ffmpeg.exe").good()) {
        std::cout << "Found: ffmpeg.exe (current directory)\n";
        return true;
    }

    if (system("ffmpeg -version >nul 2>nul") == 0) {
        std::cout << "Found: ffmpeg in system PATH\n";
        return true;
    }

    std::cerr << "\nERROR! FFmpeg not found!\n";
    std::cerr << "Please use \"run_ffmpeg.bat\" or contact me...\n";
    std::cerr << "Discord: @th3traizy\n\n";

    return false;
}

std::string toLower(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
    return str;
}

bool getYesNo(const std::string& prompt, bool defaultValue) {
    std::string input;
    std::cout << prompt;
    std::getline(std::cin, input);

    input = toLower(input);
    if (input.empty()) return defaultValue;
    return (input[0] == 'y');
}

int main() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    system("chcp 65001 > nul");

    std::cout << "=== ASCII Video Converter ===\n\n";

    if (!checkFFmpeg()) {
        system("pause");
        return 1;
    }

    std::string videoFile;
    std::cout << "\nEnter video file path (support: .mp4, .avi, .mov, .gif, etc.): ";
    std::getline(std::cin, videoFile);

    if (videoFile.front() == '"' && videoFile.back() == '"') {
        videoFile = videoFile.substr(1, videoFile.length() - 2);
    }

    std::ifstream file(videoFile);
    if (!file.good()) {
        std::cerr << "Error: File '" << videoFile << "' not found!\n";
        system("pause");
        return 1;
    }
    file.close();

    int frameRate;
    std::cout << "\nFrame rate (default 60): ";
    std::string fpsInput;
    std::getline(std::cin, fpsInput);
    if (fpsInput.empty()) {
        frameRate = 60;
    }
    else {
        frameRate = std::stoi(fpsInput);
        if (frameRate <= 0) frameRate = 60;
    }
    std::cout << "Using FPS: " << frameRate << "\n";

    bool start = getYesNo("\nStart? (y/n): ", false);

    if (!start) {
        std::cout << "Cancelled.\n";
        system("pause");
        return 0;
    }

    ASCIIVideoPlayer player(videoFile, frameRate);

    std::cout << "\nStarting fullscreen playback...\n";
    Sleep(1000);

    player.playStreaming();

    system("pause");
    return 0;
}