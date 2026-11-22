#include "i18n.h"
#include "netzacwindow.h"
#include "platinum.h"
#include "preferences.h"

#include <QApplication>
#include <QCoreApplication>
#include <QFileDialog>
#include <QFileInfo>
#include <QWebEngineDownloadRequest>
#include <QWebEngineProfile>

int main(int argc, char *argv[])
{
    initI18n();

    // Prevent WebEngine context-sharing and Wayland/pixman maximize/resize freezes.
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    if (qEnvironmentVariable("WLR_RENDERER") == QLatin1String("pixman")) {
        if (!qEnvironmentVariableIsSet("QT_QUICK_BACKEND"))
            qputenv("QT_QUICK_BACKEND", "software");
        if (!qEnvironmentVariableIsSet("QTWEBENGINE_CHROMIUM_FLAGS"))
            qputenv("QTWEBENGINE_CHROMIUM_FLAGS", "--disable-gpu --disable-gpu-compositing");
    }

    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("Netzac Navigator"));
    app.setDesktopFileName(QStringLiteral("zacos9-netzac"));
    applyZacos9Theme(app);

    NetzacPreferences::load().apply(netzacProfile());

    QObject::connect(
        netzacProfile(),
        &QWebEngineProfile::downloadRequested,
        &app,
        [](QWebEngineDownloadRequest *download) {
            if (!download)
                return;
            const QString target = QFileDialog::getSaveFileName(
                QApplication::activeWindow(),
                ztr("Save File"),
                download->suggestedFileName());
            if (target.isEmpty()) {
                download->cancel();
                return;
            }
            const QFileInfo fi(target);
            download->setDownloadDirectory(fi.absolutePath());
            download->setDownloadFileName(fi.fileName());
            download->accept();
        });

    const QString initialUrl = (argc > 1) ? QString::fromLocal8Bit(argv[1]) : QString();
    NetzacWindow window(initialUrl);
    window.show();
    return app.exec();
}
