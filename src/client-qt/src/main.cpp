// main.cpp —— Qt6 原生客户端入口。取代 Tauri 套壳。
// --grab <png>：offscreen 目检 hook（配合 QT_QPA_PLATFORM=offscreen），截图后退出。
#include <QApplication>
#include <QTimer>
#include <QPixmap>
#include <QIcon>
#include <cstdio>

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
    // 运行时窗口/任务栏图标。exe 的文件图标由 resources/app.rc 嵌进 PE 资源，
    // 两者是两处独立来源：改了图标要同时换 .ico（PE）与 .png（这份）。
    app.setWindowIcon(QIcon(":/icons/lanchat-256.png"));
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
        if (qstrcmp(argv[i], "--demo") == 0) {
            w.loadDemoData(true);
        } else if (qstrcmp(argv[i], "--demo-board") == 0) {
            w.loadDemoData(true, true);
        } else if (qstrcmp(argv[i], "--demo-empty") == 0) {
            w.loadDemoData(false);
        } else if (qstrcmp(argv[i], "--width") == 0 && i + 1 < argc) {
            w.resize(QString::fromLocal8Bit(argv[i + 1]).toInt(), 800);
        } else if (qstrcmp(argv[i], "--grab") == 0 && i + 1 < argc) {
            const QString path = QString::fromLocal8Bit(argv[i + 1]);
            QTimer::singleShot(600, &app, [path, &w, &app] {
                const QPixmap pm = w.grab();
                const bool ok = pm.save(path);
                std::fprintf(stderr, "grab %dx%d ok=%d path=%s\n",
                             pm.width(), pm.height(), int(ok), qPrintable(path));
                app.quit();
            });
        }
    }

    const int rc = app.exec();
    server.stop();
    return rc;
}