# VideoPlayer-BackendProcessing-OpenGL

A simple video player backend built using FFmpeg for video decoding and OpenGL/GLFW for rendering, designed to process and display video frames with customizable transformations. This project currently supports playing a video with the option to double the width using a bilinear scaling shader, maintaining the original height for a stretched effect.

## Features
- Decodes video files using FFmpeg's software decoding pipeline.
- Renders video frames in an OpenGL window using GLFW.
- Supports doubling the video width with smooth bilinear interpolation via an OpenGL shader.
- Maintains even dimensions for compatibility with FFmpeg's scaling requirements.
- Robust error handling to prevent crashes (e.g., access violations).
- MSVC-compatible file handling using `fopen_s`.

## Requirements
- **Operating System**: Windows (tested on Windows 10/11)
- **Compiler**: Microsoft Visual Studio (tested with VS 2022)
- **Dependencies**:
  - **FFmpeg**: Libraries (`libavformat`, `libavcodec`, `libavutil`, `libswscale`) and DLLs (`avformat-61.dll`, `avcodec-61.dll`, `avutil-59.dll`, `swscale-8.dll`,`avdevice-61`,`avfilter-10.dll`,`swresample-5.dll`,`postproc-58.dll` )
  - **GLEW**: OpenGL Extension Wrangler Library (`glew32.lib`, `glew32.dll`)
  - **GLFW**: Window and input handling (`glfw3.lib`, `glfw3.dll`)
  - **OpenGL**: Provided by Windows (`opengl32.lib`)

## Setup Instructions

### Explanation of Dependencies
Here’s a detailed breakdown of the dependencies (GLEW, FFmpeg, GLFW, and OpenGL), including their libraries, DLLs, and how they integrate into the project.

#### 1. GLEW (OpenGL Extension Wrangler Library)
- **Purpose**: Simplifies the use of OpenGL extensions by loading them at runtime. The code uses GLEW to access modern OpenGL functions (e.g., for shaders and textures).
- **Files Needed**:
  - **Header**: `include/GL/glew.h`
  - **Static Library**: `lib/Release/x64/glew32.lib` (links at compile time)
  - **DLL**: `bin/Release/x64/glew32.dll` (required at runtime)
- **Why Both Lib and DLL?**: The `glew32.lib` file tells the linker how to interface with `glew32.dll`, which contains the actual runtime code for OpenGL extension management.
- **Source**: Download from [GLEW Releases](http://glew.sourceforge.net/) or [GitHub](https://github.com/nigels-com/glew/releases).

#### 2. FFmpeg
- **Purpose**: Handles video file parsing, decoding, and color space conversion. The code uses FFmpeg’s `libavformat` (file I/O), `libavcodec` (decoding), `libavutil` (utilities), and `libswscale` (YUV-to-RGB conversion).
- **Files Needed**:
  - **Headers**: `include/libavformat/avformat.h`, `include/libavcodec/avcodec.h`, `include/libavutil/imgutils.h`, `include/libswscale/swscale.h`
  - **Static Libraries**: `lib/avformat.lib`, `lib/avcodec.lib`, `lib/avutil.lib`, `lib/swscale.lib` (links at compile time)
  - **DLLs**: `bin/avformat-61.dll`, `bin/avcodec-61.dll`, `bin/avutil-59.dll`, `bin/swscale-8.dll` (required at runtime)
- **Why Both Lib and DLL?**: The `.lib` files are import libraries that provide the interface for the linker to connect to the corresponding `.dll` files, which contain the actual FFmpeg functionality at runtime.
- **Source**: Download a shared and dev build from [FFmpeg Builds](https://github.com/BtbN/FFmpeg-Builds/releases) or [gyan.dev](https://www.gyan.dev/ffmpeg/builds/). Ensure the version matches your DLLs (e.g., 7.0 for `avcodec-61.dll`).

#### 3. GLFW
- **Purpose**: Creates windows, handles input, and manages the OpenGL context. The code uses GLFW to open a window and render video frames.
- **Files Needed**:
  - **Header**: `include/GLFW/glfw3.h`
  - **Static Library**: `lib-vc2022/glfw3.lib` (links at compile time, adjust for your VS version)
  - **DLL**: `glfw3.dll` (required at runtime, location varies by build)
- **Why Both Lib and DLL?**: Similar to GLEW and FFmpeg, `glfw3.lib` provides the linker interface to `glfw3.dll`, which handles window and context management at runtime.
- **Source**: Download pre-compiled binaries or source from [GLFW Downloads](https://www.glfw.org/download.html).

#### 4. OpenGL
- **Purpose**: Provides the graphics API for rendering video frames. The code uses OpenGL to create textures, shaders, and render a quad.
- **Files Needed**:
  - **Header**: Provided by Windows SDK (`include/GL/gl.h`, included via GLEW)
  - **Static Library**: `opengl32.lib` (included with Windows SDK, links at compile time)
  - **DLL**: `opengl32.dll` (system-provided, no need to copy)
- **Why Only Lib?**: `opengl32.lib` interfaces with the system’s `opengl32.dll`, which is part of Windows and doesn’t need to be distributed.

### Configuring Dependencies in Visual Studio
Here’s a step-by-step guide to set up the dependencies in Visual Studio 2022 for the `VideoPlayer-BackendProcessing-OpenGL` project. This assumes you’ve downloaded FFmpeg, GLEW, and GLFW and have their `include`, `lib`, and `bin` folders ready.

#### Step 1: Organize Dependency Files
1. **FFmpeg**:
   - Example path: `C:\ffmpeg`
   - Folders: `C:\ffmpeg\include`, `C:\ffmpeg\lib`, `C:\ffmpeg\bin`
   - Copy DLLs (`avformat-61.dll`, `avcodec-61.dll`, `avutil-59.dll`, `swscale-8.dll`,`avdevice-61`,`avfilter-10.dll`,`swresample-5.dll`,`postproc-58.dll` ) from `C:\ffmpeg\bin` to your project’s `x64\Debug` and `x64\Release` directories (e.g., `VideoPlayer-BackendProcessing-OpenGL\x64\Debug`).
2. **GLEW**:
   - Example path: `C:\glew`
   - Folders: `C:\glew\include`, `C:\glew\lib\Release\x64`, `C:\glew\bin\Release\x64`
   - Copy `glew32.dll` from `C:\glew\bin\Release\x64` to where your `.sln` file is.

#### Step 2: Create or Open the Project
1. Open Visual Studio 2022 and create a new C++ project (e.g., Empty Project) or open your existing `VideoPlayer-BackendProcessing-OpenGL.sln`.
2. Add `main.cpp` (the code provided in the previous message) to the project.
3. Set the solution configuration to `Debug` and platform to `x64` (Solution Explorer > Solution Platforms > x64).

#### Step 3: Configure Project Properties
1. Right-click the project in Solution Explorer > Properties.
2. Ensure the configuration is set to `Debug` and platform to `x64` in the Property Pages dialog.
3. Update the following settings:
   - **C/C++ > General > Additional Include Directories**:
     ```
     C:\ffmpeg\include;C:\glew-2.1.0\include;C:\glfw-3.4.bin.WIN64\include
     ```
     Replace paths with your actual folder locations.
   - **Linker > General > Additional Library Directories**:
     ```
     C:\ffmpeg\lib;C:\glew-2.1.0\lib\Release\x64;C:\glfw-3.4.bin.WIN64\lib-vc2022
     ```
     Adjust paths as needed.
   - **Linker > Input > Additional Dependencies**:
     ```
     glew32.lib, glfw3.lib, opengl32.lib, user32.lib, gdi32.lib, shell32.lib, avcodec.lib, avdevice.lib, avfilter.lib, avformat.lib, avutil.lib, postproc.lib, swresample.lib, swscale.lib
     ```
4. Repeat for the `Release` configuration if you plan to build in Release mode (optional for now, as you’re using Debug).

#### Step 4: Verify DLL Placement
- Ensure the following DLLs are in `x64\Debug` (relative to your project’s executable):
  - `avformat-61.dll`
  - `avcodec-61.dll`
  - `avdevice-61.dll`
  - `avfilter-10.dll`
  - `avutil-59.dll`
  - `swscale-8.dll`
  - `swresample-5.dll`
  - `postproc-58.dll`
- If Visual Studio can’t find the DLLs at runtime, you’ll get an error like “The application was unable to start correctly (0xc000007b)”. Copying them to the executable directory resolves this.

#### Step 5: Prepare the Video File
- Place a valid H.264 MP4 video at `C:\Users\ferni\Videos\Captures\video.mp4`.
- Alternatively, edit `main.cpp` to update `videoPath`:
  ```cpp
  const char* videoPath = "path\\to\\your\\video.mp4";
