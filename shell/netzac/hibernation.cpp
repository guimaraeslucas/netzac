#include "hibernation.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QWebEngineDownloadRequest>
#include <QWebEnginePage>

#if defined(Q_OS_WIN)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace {

bool isDownloadActive(const QWebEngineDownloadRequest *req)
{
    if (!req || req->isFinished())
        return false;
    const auto st = req->state();
    return st == QWebEngineDownloadRequest::DownloadRequested
        || st == QWebEngineDownloadRequest::DownloadInProgress;
}

}  // namespace

HibernationManager *HibernationManager::instance()
{
    static QPointer<HibernationManager> s_manager;
    if (!s_manager)
        s_manager = new HibernationManager(QCoreApplication::instance());
    return s_manager.data();
}

HibernationManager::HibernationManager(QObject *parent)
    : QObject(parent)
{
    m_memoryPollTimer.setInterval(kMemoryPollIntervalMs);
    connect(&m_memoryPollTimer, &QTimer::timeout, this, [this]() {
        if (isLowMemoryCondition())
            emit lowMemoryDetected();
    });
    m_memoryPollTimer.start();
}

bool HibernationManager::isSystemEnergySaverActive()
{
#if defined(Q_OS_WIN)
    SYSTEM_POWER_STATUS sps{};
    if (GetSystemPowerStatus(&sps))
        return sps.SystemStatusFlag == 1;
    return false;
#else
    QFile profileFile(QStringLiteral("/sys/firmware/acpi/platform_profile"));
    if (profileFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        const QString profile = QString::fromUtf8(profileFile.readAll()).trimmed().toLower();
        if (profile == QLatin1String("low-power"))
            return true;
    }

    QString configRoot = qEnvironmentVariable("XDG_CONFIG_HOME");
    if (configRoot.isEmpty())
        configRoot = QDir::homePath() + QStringLiteral("/.config");
    QFile desktopConf(configRoot + QStringLiteral("/zacos9/desktop.conf"));
    if (desktopConf.open(QIODevice::ReadOnly | QIODevice::Text)) {
        while (!desktopConf.atEnd()) {
            const QString line = QString::fromUtf8(desktopConf.readLine()).trimmed();
            const int eq = line.indexOf(QLatin1Char('='));
            if (eq <= 0)
                continue;
            const QString key = line.left(eq).trimmed().toLower();
            const QString val = line.mid(eq + 1).trimmed().toLower();
            if ((key == QLatin1String("energy-saver") || key == QLatin1String("power-saver"))
                && (val == QLatin1String("1") || val == QLatin1String("true"))) {
                return true;
            }
            if (key == QLatin1String("power-profile")
                && (val == QLatin1String("power-saver") || val == QLatin1String("low-power"))) {
                return true;
            }
        }
    }

    const QDir supplyDir(QStringLiteral("/sys/class/power_supply"));
    if (supplyDir.exists()) {
        const QStringList entries = supplyDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString &entry : entries) {
            const QString base = supplyDir.filePath(entry);
            QFile typeFile(base + QStringLiteral("/type"));
            if (!typeFile.open(QIODevice::ReadOnly | QIODevice::Text))
                continue;
            if (QString::fromUtf8(typeFile.readAll()).trimmed().compare(
                    QLatin1String("Battery"), Qt::CaseInsensitive) != 0) {
                continue;
            }

            QFile statusFile(base + QStringLiteral("/status"));
            if (!statusFile.open(QIODevice::ReadOnly | QIODevice::Text))
                continue;
            const QString status = QString::fromUtf8(statusFile.readAll()).trimmed();
            if (status.compare(QLatin1String("Discharging"), Qt::CaseInsensitive) != 0)
                continue;

            QFile capFile(base + QStringLiteral("/capacity"));
            if (capFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                bool ok = false;
                const int cap = QString::fromUtf8(capFile.readAll()).trimmed().toInt(&ok);
                if (ok && cap <= 20)
                    return true;
            }
        }
    }

    return false;
#endif
}

double HibernationManager::availableMemoryRatio()
{
#if defined(Q_OS_WIN)
    MEMORYSTATUSEX mem{};
    mem.dwLength = sizeof(mem);
    if (GlobalMemoryStatusEx(&mem) && mem.ullTotalPhys > 0)
        return qBound(0.0, double(mem.ullAvailPhys) / double(mem.ullTotalPhys), 1.0);
    return 1.0;
#else
    QFile f(QStringLiteral("/proc/meminfo"));
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return 1.0;

    qint64 memTotal = 0;
    qint64 memAvailable = -1;
    qint64 memFree = 0;
    qint64 buffers = 0;
    qint64 cached = 0;

    static const QRegularExpression re(QStringLiteral("^(\\w+):\\s+(\\d+)"));
    while (!f.atEnd()) {
        const QString line = QString::fromUtf8(f.readLine());
        const QRegularExpressionMatch m = re.match(line);
        if (!m.hasMatch())
            continue;
        const QString key = m.captured(1);
        const qint64 kb = m.captured(2).toLongLong();
        if (key == QLatin1String("MemTotal"))
            memTotal = kb;
        else if (key == QLatin1String("MemAvailable"))
            memAvailable = kb;
        else if (key == QLatin1String("MemFree"))
            memFree = kb;
        else if (key == QLatin1String("Buffers"))
            buffers = kb;
        else if (key == QLatin1String("Cached"))
            cached = kb;
    }

    if (memTotal <= 0)
        return 1.0;
    const qint64 avail = (memAvailable >= 0) ? memAvailable : (memFree + buffers + cached);
    return qBound(0.0, double(avail) / double(memTotal), 1.0);
#endif
}

bool HibernationManager::isLowMemoryCondition(double threshold)
{
    return availableMemoryRatio() < threshold;
}

void HibernationManager::registerDownload(QWebEngineDownloadRequest *download)
{
    if (!download)
        return;

    pruneFinishedDownloads();
    for (const TrackedDownload &existing : m_activeDownloads) {
        if (existing.req == download)
            return;
    }

    TrackedDownload entry;
    entry.req = download;
    entry.page = download->page();
    m_activeDownloads.append(entry);

    connect(download, &QWebEngineDownloadRequest::isFinishedChanged, this, [this]() {
        pruneFinishedDownloads();
    });
    connect(download, &QWebEngineDownloadRequest::stateChanged, this, [this]() {
        pruneFinishedDownloads();
    });
    connect(download, &QObject::destroyed, this, [this]() {
        pruneFinishedDownloads();
    });
}

bool HibernationManager::hasOngoingDownloadForPage(const QWebEnginePage *page) const
{
    pruneFinishedDownloads();
    if (!page)
        return !m_activeDownloads.isEmpty();

    for (const TrackedDownload &entry : m_activeDownloads) {
        if (!entry.req || !isDownloadActive(entry.req))
            continue;
        if (entry.page == page || entry.page.isNull())
            return true;
    }
    return false;
}

void HibernationManager::pruneFinishedDownloads() const
{
    for (int i = int(m_activeDownloads.size()) - 1; i >= 0; --i) {
        if (!isDownloadActive(m_activeDownloads.at(i).req))
            m_activeDownloads.removeAt(i);
    }
}
