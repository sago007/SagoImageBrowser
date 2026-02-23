# SagoImageBrowser

A Qt6-based image browser application for viewing and managing image collections.

## Features

- Browse images in a directory tree
- Thumbnail preview of images
- Single image view mode with keyboard navigation
- File system navigation
- Multi-threaded thumbnail generation for responsive UI

## Requirements

- C++17 or later
- CMake 3.21+
- Qt6 (Widgets component)

## Building

### Prerequisites

Install Qt6 development files:

```bash
# On Ubuntu/Debian
sudo apt-get install qt6-base-dev

# On macOS with Homebrew
brew install qt6
```

### Build Steps

```bash
cd SagoImageBrowser
mkdir build
cd build
cmake ..
make
```

The compiled executable will be at `./build/SagoImageBrowser`

## Running

```bash
./build/SagoImageBrowser
```

## Project Structure

```
src/
  ├── main.cpp                 # Application entry point
  ├── mainwindow.h/cpp         # Main application window
  ├── imagemodel.h/cpp         # Image data model
  ├── imageviewwidget.h/cpp    # Image display widget
  └── thumbnailworker.h/cpp    # Background thumbnail generation
CMakeLists.txt                 # Build configuration
```

## License

MIT Expat License

Copyright (c) 2026

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
