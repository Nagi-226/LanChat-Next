// main.cpp —— Qt6 原生客户端入口。取代 Tauri 套壳。
// --grab <png>：offscreen 目检 hook（配合 QT_QPA_PLATFORM=offscreen），截图后退出。
#include <QApplication>
#include <QTimer>

#include "app/devmode.h"
#include "app/server_process.h"
#include "model/store.h"
#include "net/session.h"
#include "ui/main_window.h"
#include "ui/theme.h"

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    app.setApplicationName("LanChat-Qt");
    app.setOrganizationName("LanChat");
    app.setStyleSheet(theme::stylesheet());

    ServerProcess server;
    server.ensureRunning();

    DevMode dev;
    Session sess;
    Store store;
    MainWindow w(&sess, &store, &dev);
    w.show();
    sess.connectToServer("127.0.0.1", 12346);

    for (int i = 1; i < argc; ++i) {
        if (qstrcmp(argv[i], "--grab") == 0 && i + 1 < argc) {
            const QString path = QString::fromLocal8Bit(argv[i + 1]);
            QTimer::singleShot(600, &app, [&] {
                w.grab().save(path);
                app.quit();
            });
        }
    }

    const int rc = app.exec();
    server.stop();
    return rc;
}