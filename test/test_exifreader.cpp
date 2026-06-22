/*
MIT License

Copyright (c) 2026 Poul Sander

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

*/

#include <QCoreApplication>
#include <QDebug>
#include <cassert>
#include "exifreader.h"

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    qDebug() << "Running ExifReader test...";

    // Test ExifData::isEmpty()
    ExifData data;
    assert(data.isEmpty());
    qDebug() << "ExifData::isEmpty() passed for empty data.";

    data.filename = "test.jpg";
    assert(!data.isEmpty());
    qDebug() << "ExifData::isEmpty() passed for non-empty data.";

    // Test ExifData::toList()
    data.fileSize = "1.2 MiB";
    data.orientation = "1 (top, left)";
    data.latitude = "55deg 40' 32.12\" N";
    data.longitude = "12deg 34' 11.23\" E";
    data.osmLink = "https://www.openstreetmap.org/?mlat=55.675589&mlon=12.569786#map=16/55.675589/12.569786";
    auto list = data.toList();
    bool foundFilename = false;
    bool foundSize = false;
    bool foundOrientation = false;
    bool foundLat = false;
    bool foundLon = false;
    bool foundOsm = false;
    for (const auto &pair : list) {
        if (pair.first == "Filename" && pair.second == "test.jpg") foundFilename = true;
        if (pair.first == "File Size" && pair.second == "1.2 MiB") foundSize = true;
        if (pair.first == "Orientation" && pair.second == "1 (top, left)") foundOrientation = true;
        if (pair.first == "Latitude" && pair.second == "55deg 40' 32.12\" N") foundLat = true;
        if (pair.first == "Longitude" && pair.second == "12deg 34' 11.23\" E") foundLon = true;
        if (pair.first == "OpenStreetMap" && pair.second == "https://www.openstreetmap.org/?mlat=55.675589&mlon=12.569786#map=16/55.675589/12.569786") foundOsm = true;
    }
    assert(foundFilename);
    assert(foundSize);
    assert(foundOrientation);
    assert(foundLat);
    assert(foundLon);
    assert(foundOsm);
    qDebug() << "ExifData::toList() passed.";

    qDebug() << "All ExifReader tests passed!";
    return 0;
}
