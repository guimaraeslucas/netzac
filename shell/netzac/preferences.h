#pragma once

#include "calmmode.h"

#include <QMap>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QVector>

class QWebEngineProfile;
class QWebEngineSettings;

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

    enum class LinkOpenMode {
        Tabs = 0,
        Windows = 1,
    };

    enum class TabBarStyle {
        IE45Left = 0,
        ChromeTop = 1,
    };

    enum class ProportionalFontStyle {
        SansSerif = 0,
        Serif = 1,
    };

    enum class UiTheme {
        ZacosDefault = 0,
        Light = 1,
        Dark = 2,
    };

    enum class PermissionType {
        Geolocation = 0,
        Notifications = 1,
        MediaAudioCapture = 2,
        MediaVideoCapture = 3,
        MediaAudioVideoCapture = 4,
        ClipboardReadWrite = 5,
        DesktopVideoCapture = 6,
        DesktopAudioVideoCapture = 7,
        MouseLock = 8,
        LocalFontsAccess = 9,
    };

    enum class PermissionPolicy {
        Ask = 0,
        Allow = 1,
        Deny = 2,
    };

    struct SitePermissionRule {
        QString origin;
        PermissionType type = PermissionType::Geolocation;
        PermissionPolicy policy = PermissionPolicy::Allow;
    };

    struct AccentEntry {
        const char *id;
        QString label;
        const char *swatchHex;
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
    LinkOpenMode linkOpenMode = LinkOpenMode::Tabs;
    TabBarStyle tabBarStyle = TabBarStyle::IE45Left;
    bool defaultBrowserPrompted = false;
    QString lastUrl;
    QStringList lastTabs;
    int lastActiveTab = 0;
    QString searchEngineId;
    bool autoloadImages = true;
    bool enableJavaScript = true;
    bool rememberPasswords = true;
    bool showNavButtonText = true;
    bool disableGpuAcceleration = false;
    bool enablePlatinumScrollbars = true;
    QString scrollbarAccent;
    UiTheme uiTheme = UiTheme::ZacosDefault;
    bool forceCustomFonts = false;
    ProportionalFontStyle proportionalFontStyle = ProportionalFontStyle::SansSerif;
    QString serifFont;
    QString sansSerifFont;
    QString fixedFont;
    int defaultFontSize = 16;
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

    bool adBlockEnabled = false;
    QStringList adBlockSubscriptions;

    bool autoplayMedia = false;
    QMap<int, PermissionPolicy> defaultPermissionPolicies;
    QVector<SitePermissionRule> sitePermissions;

    static NetzacPreferences defaults();
    static NetzacPreferences load();
    static bool isGpuAccelerationDisabled();
    static void resetAll();
    void save() const;
    static void saveLastUrl(const QString &url);
    static void saveLastTabs(const QStringList &urls, int activeIndex);
    static void ensureDesktopEntryRegistered();
    static bool isDefaultBrowser();
    static bool setAsDefaultBrowser();
    void apply(QWebEngineProfile *profile) const;
    void applyFontSettings(QWebEngineSettings *settings) const;
    QString searchUrlFor(const QString &query) const;

    static QVector<PresetEntry> searchEnginePresets();
    static QVector<PresetEntry> userAgentPresets();
    static QVector<AccentEntry> scrollbarAccentPresets();
    static QStringList defaultAdBlockSubscriptions();
    static QVector<PersonalToolbarItem> defaultPersonalToolbar();
    static QString defaultChromeUserAgent();
    static QString userAgentForPreset(const QString &presetId);

    static QVector<PermissionType> allPermissionTypes();
    static QString permissionTypeKey(PermissionType type);
    static bool permissionTypeFromKey(const QString &key, PermissionType *out);
    static QString permissionTypeLabel(PermissionType type);
    static QString normalizePermissionOrigin(const QUrl &origin);

    PermissionPolicy defaultPolicyFor(PermissionType type) const;
    void setDefaultPolicyFor(PermissionType type, PermissionPolicy policy);
    PermissionPolicy sitePolicyFor(const QString &origin, PermissionType type, bool *found = nullptr) const;
    void setSitePolicy(const QString &origin, PermissionType type, PermissionPolicy policy);

    static QVector<SitePermissionRule> loadSitePermissions();
    static void saveSitePermissions(const QVector<SitePermissionRule> &rules);
    static void upsertSitePermission(const QString &origin, PermissionType type, PermissionPolicy policy);
};

class PasswordStorage {
public:
    struct Credential {
        QString username;
        QString password;
    };

    static bool isAvailable();
    static bool saveCredential(const QString &host, const QString &username, const QString &password);
    static QVector<Credential> lookupCredentials(const QString &host);
    static bool lookupCredential(const QString &host, Credential *out);
    static void clearAll();
};

QWebEngineProfile *netzacProfile();
