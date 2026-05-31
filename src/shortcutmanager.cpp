#include "shortcutmanager.h"

#include <QCoreApplication>
#include <QKeyEvent>
#include <QSettings>

namespace {

struct ActionInfo {
    const char *name;        // settings key
    const char *description; // human-readable for help overlay / prefs table
    QKeySequence defaultKey;
    ShortcutManager::Category category;
};

// Order must match ShortcutManager::Action enum
static const ActionInfo kDefaults[ShortcutManager::ActionCount] = {
    {"ToggleHelp",      QT_TR_NOOP("Show/hide keyboard shortcuts"), QKeySequence(Qt::Key_F1),       ShortcutManager::CatNotConfigurable},
    {"CloseViewer",     QT_TR_NOOP("Close viewer"),                 QKeySequence(Qt::Key_Escape),   ShortcutManager::CatImageViewer},
    {"CloseViewerAlt",  QT_TR_NOOP("Close viewer (alt)"),           QKeySequence(Qt::Key_Return),   ShortcutManager::CatImageViewer},
    {"NextImage",       QT_TR_NOOP("Next image"),                   QKeySequence(Qt::Key_PageDown), ShortcutManager::CatImageViewer},
    {"PrevImage",       QT_TR_NOOP("Previous image"),               QKeySequence(Qt::Key_PageUp),   ShortcutManager::CatImageViewer},
    {"PanLeft",         QT_TR_NOOP("Pan left"),                     QKeySequence(Qt::Key_Left),     ShortcutManager::CatImageViewer},
    {"PanRight",        QT_TR_NOOP("Pan right"),                    QKeySequence(Qt::Key_Right),    ShortcutManager::CatImageViewer},
    {"PanUp",           QT_TR_NOOP("Pan up"),                       QKeySequence(Qt::Key_Up),       ShortcutManager::CatImageViewer},
    {"PanDown",         QT_TR_NOOP("Pan down"),                     QKeySequence(Qt::Key_Down),     ShortcutManager::CatImageViewer},
    {"ZoomOriginal",    QT_TR_NOOP("Zoom to original size"),        QKeySequence(Qt::Key_Slash),    ShortcutManager::CatImageViewer},
    {"ZoomFit",         QT_TR_NOOP("Zoom to fit screen"),           QKeySequence(Qt::Key_Asterisk), ShortcutManager::CatImageViewer},
    {"ZoomIn",          QT_TR_NOOP("Zoom in"),                      QKeySequence(Qt::Key_Plus),     ShortcutManager::CatImageViewer},
    {"ZoomOut",         QT_TR_NOOP("Zoom out"),                     QKeySequence(Qt::Key_Minus),    ShortcutManager::CatImageViewer},
    {"ToggleExif",      QT_TR_NOOP("Toggle EXIF info"),             QKeySequence(Qt::Key_I),        ShortcutManager::CatImageViewer},
    {"EditCaption",     QT_TR_NOOP("Edit caption"),                 QKeySequence(Qt::Key_E),        ShortcutManager::CatImageViewer},
    {"ToggleFullscreen",QT_TR_NOOP("Toggle fullscreen"),            QKeySequence(Qt::Key_F11),      ShortcutManager::CatImageViewer},
    {"ActivateEntry",   QT_TR_NOOP("Open folder / image"),          QKeySequence(Qt::Key_Return),   ShortcutManager::CatBrowser},
    {"PrevFolder",      QT_TR_NOOP("Previous folder"),              QKeySequence(Qt::CTRL | Qt::Key_Left),  ShortcutManager::CatBrowser},
    {"NextFolder",      QT_TR_NOOP("Next folder"),                  QKeySequence(Qt::CTRL | Qt::Key_Right), ShortcutManager::CatBrowser},
};

} // namespace

ShortcutManager::ShortcutManager()
{
    resetToDefaults();
}

ShortcutManager &ShortcutManager::instance()
{
    static ShortcutManager mgr;
    return mgr;
}

QKeySequence ShortcutManager::shortcut(Action action) const
{
    if (action < 0 || action >= ActionCount)
        return {};
    return m_bindings[action].key;
}

QString ShortcutManager::displayKey(Action action) const
{
    if (action < 0 || action >= ActionCount)
        return {};
    return m_bindings[action].key.toString(QKeySequence::NativeText);
}

QString ShortcutManager::description(Action action) const
{
    return actionDescription(action);
}

void ShortcutManager::setShortcut(Action action, const QKeySequence &seq)
{
    if (action >= 0 && action < ActionCount)
        m_bindings[action].key = seq;
}

void ShortcutManager::resetToDefaults()
{
    for (int i = 0; i < ActionCount; ++i)
        m_bindings[i].key = kDefaults[i].defaultKey;
}

void ShortcutManager::load(QSettings &settings)
{
    settings.beginGroup("shortcuts");
    for (int i = 0; i < ActionCount; ++i) {
        if (kDefaults[i].category == CatNotConfigurable) {
            m_bindings[i].key = kDefaults[i].defaultKey;
            continue;
        }
        const QString key = QString::fromLatin1(kDefaults[i].name);
        if (settings.contains(key))
            m_bindings[i].key = QKeySequence(settings.value(key).toString());
        else
            m_bindings[i].key = kDefaults[i].defaultKey;
    }
    settings.endGroup();
}

void ShortcutManager::save(QSettings &settings) const
{
    settings.beginGroup("shortcuts");
    for (int i = 0; i < ActionCount; ++i) {
        if (kDefaults[i].category == CatNotConfigurable)
            continue;
        const QString key = QString::fromLatin1(kDefaults[i].name);
        if (m_bindings[i].key == kDefaults[i].defaultKey)
            settings.remove(key); // only store non-defaults
        else
            settings.setValue(key, m_bindings[i].key.toString());
    }
    settings.endGroup();
}

QKeySequence ShortcutManager::defaultShortcut(Action action)
{
    if (action < 0 || action >= ActionCount)
        return {};
    return kDefaults[action].defaultKey;
}

QString ShortcutManager::actionName(Action action)
{
    if (action < 0 || action >= ActionCount)
        return {};
    return QString::fromLatin1(kDefaults[action].name);
}

QString ShortcutManager::actionDescription(Action action)
{
    if (action < 0 || action >= ActionCount)
        return {};
    return QCoreApplication::translate("ShortcutManager", kDefaults[action].description);
}

ShortcutManager::Category ShortcutManager::actionCategory(Action action)
{
    if (action < 0 || action >= ActionCount)
        return CatNotConfigurable;
    return kDefaults[action].category;
}

bool ShortcutManager::matches(const QKeyEvent *event, Action action) const
{
    if (action < 0 || action >= ActionCount)
        return false;
    const QKeySequence &seq = m_bindings[action].key;
    if (seq.isEmpty())
        return false;
    // Compare the first key in the sequence against the event's key + modifiers.
    int eventKey = event->key();
    Qt::KeyboardModifiers mods = event->modifiers() & ~Qt::KeypadModifier;
    if (mods != Qt::NoModifier)
        eventKey |= mods;
    return seq[0] == QKeyCombination::fromCombined(eventKey);
}
