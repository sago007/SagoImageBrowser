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

#include <QKeySequence>
#include <QString>
#include <array>

class QSettings;
class QKeyEvent;

class ShortcutManager
{
public:
    enum Action {
        ToggleHelp,
        CloseViewer,
        CloseViewerAlt,
        NextImage,
        PrevImage,
        PanLeft,
        PanRight,
        PanUp,
        PanDown,
        ZoomOriginal,
        ZoomFit,
        ZoomIn,
        ZoomOut,
        ToggleExif,
        EditCaption,
        ToggleFullscreen,
        ActivateEntry,
        PrevFolder,
        NextFolder,
        MoveToTrash,
        ActionCount
    };

    enum Category {
        CatBrowser,
        CatImageViewer,
        CatNotConfigurable
    };

    static ShortcutManager &instance();

    QKeySequence shortcut(Action action) const;
    QString displayKey(Action action) const;
    QString description(Action action) const;

    void setShortcut(Action action, const QKeySequence &seq);
    void resetToDefaults();

    void load(QSettings &settings);
    void save(QSettings &settings) const;

    static QKeySequence defaultShortcut(Action action);
    static QString actionName(Action action);
    static QString actionDescription(Action action);
    static Category actionCategory(Action action);

    /// Returns true if the key event matches the configured shortcut for the action.
    bool matches(const QKeyEvent *event, Action action) const;

private:
    ShortcutManager();

    struct Binding {
        QKeySequence key;
    };

    std::array<Binding, ActionCount> m_bindings;
};
