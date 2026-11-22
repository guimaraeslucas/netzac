#pragma once

#include "calmmode.h"
#include "history.h"
#include "preferences.h"

#include <QMainWindow>
#include <QPair>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QVector>
#include <QWebEnginePage>

class QAction;
class QCloseEvent;
class QHBoxLayout;
class QLabel;
class QLineEdit;
class QMenu;
class QStatusBar;
class QWebEngineContextMenuRequest;
class QWebEngineView;

class CollapsibleToolbar;
class ElidingStatusLabel;
class NetzacThrobber;
class NetzacWebPage;
class NetzacWebView;
class PlatinumCompactButton;
class PlatinumLocationBar;
class PlatinumMiniIconButton;
class PlatinumNavButton;
class PlatinumProgressBar;
class PlatinumViewportFrame;

class NetzacWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit NetzacWindow(
        const QString &initialUrl = QString(),
        QWidget *parent = nullptr,
        bool loadInitial = true);

    NetzacWebView *webView() const { return m_browser; }
    NetzacWebPage *webPage() const { return m_page; }
    CalmModeController *calmController() const { return m_calmController; }
    void setCalmModeEnabled(bool enabled);

public slots:
    void navigateTo(const QString &url);

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void populateGoMenu();
    void populateBookmarksMenu(QMenu *targetMenu = nullptr);

    void onUrlChanged(const QUrl &qurl);
    void onTitleChanged(const QString &title);
    void onLoadStarted();
    void onLoadProgress(int progress);
    void onLoadFinished(bool ok);
    void onLinkHovered(const QString &url);
    void onCalmEnabledChanged(bool enabled);
    void onCalmEvaluationFinished(const CalmModeController::EvaluationReport &report);
    void showWebContextMenu(const QPoint &globalPos, QWebEngineContextMenuRequest *request);

    void showBookmarksPopup();
    void addCurrentBookmark();
    void manageBookmarks();
    void newWindow();
    void openLocationDialog();
    void savePage();
    void printPage();
    void showFindBar();
    void hideFindBar();
    void findNext();
    void viewPageSource();
    void showPageInfo();
    void showHistoryDialog();
    void openPreferences();
    void showAbout();

private:
    void buildMenus();
    void buildToolbars();
    void buildCentralArea();
    void rebuildPersonalToolbar();
    void buildStatusBar();
    void connectBrowserSignals();
    void updateNavStates();
    void loadBuiltinAbout(const QString &aboutUrl);
    void buildNavHistoryMenu(QMenu &menu, bool back);
    void writeSavedHtml(const QString &path, const QString &htmlText);
    QWebEngineView *createBrowserWindow(QWebEnginePage::WebWindowType type);

    QAction *addMenuAction(QMenu *menu, const QString &text, const QKeySequence &shortcut = QKeySequence());
    QAction *addCheckableAction(QMenu *menu, const QString &text, bool checked);

    NetzacPreferences m_prefs;
    QVector<QPair<QString, QString>> m_bookmarks;
    QVector<HistoryEntry> m_history;
    QStringList m_visitedUrls;
    QString m_lastSearch;
    QString m_currentUrlStr;
    bool m_showingBuiltin = false;
    bool m_isLoading = false;

    NetzacWebView *m_browser = nullptr;
    NetzacWebPage *m_page = nullptr;
    CalmModeController *m_calmController = nullptr;

    QMenu *m_goMenu = nullptr;
    QMenu *m_bookmarksMenu = nullptr;
    QAction *m_actShowNav = nullptr;
    QAction *m_actShowLoc = nullptr;
    QAction *m_actShowPers = nullptr;
    QAction *m_actShowStatus = nullptr;
    QAction *m_actCalmMode = nullptr;
    QAction *m_actStopMenu = nullptr;

    PlatinumNavButton *m_btnBack = nullptr;
    PlatinumNavButton *m_btnForward = nullptr;
    PlatinumNavButton *m_btnHome = nullptr;
    PlatinumNavButton *m_btnReload = nullptr;
    PlatinumNavButton *m_btnPrint = nullptr;
    PlatinumNavButton *m_btnFind = nullptr;
    PlatinumNavButton *m_btnStop = nullptr;
    PlatinumNavButton *m_btnCalm = nullptr;
    NetzacThrobber *m_throbber = nullptr;

    PlatinumCompactButton *m_btnBookmarks = nullptr;
    QLabel *m_locationLabel = nullptr;
    PlatinumMiniIconButton *m_pageProxy = nullptr;
    PlatinumLocationBar *m_urlBar = nullptr;

    CollapsibleToolbar *m_navToolbar = nullptr;
    CollapsibleToolbar *m_locToolbar = nullptr;
    CollapsibleToolbar *m_persToolbar = nullptr;
    QWidget *m_persBody = nullptr;
    QHBoxLayout *m_persLay = nullptr;

    QWidget *m_findBar = nullptr;
    QLineEdit *m_findInput = nullptr;
    PlatinumViewportFrame *m_viewportFrame = nullptr;

    QStatusBar *m_status = nullptr;
    PlatinumMiniIconButton *m_statusSecurity = nullptr;
    PlatinumProgressBar *m_progressBar = nullptr;
    ElidingStatusLabel *m_statusLabel = nullptr;
};
