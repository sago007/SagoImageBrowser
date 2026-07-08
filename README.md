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
- Exiv2 development files (0.27 or 0.28) installed on the system, e.g. `libexiv2-dev`

## Building

### Prerequisites

Install the Qt6 and Exiv2 development files:

```bash
# On Ubuntu/Debian
sudo apt-get install qt6-base-dev qt6-tools-dev libexiv2-dev

# On macOS with Homebrew
brew install qt6 exiv2
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

GPL 3.0 or later, see `LICENSE` file for details.

Some files might have more liberal licenses, see individual file headers for details.

