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
