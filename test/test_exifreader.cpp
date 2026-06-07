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
    auto list = data.toList();
    bool foundFilename = false;
    bool foundSize = false;
    for (const auto &pair : list) {
        if (pair.first == "Filename" && pair.second == "test.jpg") foundFilename = true;
        if (pair.first == "File Size" && pair.second == "1.2 MiB") foundSize = true;
    }
    assert(foundFilename);
    assert(foundSize);
    qDebug() << "ExifData::toList() passed.";

    qDebug() << "All ExifReader tests passed!";
    return 0;
}
