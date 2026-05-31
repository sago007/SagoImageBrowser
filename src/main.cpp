#include <QApplication>
#include <QLocale>
#include <QTimer>
#include <QTranslator>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QTranslator translator;
    const QString locale = QLocale::system().name();   // e.g. "da_DK"
    if (translator.load(":/translations/" + locale.left(2)))
        app.installTranslator(&translator);

    MainWindow w;
    w.showMaximized();

    const QStringList args = app.arguments();
    if (args.size() > 1) {
        const QString path = args.at(1);
        QTimer::singleShot(0, &w, [&w, path]() { w.openPath(path); });
    }

    return app.exec();
}
