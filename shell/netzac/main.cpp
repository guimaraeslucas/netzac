#include "dialogs.h"
#include "i18n.h"
#include "netzacwindow.h"
#include "platinum.h"
#include "preferences.h"

#include <QApplication>
#include <QByteArray>
#include <QByteArrayList>
#include <QCoreApplication>
#include <QFileInfo>
#include <QIcon>
#include <QStringList>
#include <QUrl>
#include <QWebEngineDownloadRequest>
#include <QWebEngineProfile>

namespace {

bool hasCliPlatformArg(int argc, char *argv[])
{
    for (int i = 1; i < argc; ++i) {
        const QByteArray arg(argv[i]);
        if (arg == "-platform" || arg == "--platform" || arg.startsWith("-platform="))
            return true;
    }
    return false;
}

bool hasNvidiaProprietaryDriver()
{
#if defined(Q_OS_LINUX)
    return QFileInfo::exists(QStringLiteral("/proc/driver/nvidia/version"))
        || QFileInfo::exists(QStringLiteral("/sys/module/nvidia"));
#else
    return false;
#endif
}

void appendChromiumFlagsIfMissing(const QByteArrayList &flags)
{
    QByteArray current = qgetenv("QTWEBENGINE_CHROMIUM_FLAGS").trimmed();
    for (const QByteArray &flag : flags) {
        if (!current.contains(flag)) {
            if (!current.isEmpty())
                current.append(' ');
            current.append(flag);
        }
    }
    qputenv("QTWEBENGINE_CHROMIUM_FLAGS", current);
}

}  // namespace

int main(int argc, char *argv[])
{
    initI18n();

    // Prevent WebEngine context-sharing and Wayland/pixman maximize/resize freezes.
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);

    const bool isPixman = (qEnvironmentVariable("WLR_RENDERER") == QLatin1String("pixman"));
    const bool disableGpu = NetzacPreferences::isGpuAccelerationDisabled();

    // Under Wayland with the proprietary NVIDIA driver (GL/EGL path), embedding
    // QWebEngineView promotes QMainWindow to QOpenGLCompositorBackingStore /
    // QWaylandEglWindow, which hits egl-wayland/wlroots resize stride shearing.
    // Prefer XWayland (xcb) when QT_QPA_PLATFORM permits it (e.g. "wayland;xcb")
    // so full GPU web rendering stays active without resize corruption.
    if (!isPixman && !disableGpu && !hasCliPlatformArg(argc, argv)
        && !qEnvironmentVariable("WAYLAND_DISPLAY").isEmpty()
        && !qEnvironmentVariable("DISPLAY").isEmpty()
        && hasNvidiaProprietaryDriver()) {
        const QString qpa = qEnvironmentVariable("QT_QPA_PLATFORM").trimmed().toLower();
        if (qpa.isEmpty() || qpa.contains(QLatin1String("xcb")))
            qputenv("QT_QPA_PLATFORM", "xcb");
    }

    if (isPixman || disableGpu) {
        if (!qEnvironmentVariableIsSet("QT_QUICK_BACKEND"))
            qputenv("QT_QUICK_BACKEND", "software");
        if (disableGpu) {
            appendChromiumFlagsIfMissing({"--disable-gpu", "--disable-gpu-compositing"});
        } else if (!qEnvironmentVariableIsSet("QTWEBENGINE_CHROMIUM_FLAGS")) {
            qputenv("QTWEBENGINE_CHROMIUM_FLAGS", "--disable-gpu --disable-gpu-compositing");
        }
    }

    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("Netzac Navigator"));
    app.setDesktopFileName(QStringLiteral("netzac"));
    installI18nTranslations(app);
#if defined(Q_OS_WIN)
    // Windows has no icon theme: prefer the icon embedded in the executable,
    // then the files shipped next to it.
    const QString iconDir = QCoreApplication::applicationDirPath();
    const QStringList iconCandidates = {
        QStringLiteral(":/netzac/netzac.ico"),
        iconDir + QStringLiteral("/netzac.ico"),
        iconDir + QStringLiteral("/netzac.svg"),
    };
    for (const QString &candidate : iconCandidates) {
        const QIcon icon(candidate);
        if (!icon.isNull()) {
            app.setWindowIcon(icon);
            break;
        }
    }
#endif
    applyZacos9Theme(app);

    NetzacPreferences::ensureDesktopEntryRegistered();
    NetzacPreferences::load().apply(netzacProfile());

    QObject::connect(
        netzacProfile(),
        &QWebEngineProfile::downloadRequested,
        &app,
        [](QWebEngineDownloadRequest *download) {
            if (!download)
                return;
            auto *dlg = new DownloadDialog(download);
            dlg->show();
            dlg->raise();
            dlg->activateWindow();
        });

    QString initialUrl;
    for (int i = 1; i < argc; ++i) {
        const QString arg = QString::fromLocal8Bit(argv[i]).trimmed();
        if (arg.isEmpty() || arg.startsWith(QLatin1Char('-')))
            continue;
        const QFileInfo fi(arg);
        if (fi.exists() && fi.isFile())
            initialUrl = QUrl::fromLocalFile(fi.absoluteFilePath()).toString();
        else
            initialUrl = arg;
        break;
    }

    NetzacWindow window(initialUrl);
    window.show();
    return app.exec();
}
