# SagoImageBrowser

A Qt6-based image browser application for viewing and managing image collections.

## Features

- Browse images in a directory tree with file system navigation
- Multi-threaded, cached thumbnail generation for a responsive UI
- Single image view mode with keyboard-driven zoom, pan, fit and navigation
- Fullscreen viewing
- EXIF/IPTC metadata panel (camera, exposure, ISO, focal length, dimensions, …)
- Geo location with a clickable OpenStreetMap link when an image is geotagged
- Edit image captions (written to the IPTC Caption-Abstract tag)
- Lossless 90° rotation (updates the EXIF orientation tag, pixel data untouched)
- Move images to trash and delete files
- Right-click context menus (including copying file paths), also in fullscreen
- Configurable keyboard shortcuts
- Preferences for background color, thumbnail cache size, dock locking and layout
- Path entry field with autocomplete
- Translations (Danish included)
- Supports every image format provided by the installed Qt image plugins

## Requirements

- C++17 or later
- CMake 3.21+
- Qt6 (Widgets and LinguistTools components)
- Git and a network connection — Exiv2 (v0.28.8) is fetched and statically linked
  at configure time via CMake `FetchContent`

## Building

### Prerequisites

Install Qt6 development files:

```bash
# On Ubuntu/Debian
sudo apt-get install qt6-base-dev qt6-tools-dev

# On macOS with Homebrew
brew install qt6
```

### Build Steps

```bash
cd SagoImageBrowser
cmake -B build
cmake --build build
```

The compiled executable will be at `./build/SagoImageBrowser`.

### Installing

```bash
cmake --install build
```

This installs the executable, a `.desktop` entry and the application icon.

## Running

```bash
./build/SagoImageBrowser [path]
```

An optional directory or image path can be passed on the command line.

## Tests

A small test for the EXIF reader is built alongside the application:

```bash
./build/test_exifreader
```

## Project Structure

```
src/
  ├── main.cpp                  # Application entry point
  ├── mainwindow.h/cpp          # Main application window
  ├── imagemodel.h/cpp          # Image list/grid data model
  ├── fsdirmodel.h/cpp          # Directory tree model
  ├── imageviewwidget.h/cpp     # Single-image display widget (zoom/pan)
  ├── thumbnailworker.h/cpp     # Background thumbnail generation
  ├── thumbnailcache.h/cpp      # Thumbnail caching
  ├── thumbnaildelegate.h/cpp   # Thumbnail rendering in the view
  ├── exifreader.h/cpp          # EXIF/IPTC reading, caption writing, rotation
  ├── preferencesdialog.h/cpp   # Preferences dialog
  ├── shortcutmanager.h/cpp     # Configurable keyboard shortcuts
  └── pathcompletermodel.h/cpp  # Autocomplete for the path field
test/                           # Unit tests
translations/                   # Qt translation files (.ts)
extra/                          # Desktop entry and icons
CMakeLists.txt                  # Build configuration
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
