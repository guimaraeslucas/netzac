#pragma once

#include "preferences.h"

#include <QDialog>
#include <QElapsedTimer>
#include <QPair>
#include <QPointer>
#include <QString>
#include <QStringList>
#include <QTemporaryDir>
#include <QTimer>
#include <QUrl>
#include <QVector>

class QButtonGroup;
class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QPlainTextEdit;
class QPushButton;
class QRadioButton;
class QSpinBox;
class QSslSocket;
class QStackedWidget;
class QTableWidget;
class QWebEngineDownloadRequest;

class AccentSwatchWidget;
class DownloadGlobeWidget;
class PlatinumPopupButton;
class PlatinumProgressBar;

QPushButton *makeDialogButton(const QString &text, bool isDefault = false, QWidget *parent = nullptr);
QString platinumOpenFileName(
    QWidget *parent,
    const QString &caption,
    const QString &dir = QString(),
    const QString &filter = QString());
QString platinumSaveFileName(
    QWidget *parent,
    const QString &caption,
    const QString &dirOrFile = QString(),
    const QString &filter = QString());

class PlatinumDialog : public QDialog {
    Q_OBJECT

public:
    explicit PlatinumDialog(QWidget *parent = nullptr);
};

class PlatinumAlertDialog : public PlatinumDialog {
    Q_OBJECT

public:
    PlatinumAlertDialog(
        const QString &title,
        const QString &message,
        const QString &kind = QStringLiteral("info"),
        QWidget *parent = nullptr);
};

class AboutDialog : public PlatinumDialog {
    Q_OBJECT

public:
    explicit AboutDialog(QWidget *parent = nullptr);
};

class BookmarksDialog : public PlatinumDialog {
    Q_OBJECT

public:
    explicit BookmarksDialog(
        QVector<QPair<QString, QString>> *bookmarks,
        const QVector<NetzacPreferences::PersonalToolbarItem> &personalToolbar = {},
        QWidget *parent = nullptr);

signals:
    void openRequested(const QString &url);
    void bookmarksModified();

private slots:
    void openSelected();
    void deleteSelected();
    void importBookmarks();
    void exportBookmarks();

private:
    void refresh();

    QVector<QPair<QString, QString>> *m_bookmarks = nullptr;
    QVector<NetzacPreferences::PersonalToolbarItem> m_personalToolbar;
    QListWidget *m_list = nullptr;
};

class PreferencesDialog : public PlatinumDialog {
    Q_OBJECT

public:
    PreferencesDialog(
        const NetzacPreferences &prefs,
        const QString &currentUrl = QString(),
        QWidget *parent = nullptr);

    NetzacPreferences preferences() const;
    bool didResetAll() const { return m_resetRequested; }
    void selectCategory(int index);

private:
    QWidget *buildNavigatorPage(const QString &currentUrl);
    QWidget *buildAppearancePage();
    QWidget *buildCalmModePage();
    QWidget *buildPersonalToolbarPage();
    QWidget *buildQuickLinksPage();
    QWidget *buildCookiesPage();
    QWidget *buildPermissionsPage();
    QWidget *buildCachePage();
    QWidget *buildProxiesPage();
    QWidget *buildUserAgentPage();
    QWidget *buildAdBlockerPage();
    void updateProxyControlsEnabled();
    void refreshAdBlockList();
    void addAdBlockListUrl();
    void updateAdBlockStatus();
    void refreshSitePermissionsTable();
    void updateSitePermissionButtons();

    NetzacPreferences m_prefs;
    QListWidget *m_categoryList = nullptr;
    QLabel *m_pageHeader = nullptr;
    QStackedWidget *m_stack = nullptr;

    // Navigator
    QLineEdit *m_homeEdit = nullptr;
    PlatinumPopupButton *m_startupPopup = nullptr;
    PlatinumPopupButton *m_searchPopup = nullptr;
    QCheckBox *m_imagesChk = nullptr;
    QCheckBox *m_jsChk = nullptr;
    QCheckBox *m_rememberPasswordsChk = nullptr;
    QCheckBox *m_disableGpuChk = nullptr;
    QCheckBox *m_energySaverChk = nullptr;
    QLabel *m_defaultBrowserLbl = nullptr;
    QPushButton *m_setDefaultBrowserBtn = nullptr;
    bool m_resetRequested = false;

    // Appearance
    PlatinumPopupButton *m_uiThemePopup = nullptr;
    QCheckBox *m_scrollbarsChk = nullptr;
    QLabel *m_scrollbarAccentLbl = nullptr;
    PlatinumPopupButton *m_scrollbarAccentPopup = nullptr;
    AccentSwatchWidget *m_accentSwatch = nullptr;
    QCheckBox *m_forceFontsChk = nullptr;
    QWidget *m_fontsBox = nullptr;
    PlatinumPopupButton *m_fontStylePopup = nullptr;
    QComboBox *m_serifFontCombo = nullptr;
    QComboBox *m_sansFontCombo = nullptr;
    QComboBox *m_fixedFontCombo = nullptr;
    QSpinBox *m_fontSizeSpin = nullptr;

    // Calm Mode
    PlatinumPopupButton *m_calmJsPopup = nullptr;
    QRadioButton *m_calmLayoutClean = nullptr;
    QRadioButton *m_calmLayoutReader = nullptr;
    QCheckBox *m_calmRemoveScriptsChk = nullptr;
    QCheckBox *m_calmRemoveIframesChk = nullptr;
    QCheckBox *m_calmRemoveVideosChk = nullptr;
    QCheckBox *m_calmRemoveAdsChk = nullptr;
    QCheckBox *m_calmNormalizeTablesChk = nullptr;
    QSpinBox *m_calmMaxImgSpin = nullptr;
    QLineEdit *m_calmExcludedHostsEdit = nullptr;

    // Personal Toolbar
    QVector<NetzacPreferences::PersonalToolbarItem> m_personalItems;
    QListWidget *m_ptList = nullptr;

    // Quick Links (Mail / Chat / Git)
    QLineEdit *m_mailUrlEdit = nullptr;
    QLineEdit *m_chatUrlEdit = nullptr;
    QLineEdit *m_gitUrlEdit = nullptr;

    // Cookies
    QRadioButton *m_cookieAllowAll = nullptr;
    QRadioButton *m_cookieBlockThird = nullptr;
    QRadioButton *m_cookieBlockAll = nullptr;
    QCheckBox *m_cookieSessionChk = nullptr;
    QLabel *m_cookieStatusLbl = nullptr;

    // Permissions
    QCheckBox *m_autoplayMediaChk = nullptr;
    QTableWidget *m_defaultPermTable = nullptr;
    QVector<PlatinumPopupButton *> m_defaultPermPopups;
    QVector<NetzacPreferences::SitePermissionRule> m_sitePermissions;
    QTableWidget *m_sitePermTable = nullptr;
    QPushButton *m_permAllowBtn = nullptr;
    QPushButton *m_permDenyBtn = nullptr;
    QPushButton *m_permRemoveBtn = nullptr;
    QPushButton *m_permClearBtn = nullptr;

    // Cache
    PlatinumPopupButton *m_cachePopup = nullptr;
    QSpinBox *m_cacheMaxSpin = nullptr;
    QLabel *m_cacheStatusLbl = nullptr;

    // Proxies
    QRadioButton *m_proxyDirect = nullptr;
    QRadioButton *m_proxySystem = nullptr;
    QRadioButton *m_proxyManual = nullptr;
    QWidget *m_proxyManualBox = nullptr;
    PlatinumPopupButton *m_proxyTypePopup = nullptr;
    QLineEdit *m_proxyHostEdit = nullptr;
    QSpinBox *m_proxyPortSpin = nullptr;

    // User Agent
    PlatinumPopupButton *m_uaPresetPopup = nullptr;
    QLineEdit *m_uaEdit = nullptr;
    bool m_syncingUa = false;

    // Ad Blocker
    QStringList m_adBlockUrls;
    QCheckBox *m_adBlockChk = nullptr;
    QListWidget *m_adBlockList = nullptr;
    QLabel *m_adBlockStatusLbl = nullptr;
    QPushButton *m_adBlockAddBtn = nullptr;
    QPushButton *m_adBlockRemoveBtn = nullptr;
    QPushButton *m_adBlockDefaultsBtn = nullptr;
    QPushButton *m_adBlockUpdateBtn = nullptr;
};

class DownloadDialog : public PlatinumDialog {
    Q_OBJECT

public:
    explicit DownloadDialog(QWebEngineDownloadRequest *download, QWidget *parent = nullptr);
    ~DownloadDialog() override;

public slots:
    void reject() override;

private slots:
    void browseDestination();
    void startDownload();
    void updateProgress();
    void openDownloadedFile();
    void openContainingFolder();

private:
    QString resolvedTargetPath() const;
    void finalizeIfReady();
    void finishDownloadUi(bool success, const QString &errorText = QString());

    QPointer<QWebEngineDownloadRequest> m_download;
    QTemporaryDir m_stagingDir;
    QString m_stagingPath;
    QString m_suggestedName;
    QString m_targetPath;
    bool m_userConfirmed = false;
    bool m_downloadDone = false;
    bool m_downloadSuccess = false;
    QString m_downloadError;
    bool m_finished = false;
    bool m_openAutoToggled = false;
    bool m_userToggledOpen = false;

    QElapsedTimer m_elapsed;
    QTimer m_tickTimer;

    DownloadGlobeWidget *m_globeAnim = nullptr;
    QLabel *m_fileNameLbl = nullptr;
    QLabel *m_sourceUrlLbl = nullptr;
    QLabel *m_sizeInfoLbl = nullptr;

    QWidget *m_promptBox = nullptr;
    QRadioButton *m_saveCustomRadio = nullptr;
    QRadioButton *m_saveTempRadio = nullptr;
    QLineEdit *m_customPathEdit = nullptr;
    QPushButton *m_browseBtn = nullptr;
    QLabel *m_tempPathLbl = nullptr;

    QWidget *m_progressBox = nullptr;
    QLabel *m_savingToLbl = nullptr;
    PlatinumProgressBar *m_progressBar = nullptr;
    QLabel *m_downloadedLbl = nullptr;
    QLabel *m_rateLbl = nullptr;
    QLabel *m_elapsedLbl = nullptr;
    QLabel *m_remainingLbl = nullptr;

    QCheckBox *m_openAfterChk = nullptr;

    QPushButton *m_startBtn = nullptr;
    QPushButton *m_openFileBtn = nullptr;
    QPushButton *m_openFolderBtn = nullptr;
    QPushButton *m_cancelCloseBtn = nullptr;
};

class CertificateInfoDialog : public PlatinumDialog {
    Q_OBJECT

public:
    CertificateInfoDialog(const QString &title, const QString &url, QWidget *parent = nullptr);
    ~CertificateInfoDialog() override;

private:
    void startCertificateLookup(const QUrl &url);
    void displayCertificateResult();
    void finishLookupWithError(const QString &reason);

    QPlainTextEdit *m_detailsEdit = nullptr;
    QPushButton *m_copyBtn = nullptr;
    QPointer<QSslSocket> m_socket;
    QTimer m_timeoutTimer;
    QStringList m_sslWarnings;
    bool m_lookupFinished = false;
    QString m_host;
    quint16 m_port = 443;
};

class PermissionPromptDialog : public PlatinumDialog {
    Q_OBJECT

public:
    enum class Decision {
        Dismissed = 0,
        Never,
        JustNow,
        Always,
    };

    PermissionPromptDialog(
        const QUrl &origin,
        NetzacPreferences::PermissionType permissionType,
        QWidget *parent = nullptr);

signals:
    void decided(PermissionPromptDialog::Decision decision);

private:
    Decision m_decision = Decision::Dismissed;
};

class PageSourceDialog : public PlatinumDialog {
    Q_OBJECT

public:
    PageSourceDialog(
        const QString &title,
        const QString &url,
        const QString &sourceHtml,
        QWidget *parent = nullptr);

signals:
    void sourceSaved(const QString &path);

private:
    QString m_sourceHtml;
    QPlainTextEdit *m_sourceEdit = nullptr;
    QPushButton *m_copySelBtn = nullptr;
    QPushButton *m_copyAllBtn = nullptr;
    QPushButton *m_saveBtn = nullptr;
    QLabel *m_feedbackLbl = nullptr;
};
