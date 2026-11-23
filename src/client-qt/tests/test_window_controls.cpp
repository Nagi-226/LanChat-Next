#include "ui/main_window.h"
#include "ui/theme.h"
#include "app/devmode.h"
#include "net/session.h"
#include "model/store.h"
#include <QApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFontDatabase>
#include <QPushButton>
#include <QSettings>
#include <QTemporaryDir>
#include <QTest>
#include <QFile>
#include <QEnterEvent>
#include <functional>
#ifdef Q_OS_WIN
#include <qt_windows.h>
#include <windowsx.h>
#endif

#undef assert
#define assert(condition) do { if(!(condition)){const auto failure=QString("FAIL line %1: %2\n").arg(__LINE__).arg(#condition).toUtf8();QFile output(qEnvironmentVariable("LANCHAT_WINDOW_TEST_LOG",QCoreApplication::applicationDirPath()+"/window-controls-failure.log"));if(output.open(QIODevice::WriteOnly|QIODevice::Append))output.write(failure);std::fprintf(stderr,"%s",failure.constData());return 1;} } while(false)

static bool waitFor(const std::function<bool()>& condition){QElapsedTimer timer;timer.start();while(timer.elapsed()<3000){QCoreApplication::processEvents();if(condition())return true;QTest::qWait(10);}return false;}
int main(int argc,char** argv){
    QApplication app(argc,argv);QTemporaryDir temporary;assert(temporary.isValid());
    app.setOrganizationName("LanChatWindowTest");app.setApplicationName("IsolatedWindowControls");
    QSettings::setDefaultFormat(QSettings::IniFormat);QSettings::setPath(QSettings::IniFormat,QSettings::UserScope,temporary.path());
    QFontDatabase::addApplicationFont("C:/Windows/Fonts/msyh.ttc");app.setStyleSheet(theme::stylesheet());
    Session session;Store store;DevMode dev;MainWindow window(&session,&store,&dev);window.resize(1000,700);window.show();
    auto* minimize=window.findChild<QPushButton*>("windowMinimizeButton");auto* maximize=window.findChild<QPushButton*>("windowMaximizeButton");auto* close=window.findChild<QPushButton*>("windowCloseButton");auto* title=window.findChild<QWidget*>("windowTitleBar");
    assert(minimize&&maximize&&close&&title);assert(waitFor([&]{return window.isVisible();}));
    for(auto* button:{minimize,maximize,close}){assert(button->size()==QSize(46,40));assert(button->iconSize()==QSize(12,12));assert(!button->icon().isNull());assert(button->text().isEmpty());assert(!button->accessibleName().isEmpty());}
    window.raise();window.activateWindow();QTest::qWait(100);
    auto movePointer=[&](QWidget* target,const QPoint& point){
#ifdef Q_OS_WIN
        if(QGuiApplication::platformName()=="windows"){
            // Test Qt hover dispatch/painting in a real window without requiring an input desktop.
            QEvent leave(QEvent::Leave);QApplication::sendEvent(close,&leave);
            QEnterEvent enter(point,target->mapTo(&window,point),target->mapToGlobal(point));QApplication::sendEvent(target,&enter);QTest::qWait(100);return;
        }
#endif
        QTest::mouseMove(target,point);QTest::qWait(100);
    };
    movePointer(close,close->rect().center());
    const auto hover=close->grab().toImage();const auto hoverColor=hover.pixelColor(qRound(10*hover.devicePixelRatio()),qRound(10*hover.devicePixelRatio()));
    std::fprintf(stderr,"close hover: %s alpha=%d, underMouse=%d nativeHover=%d visible=%d size=%dx%d windowActive=%d\n",qPrintable(hoverColor.name()),hoverColor.alpha(),close->underMouse(),close->property("nativeHover").toBool(),close->isVisible(),close->width(),close->height(),window.isActiveWindow());
    if(QGuiApplication::platformName()=="windows")window.grab().save(QCoreApplication::applicationDirPath()+"/native-hover-window.png");
    assert(hoverColor==QColor("#C42B1C"));
    movePointer(title,QPoint(200,20));
    const auto idle=close->grab().toImage();assert(idle.pixelColor(qRound(10*idle.devicePixelRatio()),qRound(10*idle.devicePixelRatio()))!=hoverColor);
    const auto normalIcon=maximize->icon().pixmap(QSize(24,24)).toImage();
    QTest::mouseClick(maximize,Qt::LeftButton);assert(waitFor([&]{return window.isMaximized();}));assert(maximize->property("captionAction").toString()=="restore");assert(maximize->icon().pixmap(QSize(24,24)).toImage()!=normalIcon);
    const auto args=app.arguments();QString captures;if(args.contains("--capture-dir")){captures=args.value(args.indexOf("--capture-dir")+1);QDir().mkpath(captures);assert(title->grab().save(captures+"/maximized-titlebar.png"));assert(window.findChild<QWidget*>("windowCaptionButtons")->grab().save(captures+"/maximized-buttons.png"));}
    QTest::mouseClick(maximize,Qt::LeftButton);assert(waitFor([&]{return !window.isMaximized();}));assert(maximize->property("captionAction").toString()=="maximize");
    QTest::mouseDClick(title,Qt::LeftButton,Qt::NoModifier,QPoint(200,20));assert(waitFor([&]{return window.isMaximized();}));QTest::mouseDClick(title,Qt::LeftButton,Qt::NoModifier,QPoint(200,20));assert(waitFor([&]{return !window.isMaximized();}));
    dev.setEnabled(true);QCoreApplication::processEvents();assert(maximize->size()==minimize->size()&&close->size()==minimize->size());dev.setEnabled(false);QCoreApplication::processEvents();
#ifdef Q_OS_WIN
    if(QGuiApplication::platformName()=="windows"){
        const auto hwnd=reinterpret_cast<HWND>(window.winId());const auto style=GetWindowLongPtrW(hwnd,GWL_STYLE);
        assert((style&(WS_CAPTION|WS_THICKFRAME|WS_SYSMENU|WS_MINIMIZEBOX|WS_MAXIMIZEBOX))==(WS_CAPTION|WS_THICKFRAME|WS_SYSMENU|WS_MINIMIZEBOX|WS_MAXIMIZEBOX));
        auto screenPoint=[&](const QPoint& point){POINT native{qRound(point.x()*window.devicePixelRatioF()),qRound(point.y()*window.devicePixelRatioF())};ClientToScreen(hwnd,&native);return MAKELPARAM(native.x,native.y);};
        auto hit=[&](QWidget* widget,const QPoint& point){return SendMessageW(hwnd,WM_NCHITTEST,0,screenPoint(widget->mapTo(&window,point)));};
        assert(hit(title,QPoint(200,20))==HTCAPTION);assert(hit(minimize,minimize->rect().center())==HTCLIENT);assert(hit(maximize,maximize->rect().center())==HTMAXBUTTON);assert(hit(close,close->rect().center())==HTCLIENT);
        assert(!window.findChild<QPushButton*>("boardConnectionButton")&&!window.findChild<QPushButton*>("serverSettingsButton"));
        RECT normal{};GetWindowRect(hwnd,&normal);assert(SendMessageW(hwnd,WM_NCHITTEST,0,MAKELPARAM(normal.left+2,normal.top+2))==HTTOPLEFT);assert(SendMessageW(hwnd,WM_NCHITTEST,0,MAKELPARAM(normal.right-2,(normal.top+normal.bottom)/2))==HTRIGHT);
        SendMessageW(hwnd,WM_SYSCOMMAND,SC_MAXIMIZE,0);assert(waitFor([&]{return window.isMaximized()&&IsZoomed(hwnd);}));assert(maximize->property("captionAction").toString()=="restore");
        MONITORINFO monitor{sizeof(MONITORINFO)};assert(GetMonitorInfoW(MonitorFromWindow(hwnd,MONITOR_DEFAULTTONEAREST),&monitor));RECT client{};GetClientRect(hwnd,&client);POINT origin{0,0};ClientToScreen(hwnd,&origin);
        assert(origin.x==monitor.rcWork.left&&origin.y==monitor.rcWork.top);assert(client.right==monitor.rcWork.right-monitor.rcWork.left&&client.bottom==monitor.rcWork.bottom-monitor.rcWork.top);
        assert(hit(title,QPoint(200,2))==HTCAPTION);SendMessageW(hwnd,WM_SYSCOMMAND,SC_RESTORE,0);assert(waitFor([&]{return !window.isMaximized();}));RECT restored{};GetWindowRect(hwnd,&restored);assert(EqualRect(&normal,&restored));
        // 非客户区最大化按钮必须仍可点击，拖出按钮释放不能意外执行。
        const auto maxPoint=screenPoint(maximize->mapTo(&window,maximize->rect().center()));
        SendMessageW(hwnd,WM_NCLBUTTONDOWN,HTMAXBUTTON,maxPoint);assert(GetCapture()==hwnd);SendMessageW(hwnd,WM_LBUTTONUP,0,MAKELPARAM(200,200));assert(!window.isMaximized()&&GetCapture()!=hwnd);
        SendMessageW(hwnd,WM_NCLBUTTONDOWN,HTMAXBUTTON,maxPoint);SendMessageW(hwnd,WM_NCLBUTTONUP,HTMAXBUTTON,maxPoint);assert(waitFor([&]{return window.isMaximized();}));SendMessageW(hwnd,WM_SYSCOMMAND,SC_RESTORE,0);assert(waitFor([&]{return !window.isMaximized();}));
        SendMessageW(hwnd,WM_NCLBUTTONDBLCLK,HTCAPTION,screenPoint(QPoint(200,20)));assert(waitFor([&]{return window.isMaximized();}));SendMessageW(hwnd,WM_SYSCOMMAND,SC_RESTORE,0);assert(waitFor([&]{return !window.isMaximized();}));
        std::puts("PASS native Windows frame: caption/resize/button hit tests, native maximize/restore and icon state, work area bounds, button capture/cancel, nonclient double click");
    }
#endif
    QTest::mouseClick(minimize,Qt::LeftButton);assert(waitFor([&]{return window.isMinimized();}));window.showNormal();assert(waitFor([&]{return !window.isMinimized();}));
    if(!captures.isEmpty()){assert(title->grab().save(captures+"/normal-titlebar.png"));assert(window.findChild<QWidget*>("windowCaptionButtons")->grab().save(captures+"/normal-buttons.png"));}
    if(args.contains("--interactive")){window.loadDemoData();window.setWindowTitle("LanChat 窗口行为验收（隔离测试）");return app.exec();}
    QTest::mouseClick(close,Qt::LeftButton);assert(!window.isVisible());std::puts("PASS window controls: equal hit/icon sizes, maximize/restore/double click, minimize/close, developer toolbar preserved");return 0;
}
