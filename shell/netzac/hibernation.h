#pragma once

#include <QObject>
#include <QPointer>
#include <QTimer>
#include <QVector>

class QWebEngineDownloadRequest;
class QWebEnginePage;

class HibernationManager : public QObject {
    Q_OBJECT

public:
    static constexpr int kFreezeDelayMs = 5 * 60 * 1000;
    static constexpr int kDiscardDelayMs = 2 * 60 * 60 * 1000;
    static constexpr double kLowMemoryThreshold = 0.15;
    static constexpr int kMemoryPollIntervalMs = 15 * 1000;

    static HibernationManager *instance();

    static bool isSystemEnergySaverActive();
    static double availableMemoryRatio();
    static bool isLowMemoryCondition(double threshold = kLowMemoryThreshold);

    void registerDownload(QWebEngineDownloadRequest *download);
    bool hasOngoingDownloadForPage(const QWebEnginePage *page) const;

signals:
    void lowMemoryDetected();

private:
    explicit HibernationManager(QObject *parent = nullptr);
    void pruneFinishedDownloads() const;

    struct TrackedDownload {
        QPointer<QWebEngineDownloadRequest> req;
        QPointer<QWebEnginePage> page;
    };

    mutable QVector<TrackedDownload> m_activeDownloads;
    QTimer m_memoryPollTimer;
};
