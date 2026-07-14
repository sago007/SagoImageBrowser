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

#include "nativepath.h"

#include <QFile>

#ifdef _WIN32
#include <io.h>
#include <sys/stat.h>
#include <wchar.h>
#else
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace nativepath {

#ifdef _WIN32

namespace {

constexpr char32_t kReplacement    = 0xFFFD;
constexpr char32_t kHighSurrogateLo = 0xD800;
constexpr char32_t kHighSurrogateHi = 0xDBFF;
constexpr char32_t kLowSurrogateLo  = 0xDC00;
constexpr char32_t kLowSurrogateHi  = 0xDFFF;
constexpr char32_t kSupplementary   = 0x10000;

// Append a code point as (W)TF-8.  Surrogate code points (0xD800-0xDFFF) are
// encoded as 3 bytes rather than rejected — this is what makes it WTF-8.
void appendUtf8(QByteArray &out, char32_t cp)
{
    if (cp < 0x80) {
        out.append(static_cast<char>(cp));
    } else if (cp < 0x800) {
        out.append(static_cast<char>(0xC0 | (cp >> 6)));
        out.append(static_cast<char>(0x80 | (cp & 0x3F)));
    } else if (cp < kSupplementary) {
        out.append(static_cast<char>(0xE0 | (cp >> 12)));
        out.append(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.append(static_cast<char>(0x80 | (cp & 0x3F)));
    } else {
        out.append(static_cast<char>(0xF0 | (cp >> 18)));
        out.append(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
        out.append(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.append(static_cast<char>(0x80 | (cp & 0x3F)));
    }
}

} // namespace

std::wstring wideFromNative(const QByteArray &nativePath)
{
    std::wstring out;
    out.reserve(static_cast<size_t>(nativePath.size()));

    const unsigned char *p =
        reinterpret_cast<const unsigned char *>(nativePath.constData());
    const qsizetype n = nativePath.size();

    qsizetype i = 0;
    while (i < n) {
        const unsigned char b0 = p[i];
        char32_t   cp;
        qsizetype  len;
        if (b0 < 0x80)             { cp = b0;        len = 1; }
        else if ((b0 & 0xE0) == 0xC0) { cp = b0 & 0x1F; len = 2; }
        else if ((b0 & 0xF0) == 0xE0) { cp = b0 & 0x0F; len = 3; }
        else if ((b0 & 0xF8) == 0xF0) { cp = b0 & 0x07; len = 4; }
        else {
            out.push_back(static_cast<wchar_t>(kReplacement));
            ++i;
            continue;
        }

        if (i + len > n) {
            out.push_back(static_cast<wchar_t>(kReplacement));
            ++i;
            continue;
        }

        bool ok = true;
        for (qsizetype k = 1; k < len; ++k) {
            const unsigned char bk = p[i + k];
            if ((bk & 0xC0) != 0x80) { ok = false; break; }
            cp = (cp << 6) | (bk & 0x3F);
        }
        if (!ok) {
            out.push_back(static_cast<wchar_t>(kReplacement));
            ++i;
            continue;
        }

        i += len;
        if (cp < kSupplementary) {
            out.push_back(static_cast<wchar_t>(cp)); // BMP incl. lone surrogates
        } else {
            cp -= kSupplementary;
            out.push_back(static_cast<wchar_t>(kHighSurrogateLo + (cp >> 10)));
            out.push_back(static_cast<wchar_t>(kLowSurrogateLo + (cp & 0x3FF)));
        }
    }
    return out;
}

QByteArray nativeFromWide(const std::wstring &wide)
{
    QByteArray out;
    out.reserve(static_cast<qsizetype>(wide.size()) * 3);

    const size_t n = wide.size();
    for (size_t i = 0; i < n; ++i) {
        char32_t cp = static_cast<char16_t>(wide[i]);
        if (cp >= kHighSurrogateLo && cp <= kHighSurrogateHi && i + 1 < n) {
            const char16_t next = static_cast<char16_t>(wide[i + 1]);
            if (next >= kLowSurrogateLo && next <= kLowSurrogateHi) {
                cp = kSupplementary
                     + ((cp - kHighSurrogateLo) << 10)
                     + (next - kLowSurrogateLo);
                ++i;
            }
        }
        appendUtf8(out, cp);
    }
    return out;
}

std::filesystem::path pathFromNative(const QByteArray &nativePath)
{
    return std::filesystem::path(wideFromNative(nativePath));
}

QByteArray nativeFromPath(const std::filesystem::path &path)
{
    return nativeFromWide(path.native());
}

QString displayFromNative(const QByteArray &nativePath)
{
    const std::wstring w = wideFromNative(nativePath);
    return QString::fromWCharArray(w.data(), static_cast<qsizetype>(w.size()));
}

QByteArray nativeFromDisplay(const QString &text)
{
    return nativeFromWide(text.toStdWString());
}

bool openNativeRead(const QByteArray &nativePath, QFile &outFile)
{
    const std::wstring w = wideFromNative(nativePath);
    outFile.setFileName(
        QString::fromWCharArray(w.data(), static_cast<qsizetype>(w.size())));
    return outFile.open(QIODevice::ReadOnly);
}

NativeStat nativeStat(const QByteArray &nativePath)
{
    NativeStat info;
    struct _stat64 st{};
    if (_wstat64(wideFromNative(nativePath).c_str(), &st) != 0)
        return info;
    info.exists    = true;
    info.isRegular = (st.st_mode & _S_IFREG) != 0;
    info.isDir     = (st.st_mode & _S_IFDIR) != 0;
    info.size      = static_cast<qint64>(st.st_size);
    info.mtime     = static_cast<qint64>(st.st_mtime);
    return info;
}

bool isWritable(const QByteArray &nativePath)
{
    constexpr int kWriteMode = 2; // _waccess: 02 == write permission
    return _waccess(wideFromNative(nativePath).c_str(), kWriteMode) == 0;
}

#else // POSIX

std::filesystem::path pathFromNative(const QByteArray &nativePath)
{
    return std::filesystem::path(nativePath.toStdString());
}

QByteArray nativeFromPath(const std::filesystem::path &path)
{
    return QByteArray::fromStdString(path.native());
}

QString displayFromNative(const QByteArray &nativePath)
{
    return QString::fromLocal8Bit(nativePath.constData(), nativePath.size());
}

QByteArray nativeFromDisplay(const QString &text)
{
    return QFile::encodeName(text);
}

bool openNativeRead(const QByteArray &nativePath, QFile &outFile)
{
    const int fd = ::open(nativePath.constData(), O_RDONLY | O_CLOEXEC);
    if (fd < 0)
        return false;
    if (!outFile.open(fd, QIODevice::ReadOnly, QFileDevice::AutoCloseHandle)) {
        ::close(fd);
        return false;
    }
    return true;
}

NativeStat nativeStat(const QByteArray &nativePath)
{
    NativeStat info;
    struct ::stat st{};
    if (::stat(nativePath.constData(), &st) != 0)
        return info;
    info.exists    = true;
    info.isRegular = S_ISREG(st.st_mode);
    info.isDir     = S_ISDIR(st.st_mode);
    info.size      = static_cast<qint64>(st.st_size);
    info.mtime     = static_cast<qint64>(st.st_mtime);
    return info;
}

bool isWritable(const QByteArray &nativePath)
{
    return ::access(nativePath.constData(), W_OK) == 0;
}

#endif

} // namespace nativepath
