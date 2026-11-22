#include "netzacwindow.h"

#include "bookmarks.h"
#include "dialogs.h"
#include "i18n.h"
#include "platinum.h"
#include "webview.h"
#include "widgets.h"

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QColor>
#include <QDateTime>
#include <QFile>
#include <QFileDialog>
#include <QFont>
#include <QHBoxLayout>
#include <QIcon>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMenuBar>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QStatusBar>
#include <QTextStream>
#include <QVBoxLayout>
#include <QWebEngineContextMenuRequest>
#include <QWebEngineHistory>
#include <QWebEnginePage>
#include <QWebEngineProfile>
#include <QWebEngineSettings>

namespace {

QString retroHtmlPage(const QString &title, const QString &subtitle, const QString &bodyHtml)
{
    const QString footer = ztr("Netzac Communicator 4.0 &mdash; ZacOS 9 Platinum Edition");
    return QStringLiteral(
        "<!DOCTYPE html>\n"
        "<html>\n"
        "<head>\n"
        "<meta charset=\"utf-8\">\n"
        "<title>%1</title>\n"
        "<style>\n"
        "  body {\n"
        "    background-color: #c0c0c0;\n"
        "    color: #000000;\n"
        "    font-family: 'Nimbus Roman', 'Liberation Serif', Georgia, serif;\n"
        "    margin: 24px 36px;\n"
        "  }\n"
        "  .banner {\n"
        "    background: linear-gradient(to right, #001838, #0f5656);\n"
        "    color: #ffffff;\n"
        "    border: 2px outset #eeeeee;\n"
        "    padding: 12px 16px;\n"
        "    margin-bottom: 16px;\n"
        "  }\n"
        "  .banner h1 {\n"
        "    margin: 0;\n"
        "    font-size: 22px;\n"
        "    letter-spacing: 0.5px;\n"
        "  }\n"
        "  .banner p {\n"
        "    margin: 4px 0 0 0;\n"
        "    color: #cceeee;\n"
        "    font-size: 13px;\n"
        "  }\n"
        "  .box {\n"
        "    background-color: #dddddd;\n"
        "    border: 2px groove #ffffff;\n"
        "    padding: 12px 16px;\n"
        "    margin-bottom: 14px;\n"
        "  }\n"
        "  a { color: #0000ee; }\n"
        "  a:visited { color: #551a8b; }\n"
        "  hr { border: 1px inset #888888; }\n"
        "  code { background: #eeeeee; padding: 1px 4px; border: 1px solid #888888; }\n"
        "</style>\n"
        "</head>\n"
        "<body>\n"
        "  <div class=\"banner\">\n"
        "    <h1>%1</h1>\n"
        "    <p>%2</p>\n"
        "  </div>\n"
        "  <div class=\"box\">\n"
        "    %3\n"
        "  </div>\n"
        "  <hr>\n"
        "  <p style=\"font-size: 12px; color: #333333;\">\n"
        "    %4\n"
        "  </p>\n"
        "</body>\n"
        "</html>")
        .arg(title.toHtmlEscaped(), subtitle.toHtmlEscaped(), bodyHtml, footer);
}

}  // namespace

NetzacWindow::NetzacWindow(const QString &initialUrl, QWidget *parent, bool loadInitial)
    : QMainWindow(parent)
    , m_prefs(NetzacPreferences::load())
    , m_currentUrlStr(m_prefs.homeUrl)
{
    setWindowTitle(ztr("Netzac"));
    setMinimumSize(640, 400);
    resize(1024, 768);

    m_bookmarks = BookmarkStorage::load();
    m_history = HistoryStorage::load();
    for (const HistoryEntry &h : m_history) {
        if (!m_visitedUrls.contains(h.url))
            m_visitedUrls.append(h.url);
    }
    for (const auto &entry : m_bookmarks) {
        if (!m_visitedUrls.contains(entry.second))
            m_visitedUrls.append(entry.second);
    }

    m_browser = new NetzacWebView(this);
    m_page = new NetzacWebPage(netzacProfile(), m_browser);
    m_browser->setPage(m_page);
    m_calmController = new CalmModeController(m_page, m_prefs.calm, this);

    m_browser->setMinimumSize(320, 200);
    m_browser->page()->setBackgroundColor(QColor(QStringLiteral("#dddddd")));
    m_browser->page()->settings()->setAttribute(
        QWebEngineSettings::AutoLoadImages, m_prefs.autoloadImages);
    m_browser->page()->settings()->setAttribute(
        QWebEngineSettings::LocalStorageEnabled, true);
    m_browser->setWindowCreator([this](QWebEnginePage::WebWindowType type) {
        return createBrowserWindow(type);
    });

    buildMenus();
    buildToolbars();
    buildCentralArea();
    buildStatusBar();
    connectBrowserSignals();

    if (loadInitial) {
        QString startTarget = initialUrl;
        if (startTarget.isEmpty()) {
            if (m_prefs.startupMode == NetzacPreferences::StartupMode::RestoreLastPage
                && !m_prefs.lastUrl.isEmpty()) {
                startTarget = m_prefs.lastUrl;
            } else {
                startTarget = m_prefs.homeUrl;
            }
        }
        navigateTo(startTarget);
    }
}

void NetzacWindow::closeEvent(QCloseEvent *event)
{
    HistoryStorage::save(m_history);
    NetzacPreferences::saveLastUrl(m_currentUrlStr);
    QMainWindow::closeEvent(event);
}

QAction *NetzacWindow::addMenuAction(QMenu *menu, const QString &text, const QKeySequence &shortcut)
{
    auto *act = new QAction(text, this);
    if (!shortcut.isEmpty())
        act->setShortcut(shortcut);
    menu->addAction(act);
    return act;
}

QAction *NetzacWindow::addCheckableAction(QMenu *menu, const QString &text, bool checked)
{
    auto *act = new QAction(text, this);
    act->setCheckable(true);
    act->setChecked(checked);
    menu->addAction(act);
    return act;
}

void NetzacWindow::buildMenus()
{
    QMenuBar *mb = menuBar();

    // File
    QMenu *fileMenu = mb->addMenu(ztr("File"));
    connect(addMenuAction(fileMenu, ztr("New Navigator Window"), QKeySequence::New),
            &QAction::triggered, this, &NetzacWindow::newWindow);
    connect(addMenuAction(fileMenu, ztr("Open Page..."), QKeySequence::Open),
            &QAction::triggered, this, &NetzacWindow::openLocationDialog);
    connect(addMenuAction(fileMenu, ztr("Save As..."), QKeySequence::Save),
            &QAction::triggered, this, &NetzacWindow::savePage);
    fileMenu->addSeparator();
    connect(addMenuAction(fileMenu, ztr("Print..."), QKeySequence::Print),
            &QAction::triggered, this, &NetzacWindow::printPage);
    fileMenu->addSeparator();
    connect(addMenuAction(fileMenu, ztr("Close"), QKeySequence::Close),
            &QAction::triggered, this, &QWidget::close);
    connect(addMenuAction(fileMenu, ztr("Quit"), QKeySequence::Quit),
            &QAction::triggered, QApplication::instance(), &QApplication::quit);

    // Edit
    QMenu *editMenu = mb->addMenu(ztr("Edit"));
    QWebEnginePage *page = m_browser->page();
    connect(addMenuAction(editMenu, ztr("Undo"), QKeySequence::Undo),
            &QAction::triggered, this, [page]() { page->triggerAction(QWebEnginePage::Undo); });
    editMenu->addSeparator();
    connect(addMenuAction(editMenu, ztr("Cut"), QKeySequence::Cut),
            &QAction::triggered, this, [page]() { page->triggerAction(QWebEnginePage::Cut); });
    connect(addMenuAction(editMenu, ztr("Copy"), QKeySequence::Copy),
            &QAction::triggered, this, [page]() { page->triggerAction(QWebEnginePage::Copy); });
    connect(addMenuAction(editMenu, ztr("Paste"), QKeySequence::Paste),
            &QAction::triggered, this, [page]() { page->triggerAction(QWebEnginePage::Paste); });
    connect(addMenuAction(editMenu, ztr("Select All"), QKeySequence::SelectAll),
            &QAction::triggered, this, [page]() { page->triggerAction(QWebEnginePage::SelectAll); });
    editMenu->addSeparator();
    connect(addMenuAction(editMenu, ztr("Find in Page..."), QKeySequence::Find),
            &QAction::triggered, this, &NetzacWindow::showFindBar);
    connect(addMenuAction(editMenu, ztr("Find Again"), QKeySequence(QStringLiteral("Ctrl+G"))),
            &QAction::triggered, this, &NetzacWindow::findNext);
    editMenu->addSeparator();
    connect(addMenuAction(editMenu, ztr("Preferences...")),
            &QAction::triggered, this, &NetzacWindow::openPreferences);

    // View
    QMenu *viewMenu = mb->addMenu(ztr("View"));
    QMenu *showHideMenu = viewMenu->addMenu(ztr("Show/Hide"));
    m_actShowNav = addCheckableAction(showHideMenu, ztr("Navigation Toolbar"), true);
    m_actShowLoc = addCheckableAction(showHideMenu, ztr("Location Toolbar"), true);
    m_actShowPers = addCheckableAction(showHideMenu, ztr("Personal Toolbar"), true);
    m_actShowStatus = addCheckableAction(showHideMenu, ztr("Status Bar"), true);
    viewMenu->addSeparator();
    m_actStopMenu = addMenuAction(viewMenu, ztr("Stop"), QKeySequence(QStringLiteral("Esc")));
    connect(m_actStopMenu, &QAction::triggered, m_browser, &QWebEngineView::stop);
    connect(addMenuAction(viewMenu, ztr("Reload"), QKeySequence(QStringLiteral("Ctrl+R"))),
            &QAction::triggered, m_browser, &QWebEngineView::reload);
    viewMenu->addSeparator();
    m_actCalmMode = addCheckableAction(viewMenu, ztr("Calm Mode"), false);
    m_actCalmMode->setShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+C")));
    connect(m_actCalmMode, &QAction::toggled, m_calmController, &CalmModeController::setEnabled);
    viewMenu->addSeparator();
    connect(addMenuAction(viewMenu, ztr("Page Source"), QKeySequence(QStringLiteral("Ctrl+U"))),
            &QAction::triggered, this, &NetzacWindow::viewPageSource);
    connect(addMenuAction(viewMenu, ztr("Page Info"), QKeySequence(QStringLiteral("Ctrl+I"))),
            &QAction::triggered, this, &NetzacWindow::showPageInfo);

    // Go
    m_goMenu = mb->addMenu(ztr("Go"));
    connect(m_goMenu, &QMenu::aboutToShow, this, &NetzacWindow::populateGoMenu);
    populateGoMenu();

    // Communicator
    QMenu *commMenu = mb->addMenu(ztr("Communicator"));
    connect(addMenuAction(commMenu, ztr("Navigator"), QKeySequence(QStringLiteral("Ctrl+1"))),
            &QAction::triggered, this, [this]() { navigateTo(QStringLiteral("about:netzac")); });
    connect(addMenuAction(commMenu, ztr("Mail"), QKeySequence(QStringLiteral("Ctrl+2"))),
            &QAction::triggered, this, [this]() {
                navigateTo(m_prefs.mailUrl.isEmpty() ? QStringLiteral("about:mail") : m_prefs.mailUrl);
            });
    connect(addMenuAction(commMenu, ztr("Chat"), QKeySequence(QStringLiteral("Ctrl+3"))),
            &QAction::triggered, this, [this]() {
                navigateTo(m_prefs.chatUrl.isEmpty() ? QStringLiteral("about:chat") : m_prefs.chatUrl);
            });
    connect(addMenuAction(commMenu, ztr("Git"), QKeySequence(QStringLiteral("Ctrl+4"))),
            &QAction::triggered, this, [this]() {
                navigateTo(m_prefs.gitUrl.isEmpty() ? QStringLiteral("about:git") : m_prefs.gitUrl);
            });
    commMenu->addSeparator();
    m_bookmarksMenu = commMenu->addMenu(ztr("Bookmarks"));
    connect(m_bookmarksMenu, &QMenu::aboutToShow, this, [this]() { populateBookmarksMenu(nullptr); });
    populateBookmarksMenu(nullptr);
    connect(addMenuAction(commMenu, ztr("History"), QKeySequence(QStringLiteral("Ctrl+H"))),
            &QAction::triggered, this, &NetzacWindow::showHistoryDialog);
    connect(addMenuAction(commMenu, ztr("Security Info")),
            &QAction::triggered, this, &NetzacWindow::showPageInfo);

    // Help
    QMenu *helpMenu = mb->addMenu(ztr("Help"));
    connect(addMenuAction(helpMenu, ztr("Help Contents"), QKeySequence::HelpContents),
            &QAction::triggered, this, [this]() { navigateTo(QStringLiteral("about:help")); });
    helpMenu->addSeparator();
    connect(addMenuAction(helpMenu, ztr("About Netzac...")),
            &QAction::triggered, this, &NetzacWindow::showAbout);
}

void NetzacWindow::populateGoMenu()
{
    m_goMenu->clear();
    QAction *backAct = addMenuAction(m_goMenu, ztr("Back"), QKeySequence(QStringLiteral("Alt+Left")));
    connect(backAct, &QAction::triggered, m_browser, &QWebEngineView::back);
    backAct->setEnabled(m_browser->history()->canGoBack());

    QAction *fwdAct = addMenuAction(m_goMenu, ztr("Forward"), QKeySequence(QStringLiteral("Alt+Right")));
    connect(fwdAct, &QAction::triggered, m_browser, &QWebEngineView::forward);
    fwdAct->setEnabled(m_browser->history()->canGoForward());

    connect(addMenuAction(m_goMenu, ztr("Home"), QKeySequence(QStringLiteral("Alt+Home"))),
            &QAction::triggered, this, [this]() { navigateTo(m_prefs.homeUrl); });
    m_goMenu->addSeparator();
    connect(addMenuAction(m_goMenu, ztr("History..."), QKeySequence(QStringLiteral("Ctrl+H"))),
            &QAction::triggered, this, &NetzacWindow::showHistoryDialog);

    QWebEngineHistory *hist = m_browser->history();
    const QList<QWebEngineHistoryItem> items = hist->items();
    if (!items.isEmpty()) {
        m_goMenu->addSeparator();
        const int cur = hist->currentItemIndex();
        const int total = int(items.size());
        const int start = qMax(0, total - 10);
        for (int realIdx = total - 1; realIdx >= start; --realIdx) {
            const QWebEngineHistoryItem &item = items.at(realIdx);
            QString label = item.title().isEmpty() ? item.url().toString() : item.title();
            if (label.size() > 42)
                label = label.left(39) + QStringLiteral("...");
            QAction *act = addCheckableAction(m_goMenu, label, realIdx == cur);
            connect(act, &QAction::triggered, this, [hist, item]() { hist->goToItem(item); });
        }
    }
}

void NetzacWindow::populateBookmarksMenu(QMenu *targetMenu)
{
    QMenu *menu = targetMenu ? targetMenu : m_bookmarksMenu;
    menu->clear();
    connect(addMenuAction(menu, ztr("Add Bookmark"), QKeySequence(QStringLiteral("Ctrl+D"))),
            &QAction::triggered, this, &NetzacWindow::addCurrentBookmark);
    connect(addMenuAction(menu, ztr("Edit Bookmarks..."), QKeySequence(QStringLiteral("Ctrl+B"))),
            &QAction::triggered, this, &NetzacWindow::manageBookmarks);
    menu->addSeparator();
    for (const auto &entry : m_bookmarks) {
        QAction *act = menu->addAction(entry.first);
        act->setToolTip(entry.second);
        const QString url = entry.second;
        connect(act, &QAction::triggered, this, [this, url]() { navigateTo(url); });
    }
}

void NetzacWindow::buildToolbars()
{
    // 1. Navigation Toolbar: [Back][Forward][Home]  [Reload][Print][Find]  [Stop] ... [Z]
    auto *navBody = new QWidget();
    auto *navLay = new QHBoxLayout(navBody);
    navLay->setContentsMargins(0, 0, 0, 0);
    navLay->setSpacing(2);

    m_btnBack = new PlatinumNavButton(ztr("Back"), QStringLiteral("back"));
    m_btnBack->setToolTip(ztr("Go back one page (hold for history)"));
    connect(m_btnBack, &QPushButton::clicked, m_browser, &QWebEngineView::back);
    m_btnBack->setHistoryMenuProvider([this](QMenu &menu) { buildNavHistoryMenu(menu, true); });

    m_btnForward = new PlatinumNavButton(ztr("Forward"), QStringLiteral("forward"));
    m_btnForward->setToolTip(ztr("Go forward one page (hold for history)"));
    connect(m_btnForward, &QPushButton::clicked, m_browser, &QWebEngineView::forward);
    m_btnForward->setHistoryMenuProvider([this](QMenu &menu) { buildNavHistoryMenu(menu, false); });

    m_btnHome = new PlatinumNavButton(ztr("Home"), QStringLiteral("home"));
    m_btnHome->setToolTip(ztr("Go to Home page"));
    connect(m_btnHome, &QPushButton::clicked, this, [this]() { navigateTo(m_prefs.homeUrl); });

    m_btnReload = new PlatinumNavButton(ztr("Reload"), QStringLiteral("reload"));
    m_btnReload->setToolTip(ztr("Reload current page"));
    connect(m_btnReload, &QPushButton::clicked, m_browser, &QWebEngineView::reload);

    m_btnPrint = new PlatinumNavButton(ztr("Print"), QStringLiteral("print"));
    m_btnPrint->setToolTip(ztr("Print this page"));
    connect(m_btnPrint, &QPushButton::clicked, this, &NetzacWindow::printPage);

    m_btnFind = new PlatinumNavButton(ztr("Find"), QStringLiteral("find"));
    m_btnFind->setToolTip(ztr("Find in current page"));
    connect(m_btnFind, &QPushButton::clicked, this, &NetzacWindow::showFindBar);

    m_btnStop = new PlatinumNavButton(ztr("Stop"), QStringLiteral("stop"));
    m_btnStop->setToolTip(ztr("Stop loading"));
    connect(m_btnStop, &QPushButton::clicked, m_browser, &QWebEngineView::stop);

    m_btnCalm = new PlatinumNavButton(ztr("Calm"), QStringLiteral("calm"));
    m_btnCalm->setCheckable(true);
    m_btnCalm->setToolTip(ztr("Toggle Calm reading mode (configure filters in Edit > Preferences > Calm Mode)"));
    connect(m_btnCalm, &QPushButton::toggled, m_calmController, &CalmModeController::setEnabled);

    navLay->addWidget(m_btnBack);
    navLay->addWidget(m_btnForward);
    navLay->addWidget(m_btnHome);
    navLay->addSpacing(12);
    navLay->addWidget(m_btnReload);
    navLay->addWidget(m_btnPrint);
    navLay->addWidget(m_btnFind);
    navLay->addSpacing(12);
    navLay->addWidget(m_btnStop);
    navLay->addSpacing(12);
    navLay->addWidget(m_btnCalm);

    navLay->addStretch(1);

    m_throbber = new NetzacThrobber();
    connect(m_throbber, &QPushButton::clicked, this, [this]() {
        navigateTo(QStringLiteral("about:netzac"));
    });
    navLay->addWidget(m_throbber);

    m_navToolbar = new CollapsibleToolbar(ztr("Navigation Toolbar"), navBody, this);

    // 2. Location Toolbar
    auto *locBody = new QWidget();
    auto *locLay = new QHBoxLayout(locBody);
    locLay->setContentsMargins(0, 1, 0, 1);
    locLay->setSpacing(6);

    m_btnBookmarks = new PlatinumCompactButton(ztr("Bookmarks"), QStringLiteral("bookmarks"), true);
    connect(m_btnBookmarks, &QPushButton::clicked, this, &NetzacWindow::showBookmarksPopup);
    locLay->addWidget(m_btnBookmarks);

    m_locationLabel = new QLabel(ztr("Netsite:"));
    m_locationLabel->setFont(zacosFont(QStringLiteral("ZacOS 9 System"), 12));
    locLay->addWidget(m_locationLabel);

    m_pageProxy = new PlatinumMiniIconButton(
        QStringLiteral("page"), ztr("Current page URL (click to bookmark)"));
    connect(m_pageProxy, &QPushButton::clicked, this, &NetzacWindow::addCurrentBookmark);
    locLay->addWidget(m_pageProxy);

    m_urlBar = new PlatinumLocationBar(m_visitedUrls);
    connect(m_urlBar, &PlatinumLocationBar::urlSubmitted, this, &NetzacWindow::navigateTo);
    locLay->addWidget(m_urlBar, 1);

    m_locToolbar = new CollapsibleToolbar(ztr("Location Toolbar"), locBody, this);

    // 3. Personal Toolbar
    m_persBody = new QWidget();
    m_persLay = new QHBoxLayout(m_persBody);
    m_persLay->setContentsMargins(0, 1, 0, 1);
    m_persLay->setSpacing(4);

    rebuildPersonalToolbar();

    m_persToolbar = new CollapsibleToolbar(ztr("Personal Toolbar"), m_persBody, this);

    connect(m_actShowNav, &QAction::toggled, m_navToolbar, &QWidget::setVisible);
    connect(m_actShowLoc, &QAction::toggled, m_locToolbar, &QWidget::setVisible);
    connect(m_actShowPers, &QAction::toggled, m_persToolbar, &QWidget::setVisible);
}

void NetzacWindow::rebuildPersonalToolbar()
{
    if (!m_persLay)
        return;

    while (QLayoutItem *child = m_persLay->takeAt(0)) {
        if (QWidget *w = child->widget())
            w->deleteLater();
        delete child;
    }

    for (const auto &link : m_prefs.personalToolbar) {
        const bool isFolder = link.isFolder || link.icon == QLatin1String("folder");
        const QString iconName = isFolder
            ? QStringLiteral("folder")
            : (link.icon.isEmpty() ? QStringLiteral("page") : link.icon);
        auto *btn = new PlatinumCompactButton(link.title, iconName, isFolder);
        if (isFolder) {
            btn->setToolTip(ztr("%1 (%2 item(s))").arg(link.title).arg(link.children.size()));
            const QVector<NetzacPreferences::PersonalToolbarSubItem> children = link.children;
            connect(btn, &QPushButton::clicked, this, [this, btn, children]() {
                QMenu menu(this);
                if (children.isEmpty()) {
                    QAction *emptyAct = menu.addAction(ztr("(Empty folder)"));
                    emptyAct->setEnabled(false);
                } else {
                    for (const auto &sub : children) {
                        QAction *act = menu.addAction(sub.title);
                        act->setToolTip(sub.url);
                        const QString targetUrl = sub.url;
                        connect(act, &QAction::triggered, this, [this, targetUrl]() {
                            navigateTo(targetUrl);
                        });
                    }
                }
                menu.exec(btn->mapToGlobal(QPoint(0, btn->height())));
            });
        } else {
            btn->setToolTip(link.url);
            const QString targetUrl = link.url;
            connect(btn, &QPushButton::clicked, this, [this, targetUrl]() { navigateTo(targetUrl); });
        }
        m_persLay->addWidget(btn);
    }

    m_persLay->addStretch(1);
}

void NetzacWindow::buildCentralArea()
{
    auto *central = new QWidget(this);
    auto *root = new QVBoxLayout(central);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    root->addWidget(m_navToolbar);
    root->addWidget(m_locToolbar);
    root->addWidget(m_persToolbar);

    m_findBar = new QWidget(central);
    auto *findLay = new QHBoxLayout(m_findBar);
    findLay->setContentsMargins(6, 3, 6, 3);
    findLay->setSpacing(6);
    findLay->addWidget(new QLabel(ztr("Find:"), m_findBar));
    m_findInput = new QLineEdit(m_findBar);
    m_findInput->setFixedWidth(220);
    connect(m_findInput, &QLineEdit::returnPressed, this, &NetzacWindow::findNext);
    findLay->addWidget(m_findInput);
    auto *findNextBtn = new QPushButton(ztr("Find Next"), m_findBar);
    findNextBtn->setIcon(QIcon());
    connect(findNextBtn, &QPushButton::clicked, this, &NetzacWindow::findNext);
    auto *findCloseBtn = new QPushButton(ztr("Done"), m_findBar);
    findCloseBtn->setIcon(QIcon());
    connect(findCloseBtn, &QPushButton::clicked, this, &NetzacWindow::hideFindBar);
    findLay->addWidget(findNextBtn);
    findLay->addWidget(findCloseBtn);
    findLay->addStretch(1);
    m_findBar->hide();

    m_viewportFrame = new PlatinumViewportFrame(m_browser, central);
    root->addWidget(m_viewportFrame, 1);
    root->addWidget(m_findBar);

    setCentralWidget(central);
}

void NetzacWindow::buildStatusBar()
{
    m_status = new QStatusBar(this);
    m_status->setSizeGripEnabled(true);
    m_status->setFont(zacosFont(QStringLiteral("ZacOS 9 Views"), 9));
    setStatusBar(m_status);

    m_statusSecurity = new PlatinumMiniIconButton(
        QStringLiteral("security_open"), ztr("Security Info"), m_status);
    connect(m_statusSecurity, &QPushButton::clicked, this, &NetzacWindow::showPageInfo);
    m_status->addWidget(m_statusSecurity);

    m_progressBar = new PlatinumProgressBar(m_status);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_status->addWidget(m_progressBar);

    m_statusLabel = new ElidingStatusLabel(ztr("Document: Done"), m_status);
    m_statusLabel->setFont(zacosFont(QStringLiteral("ZacOS 9 Views"), 9));
    m_status->addWidget(m_statusLabel, 1);

    auto *compBox = new QWidget(m_status);
    auto *compLay = new QHBoxLayout(compBox);
    compLay->setContentsMargins(2, 0, 2, 0);
    compLay->setSpacing(2);

    auto *bNav = new PlatinumMiniIconButton(QStringLiteral("navigator"), ztr("Navigator (Ctrl+1)"), compBox);
    connect(bNav, &QPushButton::clicked, this, [this]() { navigateTo(QStringLiteral("about:netzac")); });
    compLay->addWidget(bNav);

    auto *bMail = new PlatinumMiniIconButton(QStringLiteral("mail"), ztr("Mail (Ctrl+2)"), compBox);
    connect(bMail, &QPushButton::clicked, this, [this]() {
        navigateTo(m_prefs.mailUrl.isEmpty() ? QStringLiteral("about:mail") : m_prefs.mailUrl);
    });
    compLay->addWidget(bMail);

    auto *bChat = new PlatinumMiniIconButton(QStringLiteral("collabra"), ztr("Chat (Ctrl+3)"), compBox);
    connect(bChat, &QPushButton::clicked, this, [this]() {
        navigateTo(m_prefs.chatUrl.isEmpty() ? QStringLiteral("about:chat") : m_prefs.chatUrl);
    });
    compLay->addWidget(bChat);

    auto *bGit = new PlatinumMiniIconButton(QStringLiteral("composer"), ztr("Git (Ctrl+4)"), compBox);
    connect(bGit, &QPushButton::clicked, this, [this]() {
        navigateTo(m_prefs.gitUrl.isEmpty() ? QStringLiteral("about:git") : m_prefs.gitUrl);
    });
    compLay->addWidget(bGit);

    m_status->addPermanentWidget(compBox);

    connect(m_actShowStatus, &QAction::toggled, m_status, &QWidget::setVisible);
    updateNavStates();
}

void NetzacWindow::connectBrowserSignals()
{
    connect(m_browser, &QWebEngineView::urlChanged, this, &NetzacWindow::onUrlChanged);
    connect(m_browser, &QWebEngineView::titleChanged, this, &NetzacWindow::onTitleChanged);
    connect(m_browser, &QWebEngineView::loadStarted, this, &NetzacWindow::onLoadStarted);
    connect(m_browser, &QWebEngineView::loadProgress, this, &NetzacWindow::onLoadProgress);
    connect(m_browser, &QWebEngineView::loadFinished, this, &NetzacWindow::onLoadFinished);
    connect(m_browser->page(), &QWebEnginePage::linkHovered, this, &NetzacWindow::onLinkHovered);
    connect(m_browser->page(), &QWebEnginePage::windowCloseRequested, this, &QWidget::close);
    connect(m_browser, &NetzacWebView::contextMenuRequested, this, &NetzacWindow::showWebContextMenu);
    connect(
        m_calmController,
        &CalmModeController::enabledChanged,
        this,
        &NetzacWindow::onCalmEnabledChanged);
    connect(
        m_calmController,
        &CalmModeController::evaluationFinished,
        this,
        &NetzacWindow::onCalmEvaluationFinished,
        Qt::QueuedConnection);
}

void NetzacWindow::setCalmModeEnabled(bool enabled)
{
    if (m_calmController)
        m_calmController->setEnabled(enabled);
}

void NetzacWindow::onCalmEnabledChanged(bool enabled)
{
    if (m_btnCalm) {
        const QSignalBlocker blocker(m_btnCalm);
        m_btnCalm->setChecked(enabled);
        m_btnCalm->update();
    }
    if (m_actCalmMode) {
        const QSignalBlocker blocker(m_actCalmMode);
        m_actCalmMode->setChecked(enabled);
    }
}

void NetzacWindow::onCalmEvaluationFinished(const CalmModeController::EvaluationReport &report)
{
    if (!m_statusLabel || m_isLoading)
        return;

    switch (report.status) {
    case CalmModeController::Status::Applied:
    case CalmModeController::Status::AlreadyApplied:
        m_statusLabel->setStatusText(
            report.usedReaderFocus
                ? ztr("Document: Done — Calm Reader active")
                : ztr("Document: Done — Calm Mode active"));
        break;
    case CalmModeController::Status::ExcludedHost:
        m_statusLabel->setStatusText(ztr("Document: Done — Calm Mode skipped (excluded host)"));
        break;
    case CalmModeController::Status::SkippedBuiltIn:
        m_statusLabel->setStatusText(ztr("Document: Done — Calm Mode skipped (built-in page)"));
        break;
    case CalmModeController::Status::SkippedJavaScript:
        m_statusLabel->setStatusText(ztr("Document: Done — Calm Mode skipped (JavaScript page)"));
        break;
    case CalmModeController::Status::SkippedWebApp:
        m_statusLabel->setStatusText(ztr("Document: Done — Calm Mode skipped (interactive web app)"));
        break;
    case CalmModeController::Status::Disabled:
        m_statusLabel->setStatusText(ztr("Document: Done"));
        break;
    case CalmModeController::Status::Error:
        m_statusLabel->setStatusText(ztr("Document: Done"));
        break;
    }
}

void NetzacWindow::onUrlChanged(const QUrl &qurl)
{
    QString urlStr = qurl.toString();
    if (m_showingBuiltin
        && (urlStr == QLatin1String("https://localhost/") || urlStr == QLatin1String("about:blank"))) {
        urlStr = m_currentUrlStr;
    } else if (urlStr.startsWith(QLatin1String("data:")) || urlStr == QLatin1String("about:blank")) {
        urlStr = m_currentUrlStr;
    } else {
        m_showingBuiltin = false;
        m_currentUrlStr = urlStr;
    }

    m_urlBar->setText(urlStr);
    if (!urlStr.isEmpty() && !m_visitedUrls.contains(urlStr)) {
        m_visitedUrls.prepend(urlStr);
        m_urlBar->recordUrl(urlStr);
    }
    HistoryStorage::recordVisit(m_history, urlStr, m_browser->title());
    NetzacPreferences::saveLastUrl(urlStr);

    const bool isWeb = urlStr.startsWith(QLatin1String("http://"))
        || urlStr.startsWith(QLatin1String("https://"));
    m_locationLabel->setText(isWeb ? ztr("Netsite:") : ztr("Location:"));
    const bool isHttps = urlStr.startsWith(QLatin1String("https://"));
    const QString secIcon = isHttps ? QStringLiteral("security_locked") : QStringLiteral("security_open");
    m_statusSecurity->setIconName(secIcon);
    updateNavStates();
}

void NetzacWindow::onTitleChanged(const QString &title)
{
    if (!title.isEmpty()) {
        setWindowTitle(ztr("Netzac: %1").arg(title));
        HistoryStorage::recordVisit(m_history, m_currentUrlStr, title);
    } else {
        setWindowTitle(ztr("Netzac"));
    }
}

void NetzacWindow::onLoadStarted()
{
    m_isLoading = true;
    m_throbber->setLoading(true);
    m_progressBar->setIndeterminate(true);
    m_progressBar->setValue(0);
    m_statusLabel->setStatusText(ztr("Connecting to %1...").arg(m_currentUrlStr));
    updateNavStates();
}

void NetzacWindow::onLoadProgress(int progress)
{
    if (progress > 0 && progress < 100) {
        m_progressBar->setIndeterminate(false);
        m_progressBar->setValue(progress);
        m_statusLabel->setStatusText(ztr("Loading: %1%").arg(progress));
    } else if (progress >= 100) {
        m_progressBar->setIndeterminate(false);
        m_progressBar->setValue(100);
        m_statusLabel->setStatusText(ztr("Document: Done"));
    }
}

void NetzacWindow::onLoadFinished(bool ok)
{
    m_isLoading = false;
    m_throbber->setLoading(false);
    m_progressBar->setIndeterminate(false);
    m_progressBar->setValue(0);
    m_statusLabel->setStatusText(ok ? ztr("Document: Done") : ztr("Transfer interrupted"));
    if (ok) {
        HistoryStorage::recordVisit(m_history, m_currentUrlStr, m_browser->title());
        HistoryStorage::save(m_history);
    }
    updateNavStates();
}

void NetzacWindow::onLinkHovered(const QString &url)
{
    if (!url.isEmpty())
        m_statusLabel->setStatusText(url);
    else if (!m_isLoading)
        m_statusLabel->setStatusText(
            (m_calmController && m_calmController->isAppliedOnCurrentPage())
                ? ztr("Document: Done — Calm Mode active")
                : ztr("Document: Done"));
}

QWebEngineView *NetzacWindow::createBrowserWindow(QWebEnginePage::WebWindowType type)
{
    auto *win = new NetzacWindow(QString(), nullptr, false);
    win->setAttribute(Qt::WA_DeleteOnClose);
    if (m_calmController)
        win->setCalmModeEnabled(m_calmController->isEnabled());
    if (type == QWebEnginePage::WebDialog)
        win->resize(780, 560);
    win->show();
    win->raise();
    win->activateWindow();
    return win->webView();
}

void NetzacWindow::showWebContextMenu(const QPoint &globalPos, QWebEngineContextMenuRequest *request)
{
    QMenu menu(this);
    menu.setFont(zacosFont(QStringLiteral("ZacOS 9 System"), 12));
    QWebEnginePage *page = m_browser->page();

    if (request && request->linkUrl().isValid()) {
        const QUrl linkUrl = request->linkUrl();
        const QString linkStr = linkUrl.toString();
        QAction *openWin = menu.addAction(ztr("Open Link in New Window"));
        connect(openWin, &QAction::triggered, this, [linkStr]() {
            auto *win = new NetzacWindow(linkStr, nullptr, true);
            win->setAttribute(Qt::WA_DeleteOnClose);
            win->show();
        });
        QAction *copyLink = menu.addAction(ztr("Copy Link Location"));
        connect(copyLink, &QAction::triggered, this, [linkStr]() {
            if (QClipboard *cb = QApplication::clipboard())
                cb->setText(linkStr);
        });
        QAction *bmLink = menu.addAction(ztr("Add Bookmark for Link"));
        connect(bmLink, &QAction::triggered, this, [this, linkStr]() {
            bool exists = false;
            for (const auto &entry : m_bookmarks) {
                if (entry.second == linkStr) {
                    exists = true;
                    break;
                }
            }
            if (!exists) {
                m_bookmarks.append({linkStr, linkStr});
                BookmarkStorage::save(m_bookmarks);
            }
            m_statusLabel->setStatusText(ztr("Bookmarked: %1").arg(linkStr));
        });
        menu.addSeparator();
    }

    if (request
        && request->mediaType() == QWebEngineContextMenuRequest::MediaTypeImage
        && request->mediaUrl().isValid()) {
        const QUrl mediaUrl = request->mediaUrl();
        const QString mediaStr = mediaUrl.toString();
        QAction *viewImg = menu.addAction(ztr("View Image"));
        connect(viewImg, &QAction::triggered, this, [this, mediaStr]() {
            navigateTo(mediaStr);
        });
        QAction *copyImg = menu.addAction(ztr("Copy Image Location"));
        connect(copyImg, &QAction::triggered, this, [mediaStr]() {
            if (QClipboard *cb = QApplication::clipboard())
                cb->setText(mediaStr);
        });
        QAction *saveImg = menu.addAction(ztr("Save Image As..."));
        connect(saveImg, &QAction::triggered, this, [page, mediaUrl]() {
            page->download(mediaUrl);
        });
        menu.addSeparator();
    }

    if (request && request->isContentEditable()) {
        QAction *undoAct = menu.addAction(ztr("Undo"));
        connect(undoAct, &QAction::triggered, this, [page]() { page->triggerAction(QWebEnginePage::Undo); });
        QAction *cutAct = menu.addAction(ztr("Cut"));
        connect(cutAct, &QAction::triggered, this, [page]() { page->triggerAction(QWebEnginePage::Cut); });
        QAction *copyAct = menu.addAction(ztr("Copy"));
        connect(copyAct, &QAction::triggered, this, [page]() { page->triggerAction(QWebEnginePage::Copy); });
        QAction *pasteAct = menu.addAction(ztr("Paste"));
        connect(pasteAct, &QAction::triggered, this, [page]() { page->triggerAction(QWebEnginePage::Paste); });
        QAction *selAllAct = menu.addAction(ztr("Select All"));
        connect(selAllAct, &QAction::triggered, this, [page]() { page->triggerAction(QWebEnginePage::SelectAll); });
        menu.addSeparator();
    } else if (request && !request->selectedText().isEmpty()) {
        QAction *copySel = menu.addAction(ztr("Copy"));
        connect(copySel, &QAction::triggered, this, [page]() { page->triggerAction(QWebEnginePage::Copy); });
        menu.addSeparator();
    }

    QAction *backAct = menu.addAction(ztr("Back"));
    backAct->setEnabled(m_browser->history()->canGoBack());
    connect(backAct, &QAction::triggered, m_browser, &QWebEngineView::back);

    QAction *fwdAct = menu.addAction(ztr("Forward"));
    fwdAct->setEnabled(m_browser->history()->canGoForward());
    connect(fwdAct, &QAction::triggered, m_browser, &QWebEngineView::forward);

    QAction *reloadAct = menu.addAction(ztr("Reload"));
    connect(reloadAct, &QAction::triggered, m_browser, &QWebEngineView::reload);

    QAction *stopAct = menu.addAction(ztr("Stop"));
    stopAct->setEnabled(m_isLoading);
    connect(stopAct, &QAction::triggered, m_browser, &QWebEngineView::stop);

    menu.addSeparator();
    QAction *bmPage = menu.addAction(ztr("Add Bookmark"));
    connect(bmPage, &QAction::triggered, this, &NetzacWindow::addCurrentBookmark);
    QAction *saveAs = menu.addAction(ztr("Save As..."));
    connect(saveAs, &QAction::triggered, this, &NetzacWindow::savePage);
    QAction *srcAct = menu.addAction(ztr("Page Source"));
    connect(srcAct, &QAction::triggered, this, &NetzacWindow::viewPageSource);
    QAction *infoAct = menu.addAction(ztr("Page Info"));
    connect(infoAct, &QAction::triggered, this, &NetzacWindow::showPageInfo);

    menu.exec(globalPos);
}

void NetzacWindow::updateNavStates()
{
    QWebEngineHistory *hist = m_browser->history();
    m_btnBack->setEnabled(hist->canGoBack());
    m_btnForward->setEnabled(hist->canGoForward());
    m_btnStop->setEnabled(m_isLoading);
    m_actStopMenu->setEnabled(m_isLoading);
}

void NetzacWindow::navigateTo(const QString &url)
{
    QString raw = url.trimmed();
    if (raw.isEmpty())
        return;
    if (raw.startsWith(QLatin1String("about:"))) {
        loadBuiltinAbout(raw);
        return;
    }
    if (!raw.startsWith(QLatin1String("http://"))
        && !raw.startsWith(QLatin1String("https://"))
        && !raw.startsWith(QLatin1String("file://"))) {
        const bool isLocalhost = raw.compare(QLatin1String("localhost"), Qt::CaseInsensitive) == 0
            || raw.startsWith(QLatin1String("localhost/"), Qt::CaseInsensitive)
            || raw.startsWith(QLatin1String("localhost:"), Qt::CaseInsensitive);
        if (raw.contains(QLatin1Char(' '))
            || (!raw.contains(QLatin1Char('.')) && !raw.contains(QLatin1Char(':')) && !isLocalhost)) {
            raw = m_prefs.searchUrlFor(raw);
        } else {
            raw.prepend(QStringLiteral("https://"));
        }
    }
    m_showingBuiltin = false;
    m_page->setBuiltInPage(false);
    m_currentUrlStr = raw;
    m_browser->setUrl(QUrl(raw));
}

void NetzacWindow::buildNavHistoryMenu(QMenu &menu, bool back)
{
    QWebEngineHistory *hist = m_browser->history();
    const QList<QWebEngineHistoryItem> items = back ? hist->backItems(15) : hist->forwardItems(15);
    if (back) {
        for (int i = int(items.size()) - 1; i >= 0; --i) {
            const QWebEngineHistoryItem &item = items.at(i);
            QString label = item.title().isEmpty() ? item.url().toString() : item.title();
            if (label.size() > 48)
                label = label.left(45) + QStringLiteral("...");
            QAction *act = menu.addAction(label);
            act->setToolTip(item.url().toString());
            connect(act, &QAction::triggered, this, [hist, item]() { hist->goToItem(item); });
        }
    } else {
        for (const QWebEngineHistoryItem &item : items) {
            QString label = item.title().isEmpty() ? item.url().toString() : item.title();
            if (label.size() > 48)
                label = label.left(45) + QStringLiteral("...");
            QAction *act = menu.addAction(label);
            act->setToolTip(item.url().toString());
            connect(act, &QAction::triggered, this, [hist, item]() { hist->goToItem(item); });
        }
    }
}

void NetzacWindow::loadBuiltinAbout(const QString &aboutUrlInput)
{
    QString aboutUrl = aboutUrlInput;
    if (aboutUrl == QLatin1String("about:netscape"))
        aboutUrl = QStringLiteral("about:netzac");

    m_showingBuiltin = true;
    m_page->setBuiltInPage(true);
    m_currentUrlStr = aboutUrl;
    m_urlBar->setText(aboutUrl);
    m_locationLabel->setText(ztr("Location:"));
    m_statusSecurity->setIconName(QStringLiteral("security_open"));

    QString title;
    QString subtitle;
    QString body;

    if (aboutUrl == QLatin1String("about:netzac")) {
        title = ztr("Welcome to Netzac Navigator 4.0");
        subtitle = ztr("Communicator Platinum Edition on ZacOS 9");
        body = ztr(
            "<p>Welcome aboard <b>Netzac Navigator 4.0</b> running on <b>ZacOS 9</b>.</p>"
            "<p>Quick links to explore the classic &amp; text-friendly Web:</p>"
            "<ul>"
            "<li><a href='https://frogfind.com'>FrogFind!</a> &mdash; Classic search engine for retro browsers</li>"
            "<li><a href='https://wiby.me'>Wiby</a> &mdash; Search engine for classic web pages</li>"
            "<li><a href='http://68k.news'>68k.news</a> &mdash; Fast headlines formatted for classic Macs</li>"
            "<li><a href='https://theoldnet.com'>The Old Net</a> &mdash; Browse snapshots of the 1990s web</li>"
            "</ul>");
    } else if (aboutUrl == QLatin1String("about:mail")) {
        title = ztr("Netzac - Mail");
        subtitle = ztr("Netzac - Mail");
        body = ztr(
            "<p><b>Netzac - Mail</b></p>"
            "<p>To configure this button to open your mail link (for example Gmail or webmail), "
            "open <b>Edit &rsaquo; Preferences &rsaquo; Quick Links</b> and set the <b>Mail URL</b>.</p>");
    } else if (aboutUrl == QLatin1String("about:chat") || aboutUrl == QLatin1String("about:news")) {
        aboutUrl = QStringLiteral("about:chat");
        m_currentUrlStr = aboutUrl;
        m_urlBar->setText(aboutUrl);
        title = ztr("Netzac - Chat");
        subtitle = ztr("Netzac - Chat");
        body = ztr(
            "<p><b>Netzac - Chat</b></p>"
            "<p>To configure this button to open your chat service, "
            "open <b>Edit &rsaquo; Preferences &rsaquo; Quick Links</b> and set the <b>Chat URL</b>.</p>");
    } else if (aboutUrl == QLatin1String("about:git") || aboutUrl == QLatin1String("about:composer")) {
        aboutUrl = QStringLiteral("about:git");
        m_currentUrlStr = aboutUrl;
        m_urlBar->setText(aboutUrl);
        title = ztr("Netzac - Git");
        subtitle = ztr("Netzac - Git");
        body = ztr(
            "<p><b>Netzac - Git</b></p>"
            "<p>To configure this button to open your Git forge (for example GitHub, GitLab or Gitea), "
            "open <b>Edit &rsaquo; Preferences &rsaquo; Quick Links</b> and set the <b>Git URL</b>.</p>");
    } else if (aboutUrl == QLatin1String("about:help")) {
        title = ztr("Netzac Navigator Help");
        subtitle = ztr("Keyboard Shortcuts &amp; Toolbar Guide");
        body = ztr(
            "<ul>"
            "<li><b>Collapse Toolbars:</b> Click the ribbed grip on the left of any toolbar.</li>"
            "<li><b>Open Page:</b> <code>Ctrl+O</code></li>"
            "<li><b>Find in Page:</b> <code>Ctrl+F</code> / Find Again: <code>Ctrl+G</code></li>"
            "<li><b>Add Bookmark:</b> <code>Ctrl+D</code> / Manage: <code>Ctrl+B</code></li>"
            "<li><b>Reload / Stop:</b> <code>Ctrl+R</code> / <code>Esc</code></li>"
            "<li><b>Calm Mode:</b> <code>Ctrl+Shift+C</code></li>"
            "<li><b>Page Source / Info:</b> <code>Ctrl+U</code> / <code>Ctrl+I</code></li>"
            "</ul>");
    } else {
        title = ztr("About Netzac");
        subtitle = aboutUrl;
        body = ztr("<p>Unknown built-in Navigator location.</p>");
    }

    setWindowTitle(ztr("Netzac: %1").arg(title));
    m_browser->setHtml(retroHtmlPage(title, subtitle, body), QUrl(QStringLiteral("https://localhost/")));
}

void NetzacWindow::showBookmarksPopup()
{
    QMenu menu(this);
    populateBookmarksMenu(&menu);
    menu.exec(m_btnBookmarks->mapToGlobal(QPoint(0, m_btnBookmarks->height())));
}

void NetzacWindow::addCurrentBookmark()
{
    const QString url = m_currentUrlStr;
    const QString title = m_browser->title().isEmpty() ? url : m_browser->title();
    bool exists = false;
    for (const auto &entry : m_bookmarks) {
        if (entry.second == url) {
            exists = true;
            break;
        }
    }
    if (!exists) {
        m_bookmarks.append({title, url});
        BookmarkStorage::save(m_bookmarks);
    }
    m_statusLabel->setStatusText(ztr("Bookmarked: %1").arg(title));
}

void NetzacWindow::manageBookmarks()
{
    BookmarksDialog dlg(&m_bookmarks, this);
    connect(&dlg, &BookmarksDialog::openRequested, this, &NetzacWindow::navigateTo);
    connect(&dlg, &BookmarksDialog::bookmarksModified, this, [this]() {
        BookmarkStorage::save(m_bookmarks);
    });
    dlg.exec();
}

void NetzacWindow::newWindow()
{
    auto *win = new NetzacWindow();
    win->setAttribute(Qt::WA_DeleteOnClose);
    win->show();
}

void NetzacWindow::openLocationDialog()
{
    PlatinumDialog dlg(this);
    dlg.setWindowTitle(ztr("Open Page"));
    dlg.setFixedWidth(400);

    auto *lay = new QVBoxLayout(&dlg);
    lay->setContentsMargins(12, 12, 12, 12);
    lay->setSpacing(8);
    lay->addWidget(new QLabel(ztr("Type the Internet location (URL) or choose a local file:"), &dlg));

    auto *edit = new QLineEdit(m_currentUrlStr, &dlg);
    edit->selectAll();
    lay->addWidget(edit);

    auto *row = new QHBoxLayout();
    QPushButton *chooseBtn = makeDialogButton(ztr("Choose File..."), false, &dlg);
    connect(chooseBtn, &QPushButton::clicked, &dlg, [&dlg, edit]() {
        const QString path = QFileDialog::getOpenFileName(
            &dlg,
            ztr("Open Local HTML File"),
            QString(),
            ztr("HTML Files (*.html *.htm);;All Files (*)"));
        if (!path.isEmpty())
            edit->setText(QUrl::fromLocalFile(path).toString());
    });
    QPushButton *cancelBtn = makeDialogButton(ztr("Cancel"), false, &dlg);
    connect(cancelBtn, &QPushButton::clicked, &dlg, &QDialog::reject);
    QPushButton *okBtn = makeDialogButton(ztr("OK"), true, &dlg);
    connect(okBtn, &QPushButton::clicked, &dlg, &QDialog::accept);
    row->addWidget(chooseBtn);
    row->addStretch(1);
    row->addWidget(cancelBtn);
    row->addWidget(okBtn);
    lay->addLayout(row);

    if (dlg.exec() == QDialog::Accepted)
        navigateTo(edit->text());
}

void NetzacWindow::savePage()
{
    const QString path = QFileDialog::getSaveFileName(
        this,
        ztr("Save Page As"),
        QStringLiteral("page.html"),
        ztr("HTML Files (*.html *.htm);;All Files (*)"));
    if (!path.isEmpty()) {
        m_browser->page()->toHtml([this, path](const QString &htmlText) {
            writeSavedHtml(path, htmlText);
        });
    }
}

void NetzacWindow::writeSavedHtml(const QString &path, const QString &htmlText)
{
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        PlatinumAlertDialog(ztr("Save Error"), f.errorString(), QStringLiteral("warning"), this).exec();
        return;
    }
    QTextStream out(&f);
    out << htmlText;
    f.close();
    m_statusLabel->setStatusText(ztr("Saved: %1").arg(path));
}

void NetzacWindow::printPage()
{
    const QString path = QFileDialog::getSaveFileName(
        this,
        ztr("Print Page to PDF"),
        QStringLiteral("netzac-page.pdf"),
        ztr("PDF Files (*.pdf)"));
    if (!path.isEmpty()) {
        m_browser->page()->printToPdf(path);
        m_statusLabel->setStatusText(ztr("Printed PDF: %1").arg(path));
    }
}

void NetzacWindow::showFindBar()
{
    m_findBar->show();
    m_findInput->setFocus();
    m_findInput->selectAll();
}

void NetzacWindow::hideFindBar()
{
    m_findBar->hide();
    m_browser->findText(QString());
}

void NetzacWindow::findNext()
{
    const QString query = m_findInput->text();
    if (!query.isEmpty()) {
        m_lastSearch = query;
        m_browser->findText(query);
    } else if (!m_lastSearch.isEmpty()) {
        m_browser->findText(m_lastSearch);
    }
}

void NetzacWindow::viewPageSource()
{
    m_browser->page()->toHtml([this](const QString &sourceText) {
        PlatinumDialog dlg(this);
        dlg.setWindowTitle(ztr("Source of: %1").arg(m_currentUrlStr));
        dlg.resize(680, 480);

        auto *lay = new QVBoxLayout(&dlg);
        lay->setContentsMargins(10, 10, 10, 10);
        lay->setSpacing(8);

        auto *view = new QPlainTextEdit(&dlg);
        view->setReadOnly(true);
        view->setFont(QFont(QStringLiteral("Nimbus Mono PS"), 11));
        view->setPlainText(sourceText);
        lay->addWidget(view, 1);

        auto *btnRow = new QHBoxLayout();
        btnRow->addStretch(1);
        QPushButton *closeBtn = makeDialogButton(ztr("Close"), true, &dlg);
        connect(closeBtn, &QPushButton::clicked, &dlg, &QDialog::accept);
        btnRow->addWidget(closeBtn);
        lay->addLayout(btnRow);
        dlg.exec();
    });
}

void NetzacWindow::showPageInfo()
{
    const QString url = m_currentUrlStr;
    const QString title = m_browser->title().isEmpty() ? ztr("(Untitled)") : m_browser->title();
    QString sec;
    if (url.startsWith(QLatin1String("https://")))
        sec = ztr("Encrypted connection (HTTPS) — Security padlock is locked.");
    else if (url.startsWith(QLatin1String("about:")))
        sec = ztr("Built-in Netzac Navigator local document.");
    else
        sec = ztr("Unencrypted connection (HTTP) — Information sent over this connection is not encrypted.");

    PlatinumAlertDialog(
        ztr("Page & Security Info"),
        ztr("Title: %1\nLocation: %2\n\nSecurity: %3").arg(title, url, sec),
        QStringLiteral("info"),
        this)
        .exec();
}

void NetzacWindow::showHistoryDialog()
{
    PlatinumDialog dlg(this);
    dlg.setWindowTitle(ztr("History"));
    dlg.resize(560, 380);

    auto *lay = new QVBoxLayout(&dlg);
    lay->setContentsMargins(10, 10, 10, 10);
    lay->setSpacing(8);

    auto *searchRow = new QHBoxLayout();
    searchRow->addWidget(new QLabel(ztr("Search:"), &dlg));
    auto *filterEdit = new QLineEdit(&dlg);
    filterEdit->setPlaceholderText(ztr("Filter by title or URL..."));
    filterEdit->setFont(zacosFont(QStringLiteral("ZacOS 9 System"), 12));
    searchRow->addWidget(filterEdit, 1);
    lay->addLayout(searchRow);

    auto *lst = new QListWidget(&dlg);
    const auto populateList = [this, lst](const QString &query) {
        lst->clear();
        const QString q = query.trimmed();
        for (const HistoryEntry &entry : m_history) {
            const QString title = entry.title.isEmpty() ? ztr("(Untitled)") : entry.title;
            if (!q.isEmpty()
                && !title.contains(q, Qt::CaseInsensitive)
                && !entry.url.contains(q, Qt::CaseInsensitive)) {
                continue;
            }
            QString dateStr;
            if (entry.visitedAt > 0) {
                dateStr = QDateTime::fromSecsSinceEpoch(entry.visitedAt)
                              .toString(QStringLiteral("yyyy-MM-dd hh:mm"));
            }
            const QString label = dateStr.isEmpty()
                ? QStringLiteral("%1  —  %2").arg(title, entry.url)
                : QStringLiteral("[%1]  %2  —  %3").arg(dateStr, title, entry.url);
            auto *rowItem = new QListWidgetItem(label);
            rowItem->setData(Qt::UserRole, entry.url);
            lst->addItem(rowItem);
        }
    };
    populateList(QString());

    connect(filterEdit, &QLineEdit::textChanged, &dlg, populateList);
    connect(lst, &QListWidget::itemDoubleClicked, &dlg, [this, &dlg](QListWidgetItem *item) {
        if (item) {
            navigateTo(item->data(Qt::UserRole).toString());
            dlg.accept();
        }
    });
    lay->addWidget(lst, 1);

    auto *btnRow = new QHBoxLayout();
    QPushButton *openBtn = makeDialogButton(ztr("Open"), false, &dlg);
    connect(openBtn, &QPushButton::clicked, &dlg, [this, &dlg, lst]() {
        if (QListWidgetItem *item = lst->currentItem()) {
            navigateTo(item->data(Qt::UserRole).toString());
            dlg.accept();
        }
    });
    QPushButton *clearBtn = makeDialogButton(ztr("Clear History"), false, &dlg);
    connect(clearBtn, &QPushButton::clicked, &dlg, [this, populateList, filterEdit]() {
        m_history.clear();
        HistoryStorage::save(m_history);
        populateList(filterEdit->text());
    });
    btnRow->addWidget(openBtn);
    btnRow->addWidget(clearBtn);
    btnRow->addStretch(1);
    QPushButton *closeBtn = makeDialogButton(ztr("Close"), true, &dlg);
    connect(closeBtn, &QPushButton::clicked, &dlg, &QDialog::accept);
    btnRow->addWidget(closeBtn);
    lay->addLayout(btnRow);
    dlg.exec();
}

void NetzacWindow::openPreferences()
{
    PreferencesDialog dlg(m_prefs, m_currentUrlStr, this);
    if (dlg.exec() == QDialog::Accepted) {
        m_prefs = dlg.preferences();
        m_prefs.save();
        m_prefs.apply(m_browser->page()->profile());
        m_browser->page()->settings()->setAttribute(
            QWebEngineSettings::AutoLoadImages, m_prefs.autoloadImages);
        if (m_calmController)
            m_calmController->setConfig(m_prefs.calm);
        rebuildPersonalToolbar();
    }
}

void NetzacWindow::showAbout()
{
    AboutDialog(this).exec();
}
