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

#pragma once

#include <QByteArray>
#include <QString>

#include <filesystem>
#include <string>

class QFile;

// Central conversion layer between the application's canonical path handle
// (a QByteArray of "native OS bytes") and the platform's filesystem APIs.
//
// The rest of the code stores every path as a QByteArray and never inspects
// its bytes.  What those bytes *mean* is platform-defined and lives only here:
//   * POSIX  : the raw bytes returned by the OS (may be invalid UTF-8, e.g.
//              Latin-1 names) — passed through unchanged.
//   * Windows: WTF-8 — the UTF-8 generalisation that also encodes unpaired
//              UTF-16 surrogates, so ill-formed-but-valid Windows filenames
//              round-trip losslessly, matching the POSIX "never mangle a name"
//              guarantee.
//
// This is the only translation unit that contains platform #ifdefs.
namespace nativepath {

// QByteArray native path <-> std::filesystem::path (used for enumeration and
// path decomposition such as filename()/parent_path()/extension()).
std::filesystem::path pathFromNative(const QByteArray &nativePath);
QByteArray            nativeFromPath(const std::filesystem::path &path);

// Native byte path -> lossy display string for the UI.
QString displayFromNative(const QByteArray &nativePath);

// Well-formed display/UI string (dialogs, typed input) -> native byte path.
QByteArray nativeFromDisplay(const QString &text);

// Open a file read-only and adopt it into outFile.  Returns true on success.
// Bypasses Qt's name-based path handling on POSIX (so non-UTF-8 names work);
// on Windows Qt already opens via the wide API, which is lossless.
bool openNativeRead(const QByteArray &nativePath, QFile &outFile);

struct NativeStat {
    bool   exists    = false;
    bool   isRegular = false;
    bool   isDir     = false;
    qint64 size      = 0;
    qint64 mtime     = 0;
};
NativeStat nativeStat(const QByteArray &nativePath);

// True if the file exists and is writable by the current process.
bool isWritable(const QByteArray &nativePath);

#ifdef _WIN32
// WTF-8 codec.  Exposed for the exiv2 wide-path overload and for unit tests.
std::wstring wideFromNative(const QByteArray &nativePath);
QByteArray   nativeFromWide(const std::wstring &wide);
#endif

} // namespace nativepath
