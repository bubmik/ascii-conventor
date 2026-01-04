# ASCII Converter (Video to ASCII)

ASCII Converter is a C++ console application that converts a video file into ASCII art and plays it directly in the Command Prompt (CMD)

The program processes the video frame by frame and renders it using text characters, creating a retro terminal-style animation

## What is this program?

ascii-converter converts a video into an ASCII animation that is displayed directly in the terminal

Instead of pixels, the image is drawn using text characters, giving a classic ASCII-art look while the video is playing

---

## Features

- Converts video to ASCII art
- Real-time rendering in CMD
- Uses FFmpeg for video decoding
- Lightweight console application
- Written in C++

## Requirements

- Windows
- Command Prompt (CMD)
- FFmpeg
- C++ compiler (only if building from source)

## How to use (ONLY WINDOWS!)

### Step 1: Create a folder
Create a folder with any name.

### Step 2: Add the executable
Place the file:
ascii-convertor.exe
inside the folder

### Step 3: Add a video
- Put any video file with the .mp4 extension into the same folder
- Rename the video to: ascii-convert.mp4

Example folder structure

ascii-player/
├─ ascii-convertor.exe
└─ ascii-convert.mp4

### Step 4: Run the program
Run ascii-convertor.exe

### Step 5: Watch
The video will be converted and displayed as ASCII art directly in the CMD window!

## Beta information

This program is currently in beta testing
Bugs, performance issues, or unexpected behavior may occur

## FFmpeg installation and setup (Windows)

FFmpeg is required for this program to work.

### Download FFmpeg

1. Open the official website:
https://ffmpeg.org/download.html
2. Download a Windows build
3. Extract the downloaded archive

### Add FFmpeg to PATH

1. Move the extracted FFmpeg folder to: C:\ffmpeg

2. Make sure the following path exists: C:\ffmpeg\bin

3. Press Win + R and type: sysdm.cpl

4. Open: Advanced -> Environment Variables

5. In System variables, find "Path" and click Edit

6. Add a new entry: C:\ffmpeg\bin

7. Restart Command Prompt

### Verify FFmpeg installation

Open CMD and run:
ffmpeg -version

If version information is displayed, FFmpeg is installed correctly

## Build from source (optional)

Example build command: g++ ascii-main.cpp -o ascii-convertor -lavformat -lavcodec -lavutil

## Author: th3traizy

## License

This project is provided as-is for educational and experimental purposes
