#include <QApplication>
#include <QTimer>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    MainWindow w;
    w.showMaximized();

    const QStringList args = app.arguments();
    if (args.size() > 1) {
        const QString path = args.at(1);
        QTimer::singleShot(0, &w, [&w, path]() { w.openPath(path); });
    }

    return app.exec();
}
