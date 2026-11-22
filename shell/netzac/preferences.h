#pragma once

#include "calmmode.h"

#include <QString>
#include <QVector>

class QWebEngineProfile;

struct NetzacPreferences {
    enum class CookiePolicy {
        AllowAll = 0,
        BlockThirdParty = 1,
        BlockAll = 2,
    };

    enum class CacheMode {
        Disk = 0,
        Memory = 1,
        Disabled = 2,
    };

    enum class ProxyMode {
        Direct = 0,
        System = 1,
        Manual = 2,
    };

    enum class ProxyType {
        Http = 0,
        Socks5 = 1,
    };

    enum class StartupMode {
        HomePage = 0,
        RestoreLastPage = 1,
    };

    struct PresetEntry {
        const char *id;
        QString label;
        const char *value;
    };

    struct PersonalToolbarSubItem {
        QString title;
        QString url;
    };

    struct PersonalToolbarItem {
        QString title;
        QString url;
        QString icon;
        bool isFolder = false;
        QVector<PersonalToolbarSubItem> children;
    };

    QString homeUrl;
    StartupMode startupMode = StartupMode::HomePage;
    QString lastUrl;
    QString searchEngineId;
    bool autoloadImages = true;
    CalmConfig calm;

    QVector<PersonalToolbarItem> personalToolbar;
    QString mailUrl;
    QString chatUrl;
    QString gitUrl;

    CookiePolicy cookiePolicy = CookiePolicy::AllowAll;
    bool sessionCookiesOnly = false;

    CacheMode cacheMode = CacheMode::Disk;
    int cacheMaxMb = 50;

    ProxyMode proxyMode = ProxyMode::System;
    ProxyType proxyType = ProxyType::Http;
    QString proxyHost;
    int proxyPort = 8080;

    QString userAgentPreset;
    QString userAgent;

    static NetzacPreferences defaults();
    static NetzacPreferences load();
    void save() const;
    static void saveLastUrl(const QString &url);
    void apply(QWebEngineProfile *profile) const;
    QString searchUrlFor(const QString &query) const;

    static QVector<PresetEntry> searchEnginePresets();
    static QVector<PresetEntry> userAgentPresets();
    static QVector<PersonalToolbarItem> defaultPersonalToolbar();
    static QString defaultChromeUserAgent();
    static QString userAgentForPreset(const QString &presetId);
};

QWebEngineProfile *netzacProfile();
