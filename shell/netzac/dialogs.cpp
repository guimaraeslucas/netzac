#include "dialogs.h"

#include "adblocker.h"
#include "bookmarks.h"
#include "i18n.h"
#include "netzacwindow.h"
#include "platinum.h"
#include "widgets.h"

#include <QApplication>
#include <QButtonGroup>
#include <QCheckBox>
#include <QClipboard>
#include <QComboBox>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDatabase>
#include <QFontMetrics>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QRegularExpression>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPainter>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QSizePolicy>
#include <QSpinBox>
#include <QSsl>
#include <QSslCertificate>
#include <QSslCertificateExtension>
#include <QSslCipher>
#include <QSslError>
#include <QSslKey>
#include <QSslSocket>
#include <QStackedWidget>
#include <QStandardPaths>
#include <QUrl>
#include <QVBoxLayout>
#include <QWebEngineCookieStore>
#include <QWebEngineDownloadRequest>
#include <QWebEngineProfile>

QPushButton *makeDialogButton(const QString &text, bool isDefault, QWidget *parent)
{
    auto *btn = new QPushButton(text, parent);
    btn->setIcon(QIcon());
    btn->setFont(zacosFont(QStringLiteral("ZacOS 9 System"), 12));
    btn->setMinimumWidth(78);
    btn->setFixedHeight(isDefault ? 28 : 24);
    btn->setAutoDefault(isDefault);
    btn->setDefault(isDefault);
    return btn;
}

namespace {

QString runPlatinumFileDialog(
    QWidget *parent,
    const QString &caption,
    const QString &dirOrFile,
    const QString &filter,
    bool saveMode)
{
    QFileDialog dlg(parent, caption);
    dlg.setOption(QFileDialog::DontUseNativeDialog, true);
    dlg.setAcceptMode(saveMode ? QFileDialog::AcceptSave : QFileDialog::AcceptOpen);
    dlg.setFileMode(saveMode ? QFileDialog::AnyFile : QFileDialog::ExistingFile);
    if (!filter.isEmpty())
        dlg.setNameFilter(filter);
    if (!dirOrFile.isEmpty()) {
        const QFileInfo fi(dirOrFile);
        if (fi.isDir()) {
            dlg.setDirectory(fi.absoluteFilePath());
        } else {
            if (!fi.path().isEmpty() && fi.path() != QLatin1String("."))
                dlg.setDirectory(fi.path());
            dlg.selectFile(fi.fileName());
        }
    }
    if (QApplication *app = qobject_cast<QApplication *>(QApplication::instance())) {
        dlg.setStyle(app->style());
        dlg.setPalette(app->palette());
    }
    dlg.setFont(zacosFont(QStringLiteral("ZacOS 9 System"), 12));
    if (dlg.exec() != QDialog::Accepted)
        return QString();
    const QStringList files = dlg.selectedFiles();
    return files.isEmpty() ? QString() : files.first();
}

}  // namespace

QString platinumOpenFileName(
    QWidget *parent,
    const QString &caption,
    const QString &dir,
    const QString &filter)
{
    return runPlatinumFileDialog(parent, caption, dir, filter, false);
}

QString platinumSaveFileName(
    QWidget *parent,
    const QString &caption,
    const QString &dirOrFile,
    const QString &filter)
{
    return runPlatinumFileDialog(parent, caption, dirOrFile, filter, true);
}

PlatinumDialog::PlatinumDialog(QWidget *parent)
    : QDialog(parent)
{
    if (QApplication *app = qobject_cast<QApplication *>(QApplication::instance())) {
        setStyle(app->style());
        setPalette(app->palette());
    }
    setFont(zacosFont(QStringLiteral("ZacOS 9 System"), 12));
}

PlatinumAlertDialog::PlatinumAlertDialog(
    const QString &title,
    const QString &message,
    const QString &kind,
    QWidget *parent)
    : PlatinumDialog(parent)
{
    setWindowTitle(title);
    setMinimumWidth(400);

    auto *lay = new QVBoxLayout(this);
    lay->setContentsMargins(14, 14, 14, 12);
    lay->setSpacing(12);

    auto *top = new QHBoxLayout();
    top->setSpacing(12);
    auto *iconLbl = new QLabel(this);
    iconLbl->setPixmap(makeAlertIcon(kind, 32));
    iconLbl->setFixedSize(32, 32);
    top->addWidget(iconLbl, 0, Qt::AlignTop);

    auto *msgLbl = new QLabel(message, this);
    msgLbl->setFont(zacosFont(QStringLiteral("ZacOS 9 System"), 12));
    msgLbl->setWordWrap(true);
    msgLbl->setMinimumWidth(320);
    msgLbl->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    top->addWidget(msgLbl, 1);
    lay->addLayout(top);

    auto *btnRow = new QHBoxLayout();
    btnRow->addStretch(1);
    QPushButton *okBtn = makeDialogButton(ztr("OK"), true, this);
    connect(okBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnRow->addWidget(okBtn);
    lay->addLayout(btnRow);
    adjustSize();
}

AboutDialog::AboutDialog(QWidget *parent)
    : PlatinumDialog(parent)
{
    setWindowTitle(ztr("About Netzac"));
    setFixedSize(460, 310);

    auto *lay = new QVBoxLayout(this);
    lay->setContentsMargins(14, 14, 14, 12);
    lay->setSpacing(10);

    auto *top = new QHBoxLayout();
    top->setSpacing(12);
    auto *badge = new NetzacThrobber(this);
    badge->setEnabled(false);
    badge->setFixedSize(56, 56);
    top->addWidget(badge, 0, Qt::AlignTop);

    auto *info = new QVBoxLayout();
    auto *title = new QLabel(ztr("Netzac Navigator"), this);
    title->setFont(zacosFont(QStringLiteral("ZacOS 9 System"), 12, true));
    const QString repoUrl = QStringLiteral("https://github.com/guimaraeslucas/netzac");
    const QString rawAbout = ztr(
        "Netzac Navigator (ZacOS 9)\n\n"
        "Developed by Lucas Guimarães - G3Pix (@guimaraeslucas). We count on the support of the "
        "ZacOS 9 community for contributions at:\n"
        "%1\n\n"
        "Special thanks to Adam Lawson (@chromasystemprofessional) for creating "
        "the incredible ZacOS 9 project.");
    const QString htmlAbout = rawAbout.toHtmlEscaped()
        .replace(QLatin1Char('\n'), QStringLiteral("<br>"))
        .arg(QStringLiteral("<a href=\"%1\">%1</a>").arg(repoUrl));
    auto *sub = new QLabel(htmlAbout, this);
    sub->setFont(zacosFont(QStringLiteral("ZacOS 9 Views"), 9));
    sub->setTextFormat(Qt::RichText);
    sub->setWordWrap(true);
    sub->setTextInteractionFlags(Qt::TextBrowserInteraction);
    sub->setOpenExternalLinks(false);
    connect(sub, &QLabel::linkActivated, this, [this](const QString &url) {
        if (auto *win = qobject_cast<NetzacWindow *>(parentWidget())) {
            win->newTab(url, true);
            accept();
        } else {
            QDesktopServices::openUrl(QUrl(url));
        }
    });
    info->addWidget(title);
    info->addWidget(sub);
    top->addLayout(info, 1);
    lay->addLayout(top);

    auto *line = new QFrame(this);
    line->setFrameShape(QFrame::HLine);
    lay->addWidget(line);

    auto *btnRow = new QHBoxLayout();
    btnRow->addStretch(1);
    QPushButton *okBtn = makeDialogButton(ztr("OK"), true, this);
    connect(okBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnRow->addWidget(okBtn);
    lay->addLayout(btnRow);
}

BookmarksDialog::BookmarksDialog(QVector<QPair<QString, QString>> *bookmarks, QWidget *parent)
    : PlatinumDialog(parent)
    , m_bookmarks(bookmarks)
{
    setWindowTitle(ztr("Bookmarks"));
    resize(440, 300);

    auto *lay = new QVBoxLayout(this);
    lay->setContentsMargins(10, 10, 10, 10);
    lay->setSpacing(8);

    m_list = new QListWidget(this);
    refresh();
    connect(m_list, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *) {
        openSelected();
    });
    lay->addWidget(m_list, 1);

    auto *row = new QHBoxLayout();
    QPushButton *openBtn = makeDialogButton(ztr("Open"), false, this);
    connect(openBtn, &QPushButton::clicked, this, &BookmarksDialog::openSelected);
    QPushButton *removeBtn = makeDialogButton(ztr("Delete"), false, this);
    connect(removeBtn, &QPushButton::clicked, this, &BookmarksDialog::deleteSelected);
    QPushButton *importBtn = makeDialogButton(ztr("Import..."), false, this);
    connect(importBtn, &QPushButton::clicked, this, &BookmarksDialog::importBookmarks);
    QPushButton *exportBtn = makeDialogButton(ztr("Export..."), false, this);
    connect(exportBtn, &QPushButton::clicked, this, &BookmarksDialog::exportBookmarks);
    QPushButton *closeBtn = makeDialogButton(ztr("Close"), true, this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    row->addWidget(openBtn);
    row->addWidget(removeBtn);
    row->addWidget(importBtn);
    row->addWidget(exportBtn);
    row->addStretch(1);
    row->addWidget(closeBtn);
    lay->addLayout(row);
}

void BookmarksDialog::refresh()
{
    m_list->clear();
    if (!m_bookmarks)
        return;
    for (const auto &entry : *m_bookmarks) {
        auto *item = new QListWidgetItem(
            QStringLiteral("%1  —  %2").arg(entry.first, entry.second));
        item->setData(Qt::UserRole, entry.second);
        m_list->addItem(item);
    }
}

void BookmarksDialog::openSelected()
{
    QListWidgetItem *item = m_list->currentItem();
    if (item) {
        emit openRequested(item->data(Qt::UserRole).toString());
        accept();
    }
}

void BookmarksDialog::deleteSelected()
{
    if (!m_bookmarks)
        return;
    const int idx = m_list->currentRow();
    if (idx >= 0 && idx < m_bookmarks->size()) {
        m_bookmarks->removeAt(idx);
        refresh();
        emit bookmarksModified();
    }
}

void BookmarksDialog::importBookmarks()
{
    if (!m_bookmarks)
        return;
    const QString path = platinumOpenFileName(
        this,
        ztr("Import Bookmarks (Chrome / Firefox)"),
        QString(),
        ztr("Bookmark Files (*.html *.htm Bookmarks *.json);;All Files (*)"));
    if (path.isEmpty())
        return;

    bool ok = false;
    const BookmarkStorage::BookmarkList imported = BookmarkStorage::importFromFile(path, &ok);
    if (!ok || imported.isEmpty()) {
        PlatinumAlertDialog(
            ztr("Import Bookmarks"),
            ztr("No bookmarks could be imported from the selected file."),
            QStringLiteral("warning"),
            this)
            .exec();
        return;
    }

    int added = 0;
    for (const auto &entry : imported) {
        bool exists = false;
        for (const auto &cur : *m_bookmarks) {
            if (cur.second == entry.second) {
                exists = true;
                break;
            }
        }
        if (!exists) {
            m_bookmarks->append(entry);
            ++added;
        }
    }
    refresh();
    if (added > 0)
        emit bookmarksModified();

    PlatinumAlertDialog(
        ztr("Import Bookmarks"),
        ztr("Imported %1 new bookmark(s).").arg(added),
        QStringLiteral("info"),
        this)
        .exec();
}

void BookmarksDialog::exportBookmarks()
{
    if (!m_bookmarks)
        return;
    const QString path = platinumSaveFileName(
        this,
        ztr("Export Bookmarks (Chrome / Firefox HTML)"),
        QStringLiteral("netzac-bookmarks.html"),
        ztr("HTML Bookmarks (*.html *.htm);;All Files (*)"));
    if (path.isEmpty())
        return;

    if (!BookmarkStorage::exportToHtmlFile(*m_bookmarks, path)) {
        PlatinumAlertDialog(
            ztr("Export Bookmarks"),
            ztr("Could not write bookmarks to the selected file."),
            QStringLiteral("warning"),
            this)
            .exec();
    } else {
        PlatinumAlertDialog(
            ztr("Export Bookmarks"),
            ztr("Bookmarks exported in Chrome/Firefox HTML format."),
            QStringLiteral("info"),
            this)
            .exec();
    }
}

PreferencesDialog::PreferencesDialog(
    const NetzacPreferences &prefs,
    const QString &currentUrl,
    QWidget *parent)
    : PlatinumDialog(parent)
    , m_prefs(prefs)
{
    setWindowTitle(ztr("Preferences"));
    setMinimumSize(620, 520);
    resize(660, 550);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(12, 12, 12, 12);
    root->setSpacing(10);

    auto *mainRow = new QHBoxLayout();
    mainRow->setSpacing(10);

    // Left Category pane (classic Netscape Navigator 4.0)
    auto *leftCol = new QVBoxLayout();
    leftCol->setSpacing(4);
    auto *catLbl = new QLabel(ztr("Category:"), this);
    catLbl->setFont(zacosFont(QStringLiteral("ZacOS 9 System"), 12, true));
    leftCol->addWidget(catLbl);

    m_categoryList = new QListWidget(this);
    m_categoryList->setFixedWidth(155);
    m_categoryList->addItem(ztr("Navigator"));
    m_categoryList->addItem(ztr("Appearance"));
    m_categoryList->addItem(ztr("Calm Mode"));
    m_categoryList->addItem(ztr("Personal Toolbar"));
    m_categoryList->addItem(ztr("Quick Links"));
    m_categoryList->addItem(ztr("Cookies"));
    m_categoryList->addItem(ztr("Cache"));
    m_categoryList->addItem(ztr("Proxies"));
    m_categoryList->addItem(ztr("User Agent"));
    m_categoryList->addItem(ztr("Ad Blocker"));
    leftCol->addWidget(m_categoryList, 1);
    mainRow->addLayout(leftCol);

    // Right Category Detail pane
    auto *rightCol = new QVBoxLayout();
    rightCol->setSpacing(8);

    auto *hdrFrame = new QFrame(this);
    hdrFrame->setFrameStyle(QFrame::Box | QFrame::Sunken);
    auto *hdrLay = new QHBoxLayout(hdrFrame);
    hdrLay->setContentsMargins(8, 4, 8, 4);
    m_pageHeader = new QLabel(ztr("Navigator"), hdrFrame);
    m_pageHeader->setFont(zacosFont(QStringLiteral("ZacOS 9 System"), 12, true));
    hdrLay->addWidget(m_pageHeader);
    rightCol->addWidget(hdrFrame);

    m_stack = new QStackedWidget(this);
    m_stack->addWidget(buildNavigatorPage(currentUrl));
    m_stack->addWidget(buildAppearancePage());
    m_stack->addWidget(buildCalmModePage());
    m_stack->addWidget(buildPersonalToolbarPage());
    m_stack->addWidget(buildQuickLinksPage());
    m_stack->addWidget(buildCookiesPage());
    m_stack->addWidget(buildCachePage());
    m_stack->addWidget(buildProxiesPage());
    m_stack->addWidget(buildUserAgentPage());
    m_stack->addWidget(buildAdBlockerPage());
    rightCol->addWidget(m_stack, 1);
    mainRow->addLayout(rightCol, 1);

    root->addLayout(mainRow, 1);

    auto *sep = new QFrame(this);
    sep->setFrameShape(QFrame::HLine);
    root->addWidget(sep);

    auto *btnRow = new QHBoxLayout();
    btnRow->addStretch(1);
    QPushButton *cancelBtn = makeDialogButton(ztr("Cancel"), false, this);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    QPushButton *okBtn = makeDialogButton(ztr("OK"), true, this);
    connect(okBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnRow->addWidget(cancelBtn);
    btnRow->addWidget(okBtn);
    root->addLayout(btnRow);

    connect(m_categoryList, &QListWidget::currentRowChanged, this, [this](int row) {
        if (row >= 0 && row < m_stack->count()) {
            m_stack->setCurrentIndex(row);
            if (QListWidgetItem *item = m_categoryList->item(row))
                m_pageHeader->setText(item->text());
        }
    });
    m_categoryList->setCurrentRow(0);
}

void PreferencesDialog::selectCategory(int index)
{
    if (index >= 0 && index < m_categoryList->count())
        m_categoryList->setCurrentRow(index);
}

QWidget *PreferencesDialog::buildNavigatorPage(const QString &currentUrl)
{
    auto *page = new QWidget(this);
    auto *lay = new QVBoxLayout(page);
    lay->setContentsMargins(6, 4, 6, 4);
    lay->setSpacing(8);

    auto *homeLbl = new QLabel(ztr("Navigator Home Page Location:"), page);
    lay->addWidget(homeLbl);

    m_homeEdit = new QLineEdit(m_prefs.homeUrl, page);
    m_homeEdit->setFont(zacosFont(QStringLiteral("ZacOS 9 System"), 12));
    lay->addWidget(m_homeEdit);

    auto *homeBtnRow = new QHBoxLayout();
    QPushButton *useCurBtn = makeDialogButton(ztr("Use Current Page"), false, page);
    useCurBtn->setEnabled(!currentUrl.isEmpty());
    connect(useCurBtn, &QPushButton::clicked, this, [this, currentUrl]() {
        if (!currentUrl.isEmpty())
            m_homeEdit->setText(currentUrl);
    });
    homeBtnRow->addWidget(useCurBtn);
    homeBtnRow->addStretch(1);
    lay->addLayout(homeBtnRow);

    lay->addSpacing(4);
    lay->addWidget(new QLabel(ztr("When Netzac Starts:"), page));
    m_startupPopup = new PlatinumPopupButton(page);
    m_startupPopup->addItem(ztr("Open Home Page"), int(NetzacPreferences::StartupMode::HomePage));
    m_startupPopup->addItem(
        ztr("Continue where I left off"),
        int(NetzacPreferences::StartupMode::RestoreLastPage));
    m_startupPopup->setCurrentData(int(m_prefs.startupMode));
    lay->addWidget(m_startupPopup);

    lay->addSpacing(4);
    auto *searchLbl = new QLabel(ztr("Default Search Engine:"), page);
    lay->addWidget(searchLbl);

    m_searchPopup = new PlatinumPopupButton(page);
    for (const auto &entry : NetzacPreferences::searchEnginePresets())
        m_searchPopup->addItem(entry.label, QString::fromLatin1(entry.id));
    m_searchPopup->setCurrentData(m_prefs.searchEngineId);
    lay->addWidget(m_searchPopup);

    lay->addSpacing(6);
    m_imagesChk = new QCheckBox(ztr("Automatically load images"), page);
    m_imagesChk->setChecked(m_prefs.autoloadImages);
    lay->addWidget(m_imagesChk);

    m_jsChk = new QCheckBox(ztr("Enable JavaScript"), page);
    m_jsChk->setChecked(m_prefs.enableJavaScript);
    lay->addWidget(m_jsChk);

    m_rememberPasswordsChk = new QCheckBox(ztr("Remember passwords (ZacOS Keychain)"), page);
    m_rememberPasswordsChk->setChecked(m_prefs.rememberPasswords);
    m_rememberPasswordsChk->setEnabled(PasswordStorage::isAvailable());
    lay->addWidget(m_rememberPasswordsChk);

    m_disableGpuChk = new QCheckBox(
        ztr("Disable GPU hardware acceleration (compatibility mode, restart required)"),
        page);
    m_disableGpuChk->setChecked(m_prefs.disableGpuAcceleration);
    m_disableGpuChk->setToolTip(
        ztr("Enable only on incompatible virtual machines or drivers. "
            "Keeping GPU acceleration enabled provides faster web rendering."));
    lay->addWidget(m_disableGpuChk);

    lay->addSpacing(8);
    lay->addWidget(new QLabel(ztr("Default Browser:"), page));
    m_defaultBrowserLbl = new QLabel(page);
    lay->addWidget(m_defaultBrowserLbl);

    auto *defBtnRow = new QHBoxLayout();
    m_setDefaultBrowserBtn = makeDialogButton(ztr("Set as Default Browser"), false, page);
    m_setDefaultBrowserBtn->setMinimumWidth(240);
    const auto refreshDefaultStatus = [this]() {
        const bool isDef = NetzacPreferences::isDefaultBrowser();
        m_defaultBrowserLbl->setText(
            isDef
                ? ztr("Netzac is currently your default browser.")
                : ztr("Netzac is not your default browser."));
        m_setDefaultBrowserBtn->setEnabled(!isDef);
    };
    refreshDefaultStatus();
    connect(m_setDefaultBrowserBtn, &QPushButton::clicked, this, [refreshDefaultStatus]() {
        NetzacPreferences::setAsDefaultBrowser();
        refreshDefaultStatus();
    });
    defBtnRow->addWidget(m_setDefaultBrowserBtn);
    defBtnRow->addStretch(1);
    lay->addLayout(defBtnRow);

    lay->addSpacing(10);
    auto *resetRow = new QHBoxLayout();
    QPushButton *resetBtn = makeDialogButton(ztr("Reset All Settings..."), false, page);
    resetBtn->setMinimumWidth(200);
    connect(resetBtn, &QPushButton::clicked, this, [this]() {
        PlatinumDialog confirm(this);
        confirm.setWindowTitle(ztr("Reset All Settings"));
        confirm.setFixedWidth(400);
        auto *cLay = new QVBoxLayout(&confirm);
        cLay->setContentsMargins(12, 12, 12, 12);
        cLay->setSpacing(10);

        auto *top = new QHBoxLayout();
        auto *iconLbl = new QLabel(&confirm);
        iconLbl->setPixmap(makeAlertIcon(QStringLiteral("warning"), 32));
        iconLbl->setAlignment(Qt::AlignTop);
        auto *msgLbl = new QLabel(
            ztr("This will reset all preferences, bookmarks, history, Personal Toolbar, "
                "and saved Netzac passwords back to their defaults.\n\n"
                "Do you want to continue?"),
            &confirm);
        msgLbl->setWordWrap(true);
        top->addWidget(iconLbl);
        top->addWidget(msgLbl, 1);
        cLay->addLayout(top);

        auto *btnRow = new QHBoxLayout();
        btnRow->addStretch(1);
        QPushButton *cancelBtn = makeDialogButton(ztr("Cancel"), true, &confirm);
        QPushButton *doResetBtn = makeDialogButton(ztr("Reset Everything"), false, &confirm);
        doResetBtn->setMinimumWidth(140);
        connect(cancelBtn, &QPushButton::clicked, &confirm, &QDialog::reject);
        connect(doResetBtn, &QPushButton::clicked, &confirm, &QDialog::accept);
        btnRow->addWidget(cancelBtn);
        btnRow->addWidget(doResetBtn);
        cLay->addLayout(btnRow);

        if (confirm.exec() == QDialog::Accepted) {
            NetzacPreferences::resetAll();
            m_prefs = NetzacPreferences::defaults();
            m_resetRequested = true;
            accept();
        }
    });
    resetRow->addWidget(resetBtn);
    resetRow->addStretch(1);
    lay->addLayout(resetRow);

    lay->addStretch(1);
    return page;
}

class AccentSwatchWidget : public QWidget {
public:
    explicit AccentSwatchWidget(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setFixedSize(22, 22);
    }

    void setAccentHex(const QString &hex)
    {
        m_color = hex.isEmpty() ? resolveSystemAccentColor() : QColor(hex);
        update();
    }

protected:
    void paintEvent(QPaintEvent * /*event*/) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, false);
        const QColor fill = isEnabled() ? m_color : palette().color(QPalette::Window);
        p.fillRect(rect().adjusted(1, 1, -1, -1), fill);
        p.setPen(palette().color(isEnabled() ? QPalette::Shadow : QPalette::Mid));
        p.drawRect(0, 0, width() - 1, height() - 1);
        if (isEnabled()) {
            p.setPen(fill.lighter(125));
            p.drawLine(1, 1, width() - 3, 1);
            p.drawLine(1, 1, 1, height() - 3);
            p.setPen(fill.darker(135));
            p.drawLine(2, height() - 2, width() - 2, height() - 2);
            p.drawLine(width() - 2, 2, width() - 2, height() - 2);
        }
    }

private:
    static QColor resolveSystemAccentColor()
    {
        QString configRoot = qEnvironmentVariable("XDG_CONFIG_HOME");
        if (configRoot.isEmpty())
            configRoot = QDir::homePath() + QStringLiteral("/.config");

        QString accentId = QStringLiteral("lavender");
        QFile desktopConf(configRoot + QStringLiteral("/zacos9/desktop.conf"));
        if (desktopConf.open(QIODevice::ReadOnly | QIODevice::Text)) {
            while (!desktopConf.atEnd()) {
                const QString line = QString::fromUtf8(desktopConf.readLine()).trimmed();
                if (line.startsWith(QLatin1String("accent="))) {
                    accentId = line.mid(7).trimmed().toLower();
                    break;
                }
            }
        }
        for (const auto &entry : NetzacPreferences::scrollbarAccentPresets()) {
            if (*entry.swatchHex && accentId == QLatin1String(entry.id))
                return QColor(QString::fromLatin1(entry.swatchHex));
        }
        return QColor(QStringLiteral("#9999ff"));
    }

    QColor m_color = QColor(QStringLiteral("#9999ff"));
};

namespace {

QComboBox *makeFontCombo(const QString &currentFamily, bool preferMonospace, QWidget *parent)
{
    auto *combo = new QComboBox(parent);
    combo->setFont(zacosFont(QStringLiteral("ZacOS 9 System"), 12));
    combo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    combo->addItem(ztr("(Default)"), QString());

    const QStringList allFamilies = QFontDatabase::families();
    QStringList monoFamilies;
    QStringList otherFamilies;
    for (const QString &fam : allFamilies) {
        if (fam.startsWith(QLatin1Char('@')) || QFontDatabase::isPrivateFamily(fam))
            continue;
        if (preferMonospace && QFontDatabase::isFixedPitch(fam))
            monoFamilies.append(fam);
        else
            otherFamilies.append(fam);
    }

    if (preferMonospace && !monoFamilies.isEmpty()) {
        for (const QString &fam : monoFamilies)
            combo->addItem(fam, fam);
        if (!otherFamilies.isEmpty())
            combo->insertSeparator(combo->count());
    }
    for (const QString &fam : otherFamilies)
        combo->addItem(fam, fam);

    if (!currentFamily.isEmpty()) {
        const int idx = combo->findData(currentFamily);
        if (idx >= 0) {
            combo->setCurrentIndex(idx);
        } else {
            combo->addItem(currentFamily, currentFamily);
            combo->setCurrentIndex(combo->count() - 1);
        }
    }
    return combo;
}

}  // namespace

QWidget *PreferencesDialog::buildAppearancePage()
{
    auto *page = new QWidget(this);
    auto *lay = new QVBoxLayout(page);
    lay->setContentsMargins(6, 4, 6, 4);
    lay->setSpacing(6);

    auto *themeHdr = new QLabel(ztr("Theme:"), page);
    themeHdr->setFont(zacosFont(QStringLiteral("ZacOS 9 System"), 12, true));
    lay->addWidget(themeHdr);

    auto *themeRow = new QHBoxLayout();
    themeRow->setContentsMargins(20, 0, 0, 0);
    themeRow->setSpacing(8);
    themeRow->addWidget(new QLabel(ztr("Application Theme:"), page));
    m_uiThemePopup = new PlatinumPopupButton(page);
    m_uiThemePopup->setMinimumWidth(180);
    if (hasZacos9Style()) {
        m_uiThemePopup->addItem(ztr("ZacOS 9 Default"), int(NetzacPreferences::UiTheme::ZacosDefault));
        m_uiThemePopup->addItem(ztr("Light"), int(NetzacPreferences::UiTheme::Light));
        m_uiThemePopup->addItem(ztr("Dark"), int(NetzacPreferences::UiTheme::Dark));
        m_uiThemePopup->setCurrentData(int(m_prefs.uiTheme));
    } else {
        m_uiThemePopup->addItem(ztr("Light"), int(NetzacPreferences::UiTheme::Light));
        m_uiThemePopup->addItem(ztr("Dark"), int(NetzacPreferences::UiTheme::Dark));
        const auto effective = (m_prefs.uiTheme == NetzacPreferences::UiTheme::Dark)
            ? NetzacPreferences::UiTheme::Dark
            : NetzacPreferences::UiTheme::Light;
        m_uiThemePopup->setCurrentData(int(effective));
    }
    themeRow->addWidget(m_uiThemePopup);
    themeRow->addStretch(1);
    lay->addLayout(themeRow);

    auto *themeSep = new QFrame(page);
    themeSep->setFrameShape(QFrame::HLine);
    lay->addWidget(themeSep);

    auto *sbHdr = new QLabel(ztr("Scrollbars:"), page);
    sbHdr->setFont(zacosFont(QStringLiteral("ZacOS 9 System"), 12, true));
    lay->addWidget(sbHdr);

    m_scrollbarsChk = new QCheckBox(ztr("Use Platinum CSS scrollbars on web pages"), page);
    m_scrollbarsChk->setChecked(m_prefs.enablePlatinumScrollbars);
    lay->addWidget(m_scrollbarsChk);

    auto *accentRow = new QHBoxLayout();
    accentRow->setContentsMargins(20, 0, 0, 0);
    accentRow->setSpacing(8);
    m_scrollbarAccentLbl = new QLabel(ztr("Scrollbar Accent Color:"), page);
    accentRow->addWidget(m_scrollbarAccentLbl);

    m_scrollbarAccentPopup = new PlatinumPopupButton(page);
    m_scrollbarAccentPopup->setMinimumWidth(180);
    for (const auto &entry : NetzacPreferences::scrollbarAccentPresets())
        m_scrollbarAccentPopup->addItem(entry.label, QString::fromLatin1(entry.id));
    m_scrollbarAccentPopup->setCurrentData(m_prefs.scrollbarAccent);
    accentRow->addWidget(m_scrollbarAccentPopup);

    m_accentSwatch = new AccentSwatchWidget(page);
    accentRow->addWidget(m_accentSwatch);
    accentRow->addStretch(1);
    lay->addLayout(accentRow);

    const auto updateAccentSwatch = [this]() {
        const QString id = m_scrollbarAccentPopup->currentData().toString();
        QString hex;
        for (const auto &entry : NetzacPreferences::scrollbarAccentPresets()) {
            if (id == QLatin1String(entry.id)) {
                hex = QString::fromLatin1(entry.swatchHex);
                break;
            }
        }
        m_accentSwatch->setAccentHex(hex);
    };
    connect(m_scrollbarAccentPopup, &PlatinumPopupButton::currentIndexChanged, this, updateAccentSwatch);
    updateAccentSwatch();

    const auto syncScrollbarsEnabled = [this](bool enabled) {
        m_scrollbarAccentLbl->setEnabled(enabled);
        m_scrollbarAccentPopup->setEnabled(enabled);
        m_accentSwatch->setEnabled(enabled);
    };
    connect(m_scrollbarsChk, &QCheckBox::toggled, this, syncScrollbarsEnabled);
    syncScrollbarsEnabled(m_scrollbarsChk->isChecked());

    auto *sep = new QFrame(page);
    sep->setFrameShape(QFrame::HLine);
    lay->addWidget(sep);

    auto *fontHdr = new QLabel(ztr("Web Page Fonts:"), page);
    fontHdr->setFont(zacosFont(QStringLiteral("ZacOS 9 System"), 12, true));
    lay->addWidget(fontHdr);

    m_forceFontsChk = new QCheckBox(
        ztr("Use custom fonts and force web pages to render with them"),
        page);
    m_forceFontsChk->setChecked(m_prefs.forceCustomFonts);
    lay->addWidget(m_forceFontsChk);

    m_fontsBox = new QWidget(page);
    auto *fontGrid = new QGridLayout(m_fontsBox);
    fontGrid->setContentsMargins(20, 2, 0, 0);
    fontGrid->setHorizontalSpacing(10);
    fontGrid->setVerticalSpacing(6);
    fontGrid->setColumnStretch(1, 1);

    fontGrid->addWidget(new QLabel(ztr("Default Style:"), m_fontsBox), 0, 0);
    m_fontStylePopup = new PlatinumPopupButton(m_fontsBox);
    m_fontStylePopup->addItem(ztr("Sans-Serif"), int(NetzacPreferences::ProportionalFontStyle::SansSerif));
    m_fontStylePopup->addItem(ztr("Serif"), int(NetzacPreferences::ProportionalFontStyle::Serif));
    m_fontStylePopup->setCurrentData(int(m_prefs.proportionalFontStyle));
    fontGrid->addWidget(m_fontStylePopup, 0, 1);

    fontGrid->addWidget(new QLabel(ztr("Serif Font:"), m_fontsBox), 1, 0);
    m_serifFontCombo = makeFontCombo(m_prefs.serifFont, false, m_fontsBox);
    fontGrid->addWidget(m_serifFontCombo, 1, 1);

    fontGrid->addWidget(new QLabel(ztr("Sans-Serif Font:"), m_fontsBox), 2, 0);
    m_sansFontCombo = makeFontCombo(m_prefs.sansSerifFont, false, m_fontsBox);
    fontGrid->addWidget(m_sansFontCombo, 2, 1);

    fontGrid->addWidget(new QLabel(ztr("Fixed (Code) Font:"), m_fontsBox), 3, 0);
    m_fixedFontCombo = makeFontCombo(m_prefs.fixedFont, true, m_fontsBox);
    fontGrid->addWidget(m_fixedFontCombo, 3, 1);

    fontGrid->addWidget(new QLabel(ztr("Font Size (px):"), m_fontsBox), 4, 0);
    auto *sizeRow = new QHBoxLayout();
    sizeRow->setContentsMargins(0, 0, 0, 0);
    m_fontSizeSpin = new QSpinBox(m_fontsBox);
    m_fontSizeSpin->setRange(8, 36);
    m_fontSizeSpin->setValue(m_prefs.defaultFontSize);
    m_fontSizeSpin->setFixedWidth(90);
    sizeRow->addWidget(m_fontSizeSpin);
    sizeRow->addStretch(1);
    fontGrid->addLayout(sizeRow, 4, 1);

    auto *fontHint = new QLabel(
        ztr("When enabled, Netzac injects CSS at the start of each page to force rendering with the selected fonts and size."),
        m_fontsBox);
    fontHint->setFont(zacosFont(QStringLiteral("ZacOS 9 Views"), 9));
    fontHint->setWordWrap(true);
    fontGrid->addWidget(fontHint, 5, 0, 1, 2);

    lay->addWidget(m_fontsBox);
    connect(m_forceFontsChk, &QCheckBox::toggled, m_fontsBox, &QWidget::setEnabled);
    m_fontsBox->setEnabled(m_forceFontsChk->isChecked());

    lay->addStretch(1);
    return page;
}

QWidget *PreferencesDialog::buildCalmModePage()
{
    auto *page = new QWidget(this);
    auto *lay = new QVBoxLayout(page);
    lay->setContentsMargins(6, 4, 6, 4);
    lay->setSpacing(6);

    lay->addWidget(new QLabel(ztr("JavaScript & Page Detection:"), page));
    m_calmJsPopup = new PlatinumPopupButton(page);
    m_calmJsPopup->addItem(
        ztr("Skip interactive JS web apps only (Recommended)"),
        int(CalmJsDetection::SkipWebApps));
    m_calmJsPopup->addItem(
        ztr("Apply on all non-excluded sites"),
        int(CalmJsDetection::IgnoreScripts));
    m_calmJsPopup->addItem(
        ztr("Strict: skip any page with <script> tags"),
        int(CalmJsDetection::SkipAnyScript));
    m_calmJsPopup->setCurrentData(int(m_prefs.calm.jsDetection));
    lay->addWidget(m_calmJsPopup);

    lay->addSpacing(4);
    lay->addWidget(new QLabel(ztr("Reading Layout:"), page));
    auto *layoutGroup = new QButtonGroup(page);
    m_calmLayoutClean = new QRadioButton(
        ztr("Clean full page (keep layout, remove distractions)"), page);
    m_calmLayoutReader = new QRadioButton(
        ztr("Focus on main text only (Firefox-style Reader Mode)"), page);
    layoutGroup->addButton(m_calmLayoutClean, int(CalmLayoutMode::CleanPage));
    layoutGroup->addButton(m_calmLayoutReader, int(CalmLayoutMode::ReaderFocus));
    m_calmLayoutClean->setChecked(m_prefs.calm.layoutMode == CalmLayoutMode::CleanPage);
    m_calmLayoutReader->setChecked(m_prefs.calm.layoutMode == CalmLayoutMode::ReaderFocus);
    lay->addWidget(m_calmLayoutClean);
    lay->addWidget(m_calmLayoutReader);

    lay->addSpacing(4);
    lay->addWidget(new QLabel(ztr("Elements to Remove & Clean:"), page));
    auto *chkGrid = new QGridLayout();
    chkGrid->setContentsMargins(0, 0, 0, 0);
    chkGrid->setHorizontalSpacing(14);
    chkGrid->setVerticalSpacing(4);

    m_calmRemoveScriptsChk = new QCheckBox(ztr("Remove <script> tags"), page);
    m_calmRemoveScriptsChk->setChecked(m_prefs.calm.removeScripts);
    m_calmRemoveIframesChk = new QCheckBox(ztr("Remove <iframe> embeds"), page);
    m_calmRemoveIframesChk->setChecked(m_prefs.calm.removeIframes);
    m_calmRemoveVideosChk = new QCheckBox(ztr("Remove <video> elements"), page);
    m_calmRemoveVideosChk->setChecked(m_prefs.calm.removeVideos);
    m_calmRemoveAdsChk = new QCheckBox(ztr("Remove ads, popups and hidden noise"), page);
    m_calmRemoveAdsChk->setChecked(m_prefs.calm.removeAds);
    m_calmNormalizeTablesChk = new QCheckBox(ztr("Normalize tables"), page);
    m_calmNormalizeTablesChk->setChecked(m_prefs.calm.normalizeTables);

    chkGrid->addWidget(m_calmRemoveScriptsChk, 0, 0);
    chkGrid->addWidget(m_calmRemoveIframesChk, 0, 1);
    chkGrid->addWidget(m_calmRemoveVideosChk, 1, 0);
    chkGrid->addWidget(m_calmNormalizeTablesChk, 1, 1);
    chkGrid->addWidget(m_calmRemoveAdsChk, 2, 0, 1, 2);
    lay->addLayout(chkGrid);

    lay->addSpacing(4);
    auto *imgRow = new QHBoxLayout();
    imgRow->addWidget(new QLabel(ztr("Maximum image width (px):"), page));
    m_calmMaxImgSpin = new QSpinBox(page);
    m_calmMaxImgSpin->setRange(240, 1920);
    m_calmMaxImgSpin->setSingleStep(20);
    m_calmMaxImgSpin->setValue(m_prefs.calm.maxImageWidth);
    m_calmMaxImgSpin->setFixedWidth(90);
    imgRow->addWidget(m_calmMaxImgSpin);
    imgRow->addStretch(1);
    lay->addLayout(imgRow);

    lay->addWidget(new QLabel(ztr("Excluded hosts (comma-separated, includes subdomains):"), page));
    m_calmExcludedHostsEdit = new QLineEdit(
        m_prefs.calm.excludedHosts.join(QStringLiteral(", ")), page);
    m_calmExcludedHostsEdit->setFont(zacosFont(QStringLiteral("ZacOS 9 System"), 12));
    lay->addWidget(m_calmExcludedHostsEdit);

    lay->addStretch(1);
    return page;
}

namespace {

bool editFolderSubItemDialog(
    NetzacPreferences::PersonalToolbarSubItem *sub,
    const QString &dialogTitle,
    QWidget *parent)
{
    PlatinumDialog dlg(parent);
    dlg.setWindowTitle(dialogTitle);
    dlg.setFixedWidth(360);

    auto *lay = new QVBoxLayout(&dlg);
    lay->setContentsMargins(12, 12, 12, 12);
    lay->setSpacing(6);

    lay->addWidget(new QLabel(ztr("Site Title:"), &dlg));
    auto *titleEdit = new QLineEdit(sub->title, &dlg);
    titleEdit->setFont(zacosFont(QStringLiteral("ZacOS 9 System"), 12));
    lay->addWidget(titleEdit);

    lay->addWidget(new QLabel(ztr("Target URL:"), &dlg));
    auto *urlEdit = new QLineEdit(sub->url, &dlg);
    urlEdit->setFont(zacosFont(QStringLiteral("ZacOS 9 System"), 12));
    lay->addWidget(urlEdit);

    auto *btnRow = new QHBoxLayout();
    btnRow->addStretch(1);
    QPushButton *cancelBtn = makeDialogButton(ztr("Cancel"), false, &dlg);
    QObject::connect(cancelBtn, &QPushButton::clicked, &dlg, &QDialog::reject);
    QPushButton *okBtn = makeDialogButton(ztr("OK"), true, &dlg);
    QObject::connect(okBtn, &QPushButton::clicked, &dlg, &QDialog::accept);
    btnRow->addWidget(cancelBtn);
    btnRow->addWidget(okBtn);
    lay->addLayout(btnRow);

    if (dlg.exec() != QDialog::Accepted)
        return false;

    const QString t = titleEdit->text().trimmed();
    const QString u = urlEdit->text().trimmed();
    if (t.isEmpty() || u.isEmpty())
        return false;
    sub->title = t;
    sub->url = u;
    return true;
}

bool editPersonalToolbarItemDialog(
    NetzacPreferences::PersonalToolbarItem *item,
    const QString &dialogTitle,
    QWidget *parent)
{
    PlatinumDialog dlg(parent);
    dlg.setWindowTitle(dialogTitle);
    dlg.setFixedWidth(430);

    auto *lay = new QVBoxLayout(&dlg);
    lay->setContentsMargins(12, 12, 12, 12);
    lay->setSpacing(6);

    lay->addWidget(new QLabel(ztr("Button Label:"), &dlg));
    auto *titleEdit = new QLineEdit(item->title, &dlg);
    titleEdit->setFont(zacosFont(QStringLiteral("ZacOS 9 System"), 12));
    lay->addWidget(titleEdit);

    lay->addWidget(new QLabel(ztr("Icon:"), &dlg));
    auto *iconPopup = new PlatinumPopupButton(&dlg);
    iconPopup->addItem(ztr("Page"), QStringLiteral("page"));
    iconPopup->addItem(ztr("Home"), QStringLiteral("home"));
    iconPopup->addItem(ztr("Search"), QStringLiteral("search"));
    iconPopup->addItem(ztr("Guide"), QStringLiteral("guide"));
    iconPopup->addItem(ztr("Folder"), QStringLiteral("folder"));
    const QString initIcon = item->isFolder
        ? QStringLiteral("folder")
        : (item->icon.isEmpty() ? QStringLiteral("page") : item->icon);
    iconPopup->setCurrentData(initIcon);
    lay->addWidget(iconPopup);

    auto *urlBox = new QWidget(&dlg);
    auto *urlLay = new QVBoxLayout(urlBox);
    urlLay->setContentsMargins(0, 0, 0, 0);
    urlLay->setSpacing(4);
    urlLay->addWidget(new QLabel(ztr("Target URL:"), urlBox));
    auto *urlEdit = new QLineEdit(item->url, urlBox);
    urlEdit->setFont(zacosFont(QStringLiteral("ZacOS 9 System"), 12));
    urlLay->addWidget(urlEdit);
    lay->addWidget(urlBox);

    QVector<NetzacPreferences::PersonalToolbarSubItem> subItems = item->children;
    auto *folderBox = new QWidget(&dlg);
    auto *folderLay = new QVBoxLayout(folderBox);
    folderLay->setContentsMargins(0, 0, 0, 0);
    folderLay->setSpacing(4);
    folderLay->addWidget(new QLabel(ztr("Sites inside folder:"), folderBox));
    auto *subList = new QListWidget(folderBox);
    subList->setFixedHeight(140);
    const auto refreshSubList = [subList, &subItems](int selectRow) {
        subList->clear();
        for (const auto &s : subItems)
            subList->addItem(QStringLiteral("%1  —  %2").arg(s.title, s.url));
        if (selectRow >= 0 && selectRow < subList->count())
            subList->setCurrentRow(selectRow);
    };
    refreshSubList(0);
    folderLay->addWidget(subList);

    auto *subBtnRow = new QHBoxLayout();
    subBtnRow->setSpacing(4);
    QPushButton *subAdd = makeDialogButton(ztr("Add"), false, folderBox);
    QPushButton *subEdit = makeDialogButton(ztr("Edit"), false, folderBox);
    QPushButton *subRem = makeDialogButton(ztr("Remove"), false, folderBox);
    QPushButton *subUp = makeDialogButton(ztr("Up"), false, folderBox);
    QPushButton *subDown = makeDialogButton(ztr("Down"), false, folderBox);
    for (QPushButton *b : {subAdd, subEdit, subRem, subUp, subDown})
        b->setMinimumWidth(56);

    QObject::connect(subAdd, &QPushButton::clicked, &dlg, [&dlg, &subItems, refreshSubList]() {
        NetzacPreferences::PersonalToolbarSubItem s{QString(), QStringLiteral("https://")};
        if (editFolderSubItemDialog(&s, ztr("Add Site to Folder"), &dlg)) {
            subItems.append(s);
            refreshSubList(int(subItems.size()) - 1);
        }
    });
    const auto editSelectedSub = [&dlg, subList, &subItems, refreshSubList]() {
        const int idx = subList->currentRow();
        if (idx >= 0 && idx < subItems.size()) {
            NetzacPreferences::PersonalToolbarSubItem s = subItems.at(idx);
            if (editFolderSubItemDialog(&s, ztr("Edit Folder Site"), &dlg)) {
                subItems[idx] = s;
                refreshSubList(idx);
            }
        }
    };
    QObject::connect(subEdit, &QPushButton::clicked, &dlg, editSelectedSub);
    QObject::connect(subList, &QListWidget::itemDoubleClicked, &dlg, [editSelectedSub](QListWidgetItem *) {
        editSelectedSub();
    });
    QObject::connect(subRem, &QPushButton::clicked, &dlg, [subList, &subItems, refreshSubList]() {
        const int idx = subList->currentRow();
        if (idx >= 0 && idx < subItems.size()) {
            subItems.removeAt(idx);
            refreshSubList(qMin(idx, int(subItems.size()) - 1));
        }
    });
    QObject::connect(subUp, &QPushButton::clicked, &dlg, [subList, &subItems, refreshSubList]() {
        const int idx = subList->currentRow();
        if (idx > 0 && idx < subItems.size()) {
            subItems.swapItemsAt(idx, idx - 1);
            refreshSubList(idx - 1);
        }
    });
    QObject::connect(subDown, &QPushButton::clicked, &dlg, [subList, &subItems, refreshSubList]() {
        const int idx = subList->currentRow();
        if (idx >= 0 && idx + 1 < subItems.size()) {
            subItems.swapItemsAt(idx, idx + 1);
            refreshSubList(idx + 1);
        }
    });

    subBtnRow->addWidget(subAdd);
    subBtnRow->addWidget(subEdit);
    subBtnRow->addWidget(subRem);
    subBtnRow->addWidget(subUp);
    subBtnRow->addWidget(subDown);
    subBtnRow->addStretch(1);
    folderLay->addLayout(subBtnRow);
    lay->addWidget(folderBox);

    const auto syncFolderVisibility = [&dlg, iconPopup, urlBox, folderBox]() {
        const bool isFolder = iconPopup->currentData().toString() == QLatin1String("folder");
        urlBox->setVisible(!isFolder);
        folderBox->setVisible(isFolder);
        dlg.adjustSize();
    };
    QObject::connect(iconPopup, &PlatinumPopupButton::currentIndexChanged, &dlg, syncFolderVisibility);
    syncFolderVisibility();

    auto *btnRow = new QHBoxLayout();
    btnRow->addStretch(1);
    QPushButton *cancelBtn = makeDialogButton(ztr("Cancel"), false, &dlg);
    QObject::connect(cancelBtn, &QPushButton::clicked, &dlg, &QDialog::reject);
    QPushButton *okBtn = makeDialogButton(ztr("OK"), true, &dlg);
    QObject::connect(okBtn, &QPushButton::clicked, &dlg, &QDialog::accept);
    btnRow->addWidget(cancelBtn);
    btnRow->addWidget(okBtn);
    lay->addLayout(btnRow);

    if (dlg.exec() != QDialog::Accepted)
        return false;

    const QString t = titleEdit->text().trimmed();
    const bool isFolder = iconPopup->currentData().toString() == QLatin1String("folder");
    const QString u = urlEdit->text().trimmed();
    if (t.isEmpty() || (!isFolder && u.isEmpty()))
        return false;

    item->title = t;
    item->isFolder = isFolder;
    item->icon = iconPopup->currentData().toString();
    item->url = isFolder ? QString() : u;
    item->children = isFolder ? subItems : QVector<NetzacPreferences::PersonalToolbarSubItem>{};
    return true;
}

}  // namespace

QWidget *PreferencesDialog::buildPersonalToolbarPage()
{
    m_personalItems = m_prefs.personalToolbar;

    auto *page = new QWidget(this);
    auto *lay = new QVBoxLayout(page);
    lay->setContentsMargins(6, 4, 6, 4);
    lay->setSpacing(6);

    lay->addWidget(new QLabel(ztr("Customize Personal Toolbar buttons & folders:"), page));

    m_ptList = new QListWidget(page);
    const auto refreshPtList = [this](int selectRow) {
        m_ptList->clear();
        for (const auto &entry : m_personalItems) {
            if (entry.isFolder) {
                m_ptList->addItem(
                    QStringLiteral("[%1: %2]  %3")
                        .arg(ztr("Folder"))
                        .arg(entry.children.size())
                        .arg(entry.title));
            } else {
                m_ptList->addItem(QStringLiteral("%1  —  %2").arg(entry.title, entry.url));
            }
        }
        if (selectRow >= 0 && selectRow < m_ptList->count())
            m_ptList->setCurrentRow(selectRow);
    };
    refreshPtList(0);
    lay->addWidget(m_ptList, 1);

    auto *row1 = new QHBoxLayout();
    row1->setSpacing(4);
    QPushButton *addBtn = makeDialogButton(ztr("Add"), false, page);
    QPushButton *editBtn = makeDialogButton(ztr("Edit"), false, page);
    QPushButton *remBtn = makeDialogButton(ztr("Remove"), false, page);
    QPushButton *upBtn = makeDialogButton(ztr("Up"), false, page);
    QPushButton *downBtn = makeDialogButton(ztr("Down"), false, page);
    QPushButton *resetBtn = makeDialogButton(ztr("Reset"), false, page);
    for (QPushButton *b : {addBtn, editBtn, remBtn, upBtn, downBtn, resetBtn})
        b->setMinimumWidth(56);

    connect(addBtn, &QPushButton::clicked, page, [this, page, refreshPtList]() {
        NetzacPreferences::PersonalToolbarItem item{QString(), QStringLiteral("https://"), QStringLiteral("page"), false, {}};
        if (editPersonalToolbarItemDialog(&item, ztr("Add Toolbar Button"), page)) {
            m_personalItems.append(item);
            refreshPtList(int(m_personalItems.size()) - 1);
        }
    });
    const auto editSelected = [this, page, refreshPtList]() {
        const int idx = m_ptList->currentRow();
        if (idx >= 0 && idx < m_personalItems.size()) {
            NetzacPreferences::PersonalToolbarItem item = m_personalItems.at(idx);
            if (editPersonalToolbarItemDialog(&item, ztr("Edit Toolbar Button"), page)) {
                m_personalItems[idx] = item;
                refreshPtList(idx);
            }
        }
    };
    connect(editBtn, &QPushButton::clicked, page, editSelected);
    connect(m_ptList, &QListWidget::itemDoubleClicked, page, [editSelected](QListWidgetItem *) {
        editSelected();
    });
    connect(remBtn, &QPushButton::clicked, page, [this, refreshPtList]() {
        const int idx = m_ptList->currentRow();
        if (idx >= 0 && idx < m_personalItems.size()) {
            m_personalItems.removeAt(idx);
            refreshPtList(qMin(idx, int(m_personalItems.size()) - 1));
        }
    });
    connect(upBtn, &QPushButton::clicked, page, [this, refreshPtList]() {
        const int idx = m_ptList->currentRow();
        if (idx > 0 && idx < m_personalItems.size()) {
            m_personalItems.swapItemsAt(idx, idx - 1);
            refreshPtList(idx - 1);
        }
    });
    connect(downBtn, &QPushButton::clicked, page, [this, refreshPtList]() {
        const int idx = m_ptList->currentRow();
        if (idx >= 0 && idx + 1 < m_personalItems.size()) {
            m_personalItems.swapItemsAt(idx, idx + 1);
            refreshPtList(idx + 1);
        }
    });
    connect(resetBtn, &QPushButton::clicked, page, [this, refreshPtList]() {
        m_personalItems = NetzacPreferences::defaultPersonalToolbar();
        refreshPtList(0);
    });

    row1->addWidget(addBtn);
    row1->addWidget(editBtn);
    row1->addWidget(remBtn);
    row1->addWidget(upBtn);
    row1->addWidget(downBtn);
    row1->addStretch(1);
    row1->addWidget(resetBtn);
    lay->addLayout(row1);

    return page;
}

QWidget *PreferencesDialog::buildQuickLinksPage()
{
    auto *page = new QWidget(this);
    auto *lay = new QVBoxLayout(page);
    lay->setContentsMargins(6, 4, 6, 4);
    lay->setSpacing(8);

    auto *infoLbl = new QLabel(
        ztr("Set custom URLs for the bottom-right Mail, Chat and Git buttons.\n"
            "Leave a field empty to open its built-in about: page."),
        page);
    infoLbl->setWordWrap(true);
    lay->addWidget(infoLbl);

    lay->addWidget(new QLabel(ztr("Mail URL (e.g. https://mail.google.com):"), page));
    m_mailUrlEdit = new QLineEdit(m_prefs.mailUrl, page);
    m_mailUrlEdit->setFont(zacosFont(QStringLiteral("ZacOS 9 System"), 12));
    lay->addWidget(m_mailUrlEdit);

    lay->addWidget(new QLabel(ztr("Chat URL (e.g. https://app.element.io):"), page));
    m_chatUrlEdit = new QLineEdit(m_prefs.chatUrl, page);
    m_chatUrlEdit->setFont(zacosFont(QStringLiteral("ZacOS 9 System"), 12));
    lay->addWidget(m_chatUrlEdit);

    lay->addWidget(new QLabel(ztr("Git URL (e.g. https://github.com):"), page));
    m_gitUrlEdit = new QLineEdit(m_prefs.gitUrl, page);
    m_gitUrlEdit->setFont(zacosFont(QStringLiteral("ZacOS 9 System"), 12));
    lay->addWidget(m_gitUrlEdit);

    lay->addStretch(1);
    return page;
}

QWidget *PreferencesDialog::buildCookiesPage()
{
    auto *page = new QWidget(this);
    auto *lay = new QVBoxLayout(page);
    lay->setContentsMargins(6, 4, 6, 4);
    lay->setSpacing(8);

    lay->addWidget(new QLabel(ztr("Cookie Permissions:"), page));

    auto *group = new QButtonGroup(page);
    m_cookieAllowAll = new QRadioButton(ztr("Accept all cookies"), page);
    m_cookieBlockThird = new QRadioButton(
        ztr("Accept only cookies from originating server"), page);
    m_cookieBlockAll = new QRadioButton(ztr("Disable cookies"), page);
    group->addButton(m_cookieAllowAll, int(NetzacPreferences::CookiePolicy::AllowAll));
    group->addButton(m_cookieBlockThird, int(NetzacPreferences::CookiePolicy::BlockThirdParty));
    group->addButton(m_cookieBlockAll, int(NetzacPreferences::CookiePolicy::BlockAll));

    m_cookieAllowAll->setChecked(m_prefs.cookiePolicy == NetzacPreferences::CookiePolicy::AllowAll);
    m_cookieBlockThird->setChecked(m_prefs.cookiePolicy == NetzacPreferences::CookiePolicy::BlockThirdParty);
    m_cookieBlockAll->setChecked(m_prefs.cookiePolicy == NetzacPreferences::CookiePolicy::BlockAll);

    lay->addWidget(m_cookieAllowAll);
    lay->addWidget(m_cookieBlockThird);
    lay->addWidget(m_cookieBlockAll);

    lay->addSpacing(4);
    m_cookieSessionChk = new QCheckBox(ztr("Keep cookies only until Netzac closes"), page);
    m_cookieSessionChk->setChecked(m_prefs.sessionCookiesOnly);
    lay->addWidget(m_cookieSessionChk);

    lay->addSpacing(6);
    auto *clearRow = new QHBoxLayout();
    QPushButton *clearBtn = makeDialogButton(ztr("Clear Cookies Now"), false, page);
    m_cookieStatusLbl = new QLabel(page);
    m_cookieStatusLbl->setFont(zacosFont(QStringLiteral("ZacOS 9 Views"), 9));
    connect(clearBtn, &QPushButton::clicked, this, [this]() {
        if (QWebEngineProfile *profile = netzacProfile()) {
            if (QWebEngineCookieStore *store = profile->cookieStore())
                store->deleteAllCookies();
        }
        m_cookieStatusLbl->setText(ztr("All cookies cleared."));
    });
    clearRow->addWidget(clearBtn);
    clearRow->addWidget(m_cookieStatusLbl, 1);
    lay->addLayout(clearRow);

    lay->addStretch(1);
    return page;
}

QWidget *PreferencesDialog::buildCachePage()
{
    auto *page = new QWidget(this);
    auto *lay = new QVBoxLayout(page);
    lay->setContentsMargins(6, 4, 6, 4);
    lay->setSpacing(8);

    lay->addWidget(new QLabel(ztr("Browser Cache Storage:"), page));
    m_cachePopup = new PlatinumPopupButton(page);
    m_cachePopup->addItem(ztr("Disk Cache"), int(NetzacPreferences::CacheMode::Disk));
    m_cachePopup->addItem(ztr("Memory Cache"), int(NetzacPreferences::CacheMode::Memory));
    m_cachePopup->addItem(ztr("Disable Cache"), int(NetzacPreferences::CacheMode::Disabled));
    m_cachePopup->setCurrentData(int(m_prefs.cacheMode));
    lay->addWidget(m_cachePopup);

    auto *sizeRow = new QHBoxLayout();
    sizeRow->addWidget(new QLabel(ztr("Maximum Cache Size (MB):"), page));
    m_cacheMaxSpin = new QSpinBox(page);
    m_cacheMaxSpin->setRange(1, 2048);
    m_cacheMaxSpin->setValue(m_prefs.cacheMaxMb);
    m_cacheMaxSpin->setFixedWidth(90);
    sizeRow->addWidget(m_cacheMaxSpin);
    sizeRow->addStretch(1);
    lay->addLayout(sizeRow);

    const auto updateCacheSpin = [this]() {
        const auto mode = static_cast<NetzacPreferences::CacheMode>(m_cachePopup->currentData().toInt());
        m_cacheMaxSpin->setEnabled(mode != NetzacPreferences::CacheMode::Disabled);
    };
    connect(m_cachePopup, &PlatinumPopupButton::currentIndexChanged, this, updateCacheSpin);
    updateCacheSpin();

    lay->addSpacing(6);
    auto *clearRow = new QHBoxLayout();
    QPushButton *clearBtn = makeDialogButton(ztr("Clear Cache Now"), false, page);
    m_cacheStatusLbl = new QLabel(page);
    m_cacheStatusLbl->setFont(zacosFont(QStringLiteral("ZacOS 9 Views"), 9));
    connect(clearBtn, &QPushButton::clicked, this, [this]() {
        if (QWebEngineProfile *profile = netzacProfile())
            profile->clearHttpCache();
        m_cacheStatusLbl->setText(ztr("HTTP cache cleared."));
    });
    clearRow->addWidget(clearBtn);
    clearRow->addWidget(m_cacheStatusLbl, 1);
    lay->addLayout(clearRow);

    lay->addStretch(1);
    return page;
}

QWidget *PreferencesDialog::buildProxiesPage()
{
    auto *page = new QWidget(this);
    auto *lay = new QVBoxLayout(page);
    lay->setContentsMargins(6, 4, 6, 4);
    lay->setSpacing(8);

    lay->addWidget(new QLabel(ztr("Configure Proxies to Access the Internet:"), page));

    auto *group = new QButtonGroup(page);
    m_proxyDirect = new QRadioButton(ztr("Direct connection to the Internet"), page);
    m_proxySystem = new QRadioButton(ztr("Use system proxy settings"), page);
    m_proxyManual = new QRadioButton(ztr("Manual proxy configuration"), page);
    group->addButton(m_proxyDirect, int(NetzacPreferences::ProxyMode::Direct));
    group->addButton(m_proxySystem, int(NetzacPreferences::ProxyMode::System));
    group->addButton(m_proxyManual, int(NetzacPreferences::ProxyMode::Manual));

    m_proxyDirect->setChecked(m_prefs.proxyMode == NetzacPreferences::ProxyMode::Direct);
    m_proxySystem->setChecked(m_prefs.proxyMode == NetzacPreferences::ProxyMode::System);
    m_proxyManual->setChecked(m_prefs.proxyMode == NetzacPreferences::ProxyMode::Manual);

    lay->addWidget(m_proxyDirect);
    lay->addWidget(m_proxySystem);
    lay->addWidget(m_proxyManual);

    m_proxyManualBox = new QWidget(page);
    auto *manLay = new QVBoxLayout(m_proxyManualBox);
    manLay->setContentsMargins(20, 4, 0, 0);
    manLay->setSpacing(6);

    auto *typeRow = new QHBoxLayout();
    typeRow->addWidget(new QLabel(ztr("Proxy Type:"), m_proxyManualBox));
    m_proxyTypePopup = new PlatinumPopupButton(m_proxyManualBox);
    m_proxyTypePopup->addItem(ztr("HTTP Proxy"), int(NetzacPreferences::ProxyType::Http));
    m_proxyTypePopup->addItem(ztr("SOCKS5 Proxy"), int(NetzacPreferences::ProxyType::Socks5));
    m_proxyTypePopup->setCurrentData(int(m_prefs.proxyType));
    typeRow->addWidget(m_proxyTypePopup, 1);
    manLay->addLayout(typeRow);

    auto *hostRow = new QHBoxLayout();
    hostRow->addWidget(new QLabel(ztr("Host:"), m_proxyManualBox));
    m_proxyHostEdit = new QLineEdit(m_prefs.proxyHost, m_proxyManualBox);
    m_proxyHostEdit->setPlaceholderText(QStringLiteral("127.0.0.1"));
    hostRow->addWidget(m_proxyHostEdit, 1);
    hostRow->addWidget(new QLabel(ztr("Port:"), m_proxyManualBox));
    m_proxyPortSpin = new QSpinBox(m_proxyManualBox);
    m_proxyPortSpin->setRange(1, 65535);
    m_proxyPortSpin->setValue(m_prefs.proxyPort);
    m_proxyPortSpin->setFixedWidth(78);
    hostRow->addWidget(m_proxyPortSpin);
    manLay->addLayout(hostRow);

    lay->addWidget(m_proxyManualBox);

    connect(m_proxyDirect, &QRadioButton::toggled, this, &PreferencesDialog::updateProxyControlsEnabled);
    connect(m_proxySystem, &QRadioButton::toggled, this, &PreferencesDialog::updateProxyControlsEnabled);
    connect(m_proxyManual, &QRadioButton::toggled, this, &PreferencesDialog::updateProxyControlsEnabled);
    updateProxyControlsEnabled();

    lay->addStretch(1);
    return page;
}

void PreferencesDialog::updateProxyControlsEnabled()
{
    m_proxyManualBox->setEnabled(m_proxyManual->isChecked());
}

QWidget *PreferencesDialog::buildUserAgentPage()
{
    auto *page = new QWidget(this);
    auto *lay = new QVBoxLayout(page);
    lay->setContentsMargins(6, 4, 6, 4);
    lay->setSpacing(8);

    lay->addWidget(new QLabel(ztr("Browser Identity Preset:"), page));
    m_uaPresetPopup = new PlatinumPopupButton(page);
    for (const auto &entry : NetzacPreferences::userAgentPresets())
        m_uaPresetPopup->addItem(entry.label, QString::fromLatin1(entry.id));
    m_uaPresetPopup->setCurrentData(m_prefs.userAgentPreset);
    lay->addWidget(m_uaPresetPopup);

    lay->addSpacing(4);
    lay->addWidget(new QLabel(ztr("HTTP User-Agent String:"), page));
    m_uaEdit = new QLineEdit(m_prefs.userAgent, page);
    m_uaEdit->setFont(zacosFont(QStringLiteral("ZacOS 9 Views"), 9));
    lay->addWidget(m_uaEdit);

    auto *resetRow = new QHBoxLayout();
    QPushButton *resetBtn = makeDialogButton(ztr("Reset to Chrome (Linux)"), false, page);
    connect(resetBtn, &QPushButton::clicked, this, [this]() {
        m_syncingUa = true;
        m_uaPresetPopup->setCurrentData(QStringLiteral("chrome_linux"));
        m_uaEdit->setText(NetzacPreferences::defaultChromeUserAgent());
        m_syncingUa = false;
    });
    resetRow->addWidget(resetBtn);
    resetRow->addStretch(1);
    lay->addLayout(resetRow);

    connect(m_uaPresetPopup, &PlatinumPopupButton::currentIndexChanged, this, [this]() {
        if (m_syncingUa)
            return;
        const QString preset = m_uaPresetPopup->currentData().toString();
        const QString ua = NetzacPreferences::userAgentForPreset(preset);
        if (!ua.isEmpty()) {
            m_syncingUa = true;
            m_uaEdit->setText(ua);
            m_syncingUa = false;
        }
    });

    connect(m_uaEdit, &QLineEdit::textEdited, this, [this](const QString &text) {
        if (m_syncingUa)
            return;
        m_syncingUa = true;
        QString matchedPreset = QStringLiteral("custom");
        for (const auto &entry : NetzacPreferences::userAgentPresets()) {
            if (*entry.value && text == QLatin1String(entry.value)) {
                matchedPreset = QString::fromLatin1(entry.id);
                break;
            }
        }
        m_uaPresetPopup->setCurrentData(matchedPreset);
        m_syncingUa = false;
    });

    lay->addStretch(1);
    return page;
}

QWidget *PreferencesDialog::buildAdBlockerPage()
{
    auto *page = new QWidget(this);
    auto *lay = new QVBoxLayout(page);
    lay->setContentsMargins(6, 4, 6, 4);
    lay->setSpacing(8);

    m_adBlockChk = new QCheckBox(ztr("Enable the Netzac Ad Blocker"), page);
    m_adBlockChk->setChecked(m_prefs.adBlockEnabled);
    lay->addWidget(m_adBlockChk);

    auto *desc = new QLabel(
        ztr("Netzac blocks advertisements, trackers and malicious addresses using filter "
            "lists such as EasyList. Lists are downloaded from the addresses below and "
            "stored locally on this computer."),
        page);
    desc->setFont(zacosFont(QStringLiteral("ZacOS 9 Views"), 9));
    desc->setWordWrap(true);
    lay->addWidget(desc);

    lay->addSpacing(4);
    lay->addWidget(new QLabel(ztr("Filter list addresses:"), page));

    m_adBlockUrls = m_prefs.adBlockSubscriptions;
    m_adBlockList = new QListWidget(page);
    m_adBlockList->setMinimumHeight(90);
    lay->addWidget(m_adBlockList, 1);
    refreshAdBlockList();

    auto *listButtons = new QHBoxLayout();
    m_adBlockAddBtn = makeDialogButton(ztr("Add List..."), false, page);
    connect(m_adBlockAddBtn, &QPushButton::clicked, this, &PreferencesDialog::addAdBlockListUrl);
    m_adBlockRemoveBtn = makeDialogButton(ztr("Remove List"), false, page);
    connect(m_adBlockRemoveBtn, &QPushButton::clicked, this, [this]() {
        const int row = m_adBlockList->currentRow();
        if (row < 0 || row >= m_adBlockUrls.size())
            return;
        m_adBlockUrls.removeAt(row);
        refreshAdBlockList();
        updateAdBlockStatus();
    });
    m_adBlockDefaultsBtn = makeDialogButton(ztr("Restore Default Lists"), false, page);
    connect(m_adBlockDefaultsBtn, &QPushButton::clicked, this, [this]() {
        m_adBlockUrls = NetzacPreferences::defaultAdBlockSubscriptions();
        refreshAdBlockList();
        updateAdBlockStatus();
    });
    listButtons->addWidget(m_adBlockAddBtn);
    listButtons->addWidget(m_adBlockRemoveBtn);
    listButtons->addWidget(m_adBlockDefaultsBtn);
    listButtons->addStretch(1);
    lay->addLayout(listButtons);

    lay->addSpacing(4);
    auto *updateRow = new QHBoxLayout();
    m_adBlockUpdateBtn = makeDialogButton(ztr("Update Filter Lists Now"), false, page);
    m_adBlockUpdateBtn->setMinimumWidth(190);
    connect(m_adBlockUpdateBtn, &QPushButton::clicked, this, [this]() {
        NetzacAdBlocker *blocker = NetzacAdBlocker::instance();
        blocker->configure(m_adBlockChk->isChecked(), m_adBlockUrls);
        blocker->refreshLists(true);
        updateAdBlockStatus();
    });
    updateRow->addWidget(m_adBlockUpdateBtn);
    updateRow->addStretch(1);
    lay->addLayout(updateRow);

    m_adBlockStatusLbl = new QLabel(page);
    m_adBlockStatusLbl->setFont(zacosFont(QStringLiteral("ZacOS 9 Views"), 9));
    m_adBlockStatusLbl->setWordWrap(true);
    lay->addWidget(m_adBlockStatusLbl);

    connect(m_adBlockChk, &QCheckBox::toggled, this, [this](bool enabled) {
        m_adBlockList->setEnabled(enabled);
        m_adBlockAddBtn->setEnabled(enabled);
        m_adBlockRemoveBtn->setEnabled(enabled);
        m_adBlockDefaultsBtn->setEnabled(enabled);
        m_adBlockUpdateBtn->setEnabled(enabled);
        updateAdBlockStatus();
    });

    const bool enabled = m_adBlockChk->isChecked();
    m_adBlockList->setEnabled(enabled);
    m_adBlockAddBtn->setEnabled(enabled);
    m_adBlockRemoveBtn->setEnabled(enabled);
    m_adBlockDefaultsBtn->setEnabled(enabled);
    m_adBlockUpdateBtn->setEnabled(enabled);

    connect(
        NetzacAdBlocker::instance(),
        &NetzacAdBlocker::stateChanged,
        this,
        &PreferencesDialog::updateAdBlockStatus);
    updateAdBlockStatus();

    return page;
}

void PreferencesDialog::refreshAdBlockList()
{
    if (!m_adBlockList)
        return;
    m_adBlockList->clear();
    for (const QString &url : m_adBlockUrls)
        m_adBlockList->addItem(url);
}

void PreferencesDialog::addAdBlockListUrl()
{
    PlatinumDialog dlg(this);
    dlg.setWindowTitle(ztr("Add Filter List"));
    dlg.setFixedWidth(420);

    auto *lay = new QVBoxLayout(&dlg);
    lay->setContentsMargins(12, 12, 12, 12);
    lay->setSpacing(8);
    lay->addWidget(new QLabel(ztr("Filter list address (URL):"), &dlg));
    auto *edit = new QLineEdit(QStringLiteral("https://"), &dlg);
    edit->setFont(zacosFont(QStringLiteral("ZacOS 9 System"), 12));
    lay->addWidget(edit);

    auto *row = new QHBoxLayout();
    row->addStretch(1);
    QPushButton *cancelBtn = makeDialogButton(ztr("Cancel"), true, &dlg);
    connect(cancelBtn, &QPushButton::clicked, &dlg, &QDialog::reject);
    QPushButton *okBtn = makeDialogButton(ztr("OK"), false, &dlg);
    connect(okBtn, &QPushButton::clicked, &dlg, &QDialog::accept);
    row->addWidget(cancelBtn);
    row->addWidget(okBtn);
    lay->addLayout(row);

    if (dlg.exec() != QDialog::Accepted)
        return;

    const QString text = edit->text().trimmed();
    const QUrl url(text);
    const QString scheme = url.scheme().toLower();
    if (!url.isValid() || url.host().isEmpty()
        || (scheme != QLatin1String("http") && scheme != QLatin1String("https"))) {
        PlatinumAlertDialog(
            ztr("Add Filter List"),
            ztr("The address must be a valid http:// or https:// URL."),
            QStringLiteral("warning"),
            this)
            .exec();
        return;
    }

    if (!m_adBlockUrls.contains(text)) {
        m_adBlockUrls.append(text);
        refreshAdBlockList();
        m_adBlockList->setCurrentRow(m_adBlockUrls.size() - 1);
    }
    updateAdBlockStatus();
}

void PreferencesDialog::updateAdBlockStatus()
{
    if (!m_adBlockStatusLbl || !m_adBlockChk)
        return;

    if (!m_adBlockChk->isChecked()) {
        m_adBlockStatusLbl->setText(ztr("Ad Blocker is disabled."));
        return;
    }
    if (NetzacAdBlocker::instance()->isUpdating()) {
        m_adBlockStatusLbl->setText(ztr("Updating filter lists..."));
        return;
    }
    const QString error = NetzacAdBlocker::instance()->lastError();
    if (!error.isEmpty()) {
        m_adBlockStatusLbl->setText(ztr("Update failed: %1").arg(error));
        return;
    }
    const int rules = NetzacAdBlocker::instance()->ruleCount();
    if (rules <= 0) {
        m_adBlockStatusLbl->setText(ztr("No filter lists loaded yet."));
        return;
    }
    const QDateTime updated = NetzacAdBlocker::instance()->lastUpdate();
    const QString when = updated.isValid()
        ? updated.toString(QStringLiteral("yyyy-MM-dd HH:mm"))
        : ztr("Never");
    m_adBlockStatusLbl->setText(ztr("%1 rules loaded. Last update: %2").arg(rules).arg(when));
}

NetzacPreferences PreferencesDialog::preferences() const
{
    if (m_resetRequested)
        return NetzacPreferences::defaults();

    NetzacPreferences out = m_prefs;
    const QString home = m_homeEdit->text().trimmed();
    if (!home.isEmpty())
        out.homeUrl = home;
    out.startupMode = static_cast<NetzacPreferences::StartupMode>(m_startupPopup->currentData().toInt());
    out.searchEngineId = m_searchPopup->currentData().toString();
    out.autoloadImages = m_imagesChk->isChecked();
    out.enableJavaScript = m_jsChk->isChecked();
    out.rememberPasswords = m_rememberPasswordsChk->isChecked();
    out.disableGpuAcceleration = m_disableGpuChk->isChecked();

    out.uiTheme = static_cast<NetzacPreferences::UiTheme>(m_uiThemePopup->currentData().toInt());
    out.enablePlatinumScrollbars = m_scrollbarsChk->isChecked();
    out.scrollbarAccent = m_scrollbarAccentPopup->currentData().toString();
    out.forceCustomFonts = m_forceFontsChk->isChecked();
    out.proportionalFontStyle = static_cast<NetzacPreferences::ProportionalFontStyle>(
        m_fontStylePopup->currentData().toInt());
    out.serifFont = m_serifFontCombo->currentData().toString().trimmed();
    out.sansSerifFont = m_sansFontCombo->currentData().toString().trimmed();
    out.fixedFont = m_fixedFontCombo->currentData().toString().trimmed();
    out.defaultFontSize = m_fontSizeSpin->value();

    out.calm.jsDetection = static_cast<CalmJsDetection>(m_calmJsPopup->currentData().toInt());
    out.calm.layoutMode = m_calmLayoutReader->isChecked()
        ? CalmLayoutMode::ReaderFocus
        : CalmLayoutMode::CleanPage;
    out.calm.removeScripts = m_calmRemoveScriptsChk->isChecked();
    out.calm.removeIframes = m_calmRemoveIframesChk->isChecked();
    out.calm.removeVideos = m_calmRemoveVideosChk->isChecked();
    out.calm.removeAds = m_calmRemoveAdsChk->isChecked();
    out.calm.normalizeTables = m_calmNormalizeTablesChk->isChecked();
    out.calm.maxImageWidth = m_calmMaxImgSpin->value();

    QStringList parsedHosts;
    const QStringList tokens = m_calmExcludedHostsEdit->text().split(
        QRegularExpression(QStringLiteral("[,\\s]+")), Qt::SkipEmptyParts);
    for (const QString &tok : tokens) {
        const QString norm = CalmPolicy::normalizeHost(tok);
        if (!norm.isEmpty() && !parsedHosts.contains(norm))
            parsedHosts.append(norm);
    }
    out.calm.excludedHosts = parsedHosts;

    out.personalToolbar = m_personalItems;
    out.mailUrl = m_mailUrlEdit->text().trimmed();
    out.chatUrl = m_chatUrlEdit->text().trimmed();
    out.gitUrl = m_gitUrlEdit->text().trimmed();

    if (m_cookieBlockAll->isChecked())
        out.cookiePolicy = NetzacPreferences::CookiePolicy::BlockAll;
    else if (m_cookieBlockThird->isChecked())
        out.cookiePolicy = NetzacPreferences::CookiePolicy::BlockThirdParty;
    else
        out.cookiePolicy = NetzacPreferences::CookiePolicy::AllowAll;
    out.sessionCookiesOnly = m_cookieSessionChk->isChecked();

    out.cacheMode = static_cast<NetzacPreferences::CacheMode>(m_cachePopup->currentData().toInt());
    out.cacheMaxMb = m_cacheMaxSpin->value();

    if (m_proxyDirect->isChecked())
        out.proxyMode = NetzacPreferences::ProxyMode::Direct;
    else if (m_proxyManual->isChecked())
        out.proxyMode = NetzacPreferences::ProxyMode::Manual;
    else
        out.proxyMode = NetzacPreferences::ProxyMode::System;
    out.proxyType = static_cast<NetzacPreferences::ProxyType>(m_proxyTypePopup->currentData().toInt());
    out.proxyHost = m_proxyHostEdit->text().trimmed();
    out.proxyPort = m_proxyPortSpin->value();

    out.userAgentPreset = m_uaPresetPopup->currentData().toString();
    const QString ua = m_uaEdit->text().trimmed();
    out.userAgent = ua.isEmpty() ? NetzacPreferences::defaultChromeUserAgent() : ua;

    out.adBlockEnabled = m_adBlockChk->isChecked();
    out.adBlockSubscriptions = m_adBlockUrls;
    return out;
}

namespace {

QString formatByteSize(qint64 bytes)
{
    if (bytes < 0)
        return ztr("Unknown");
    if (bytes < 1024)
        return QStringLiteral("%1 B").arg(bytes);
    const double kb = double(bytes) / 1024.0;
    if (kb < 1024.0)
        return QStringLiteral("%1 KB").arg(kb, 0, 'f', 1);
    const double mb = kb / 1024.0;
    if (mb < 1024.0)
        return QStringLiteral("%1 MB").arg(mb, 0, 'f', 1);
    const double gb = mb / 1024.0;
    return QStringLiteral("%1 GB").arg(gb, 0, 'f', 2);
}

QString formatDownloadTime(qint64 totalSeconds)
{
    if (totalSeconds < 0)
        return ztr("Unknown");
    const qint64 hrs = totalSeconds / 3600;
    const qint64 mins = (totalSeconds % 3600) / 60;
    const qint64 secs = totalSeconds % 60;
    if (hrs > 0) {
        return QStringLiteral("%1:%2:%3")
            .arg(hrs, 2, 10, QLatin1Char('0'))
            .arg(mins, 2, 10, QLatin1Char('0'))
            .arg(secs, 2, 10, QLatin1Char('0'));
    }
    return QStringLiteral("%1:%2")
        .arg(mins, 2, 10, QLatin1Char('0'))
        .arg(secs, 2, 10, QLatin1Char('0'));
}

bool moveFileWithFallback(const QString &src, const QString &dst)
{
    if (src == dst)
        return true;
    QDir().mkpath(QFileInfo(dst).absolutePath());
    if (QFile::exists(dst))
        QFile::remove(dst);
    if (QFile::rename(src, dst))
        return true;
    if (QFile::copy(src, dst)) {
        QFile::remove(src);
        return true;
    }
    return false;
}

} // namespace

class DownloadGlobeWidget : public QWidget {
public:
    explicit DownloadGlobeWidget(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setFixedSize(136, 44);
        m_timer.setInterval(55);
        connect(&m_timer, &QTimer::timeout, this, [this]() {
            m_frame = (m_frame + 1) % 24;
            update();
        });
    }

    void setAnimating(bool animating)
    {
        if (m_animating == animating)
            return;
        m_animating = animating;
        if (m_animating)
            m_timer.start();
        else
            m_timer.stop();
        update();
    }

protected:
    void paintEvent(QPaintEvent * /*event*/) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, false);

        // 1. Classic retro Earth globe on the left
        const QRect globeRect(4, 6, 32, 32);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(QStringLiteral("#2060b0")));
        p.drawEllipse(globeRect);

        p.save();
        p.setClipRegion(QRegion(globeRect, QRegion::Ellipse));
        // Continents
        p.setBrush(QColor(QStringLiteral("#46a24a")));
        p.drawPolygon(QPolygon({
            QPoint(10, 11), QPoint(19, 10), QPoint(22, 16),
            QPoint(17, 21), QPoint(12, 19)
        }));
        p.drawPolygon(QPolygon({
            QPoint(23, 20), QPoint(33, 18), QPoint(35, 29),
            QPoint(26, 33), QPoint(22, 26)
        }));
        p.drawPolygon(QPolygon({
            QPoint(6, 24), QPoint(14, 25), QPoint(15, 34),
            QPoint(7, 34)
        }));
        // Latitude / longitude grid lines
        p.setPen(QColor(QStringLiteral("#7ab8e8")));
        p.drawLine(globeRect.left(), globeRect.center().y(), globeRect.right(), globeRect.center().y());
        p.drawArc(globeRect.adjusted(6, 0, -6, 0), 0, 360 * 16);
        p.restore();

        // Globe highlight & outline
        p.setPen(QColor(QStringLiteral("#a8dcff")));
        p.drawArc(globeRect.adjusted(2, 2, -2, -2), 100 * 16, 70 * 16);
        p.setPen(QColor(QStringLiteral("#000000")));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(globeRect);

        // 2. Classic yellow/Platinum folder on the right
        const int fx = width() - 36;
        const int fy = 12;
        p.setPen(QColor(QStringLiteral("#000000")));
        p.setBrush(QColor(QStringLiteral("#e5bc42")));
        p.drawPolygon(QPolygon({
            QPoint(fx, fy), QPoint(fx + 11, fy), QPoint(fx + 14, fy + 4),
            QPoint(fx + 30, fy + 4), QPoint(fx + 30, fy + 26), QPoint(fx, fy + 26)
        }));
        p.setBrush(QColor(QStringLiteral("#f7dc7a")));
        p.drawRect(fx + 2, fy + 8, 28, 18);
        p.setPen(QColor(QStringLiteral("#fff4bd")));
        p.drawLine(fx + 3, fy + 9, fx + 29, fy + 9);

        // 3. Animated flying document from globe to folder
        if (m_animating) {
            const qreal t = qreal(m_frame) / 23.0;
            const int docX = int(38 + t * (fx - 52));
            const int arcLift = int(10.0 * (4.0 * t * (1.0 - t)));
            const int docY = 16 - arcLift;

            // Motion trail dots
            p.setPen(QColor(QStringLiteral("#4080c0")));
            for (int d = 1; d <= 3; ++d) {
                const qreal td = qMax<qreal>(0.0, t - d * 0.12);
                if (td > 0.0) {
                    const int tx = int(38 + td * (fx - 52));
                    const int ty = 22 - int(10.0 * (4.0 * td * (1.0 - td)));
                    p.drawPoint(tx, ty);
                    p.drawPoint(tx + 1, ty);
                }
            }

            // Document page with folded corner
            const QRect doc(docX, docY, 12, 15);
            p.setPen(QColor(QStringLiteral("#000000")));
            p.setBrush(QColor(QStringLiteral("#ffffff")));
            p.drawRect(doc);
            p.setPen(QColor(QStringLiteral("#8899aa")));
            p.drawLine(doc.left() + 3, doc.top() + 5, doc.right() - 2, doc.top() + 5);
            p.drawLine(doc.left() + 3, doc.top() + 8, doc.right() - 2, doc.top() + 8);
            p.drawLine(doc.left() + 3, doc.top() + 11, doc.right() - 4, doc.top() + 11);
        }
    }

private:
    QTimer m_timer;
    int m_frame = 0;
    bool m_animating = false;
};

DownloadDialog::DownloadDialog(QWebEngineDownloadRequest *download, QWidget *parent)
    : PlatinumDialog(parent)
    , m_download(download)
{
    setModal(false);
    setAttribute(Qt::WA_DeleteOnClose, true);
    setAttribute(Qt::WA_QuitOnClose, false);
    setWindowTitle(ztr("File Download"));
    setMinimumWidth(480);

    m_suggestedName = download ? download->suggestedFileName().trimmed() : QString();
    if (m_suggestedName.isEmpty())
        m_suggestedName = QStringLiteral("download");

    QString defaultDir = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    if (defaultDir.isEmpty())
        defaultDir = QDir::homePath();
    const QString defaultFilePath = QDir(defaultDir).filePath(m_suggestedName);
    const QString tempFilePath = QDir(QDir::tempPath()).filePath(m_suggestedName);

    auto *rootLay = new QVBoxLayout(this);
    rootLay->setContentsMargins(14, 14, 14, 12);
    rootLay->setSpacing(10);

    // Header: animated globe-to-folder icon + file metadata
    auto *headerLay = new QHBoxLayout();
    headerLay->setSpacing(12);

    m_globeAnim = new DownloadGlobeWidget(this);
    headerLay->addWidget(m_globeAnim, 0, Qt::AlignVCenter);

    auto *metaLay = new QVBoxLayout();
    metaLay->setSpacing(3);

    m_fileNameLbl = new QLabel(m_suggestedName, this);
    QFont boldFont = zacosFont(QStringLiteral("ZacOS 9 System"), 12);
    boldFont.setBold(true);
    m_fileNameLbl->setFont(boldFont);
    metaLay->addWidget(m_fileNameLbl);

    const QString rawUrl = download ? download->url().toString(QUrl::RemoveUserInfo) : QString();
    const QFontMetrics fm(font());
    const QString elidedUrl = fm.elidedText(rawUrl, Qt::ElideMiddle, 290);
    m_sourceUrlLbl = new QLabel(ztr("From: %1").arg(elidedUrl), this);
    m_sourceUrlLbl->setToolTip(rawUrl);
    metaLay->addWidget(m_sourceUrlLbl);

    const qint64 initialTotal = download ? download->totalBytes() : -1;
    const QString sizeText = initialTotal > 0 ? formatByteSize(initialTotal) : ztr("Unknown");
    m_sizeInfoLbl = new QLabel(ztr("Size: %1").arg(sizeText), this);
    metaLay->addWidget(m_sizeInfoLbl);

    headerLay->addLayout(metaLay, 1);
    rootLay->addLayout(headerLay);

    // Progress bar & IE-style stats (visible immediately)
    m_progressBox = new QWidget(this);
    auto *progLay = new QVBoxLayout(m_progressBox);
    progLay->setContentsMargins(0, 0, 0, 0);
    progLay->setSpacing(6);

    m_savingToLbl = new QLabel(this);
    m_savingToLbl->setWordWrap(false);
    m_savingToLbl->setVisible(false);
    progLay->addWidget(m_savingToLbl);

    m_progressBar = new PlatinumProgressBar(m_progressBox);
    m_progressBar->setMinimumSize(280, 16);
    m_progressBar->setMaximumSize(QWIDGETSIZE_MAX, 16);
    m_progressBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    progLay->addWidget(m_progressBar);

    auto *statsGrid = new QGridLayout();
    statsGrid->setHorizontalSpacing(12);
    statsGrid->setVerticalSpacing(3);
    statsGrid->setColumnStretch(1, 1);

    m_downloadedLbl = new QLabel(
        initialTotal > 0 ? QStringLiteral("0 B / %1 (0%)").arg(formatByteSize(initialTotal)) : QStringLiteral("0 B"),
        m_progressBox);
    m_rateLbl = new QLabel(QStringLiteral("0 B/s"), m_progressBox);
    m_elapsedLbl = new QLabel(formatDownloadTime(0), m_progressBox);
    m_remainingLbl = new QLabel(QStringLiteral("—"), m_progressBox);

    statsGrid->addWidget(new QLabel(ztr("Downloaded:"), m_progressBox), 0, 0);
    statsGrid->addWidget(m_downloadedLbl, 0, 1);
    statsGrid->addWidget(new QLabel(ztr("Transfer rate:"), m_progressBox), 1, 0);
    statsGrid->addWidget(m_rateLbl, 1, 1);
    statsGrid->addWidget(new QLabel(ztr("Time elapsed:"), m_progressBox), 2, 0);
    statsGrid->addWidget(m_elapsedLbl, 2, 1);
    statsGrid->addWidget(new QLabel(ztr("Estimated time left:"), m_progressBox), 3, 0);
    statsGrid->addWidget(m_remainingLbl, 3, 1);
    progLay->addLayout(statsGrid);

    rootLay->addWidget(m_progressBox);

    auto *sep = new QFrame(this);
    sep->setFrameShape(QFrame::HLine);
    sep->setFrameShadow(QFrame::Sunken);
    rootLay->addWidget(sep);

    // Destination selection
    m_promptBox = new QWidget(this);
    auto *promptLay = new QVBoxLayout(m_promptBox);
    promptLay->setContentsMargins(0, 0, 0, 0);
    promptLay->setSpacing(6);

    m_saveCustomRadio = new QRadioButton(ztr("Save to folder:"), m_promptBox);
    m_saveCustomRadio->setChecked(true);
    promptLay->addWidget(m_saveCustomRadio);

    auto *customRow = new QHBoxLayout();
    customRow->setContentsMargins(20, 0, 0, 0);
    customRow->setSpacing(6);
    m_customPathEdit = new QLineEdit(defaultFilePath, m_promptBox);
    m_customPathEdit->setFont(zacosFont(QStringLiteral("ZacOS 9 System"), 12));
    customRow->addWidget(m_customPathEdit, 1);

    m_browseBtn = makeDialogButton(ztr("Browse..."), false, m_promptBox);
    connect(m_browseBtn, &QPushButton::clicked, this, &DownloadDialog::browseDestination);
    customRow->addWidget(m_browseBtn);
    promptLay->addLayout(customRow);

    m_saveTempRadio = new QRadioButton(ztr("Save to system temporary folder"), m_promptBox);
    promptLay->addWidget(m_saveTempRadio);

    m_tempPathLbl = new QLabel(fm.elidedText(tempFilePath, Qt::ElideMiddle, 400), m_promptBox);
    m_tempPathLbl->setToolTip(tempFilePath);
    m_tempPathLbl->setContentsMargins(20, 0, 0, 0);
    m_tempPathLbl->setEnabled(false);
    promptLay->addWidget(m_tempPathLbl);

    auto *destGroup = new QButtonGroup(this);
    destGroup->addButton(m_saveCustomRadio);
    destGroup->addButton(m_saveTempRadio);

    connect(m_saveCustomRadio, &QRadioButton::toggled, this, [this](bool checked) {
        m_customPathEdit->setEnabled(checked);
        m_browseBtn->setEnabled(checked);
        m_tempPathLbl->setEnabled(!checked);
        if (!checked && !m_userToggledOpen) {
            m_openAutoToggled = true;
            m_openAfterChk->setChecked(true);
            m_openAutoToggled = false;
        } else if (checked && !m_userToggledOpen) {
            m_openAutoToggled = true;
            m_openAfterChk->setChecked(false);
            m_openAutoToggled = false;
        }
    });

    rootLay->addWidget(m_promptBox);

    // Shared option: open after download completes
    m_openAfterChk = new QCheckBox(ztr("Open file after download completes"), this);
    connect(m_openAfterChk, &QCheckBox::toggled, this, [this](bool) {
        if (!m_openAutoToggled)
            m_userToggledOpen = true;
    });
    rootLay->addWidget(m_openAfterChk);

    // Buttons row
    auto *btnRow = new QHBoxLayout();
    btnRow->addStretch(1);

    m_openFileBtn = makeDialogButton(ztr("Open File"), false, this);
    m_openFileBtn->setVisible(false);
    connect(m_openFileBtn, &QPushButton::clicked, this, &DownloadDialog::openDownloadedFile);
    btnRow->addWidget(m_openFileBtn);

    m_openFolderBtn = makeDialogButton(ztr("Open Folder"), false, this);
    m_openFolderBtn->setVisible(false);
    connect(m_openFolderBtn, &QPushButton::clicked, this, &DownloadDialog::openContainingFolder);
    btnRow->addWidget(m_openFolderBtn);

    m_startBtn = makeDialogButton(ztr("Start Download"), true, this);
    connect(m_startBtn, &QPushButton::clicked, this, &DownloadDialog::startDownload);
    btnRow->addWidget(m_startBtn);

    m_cancelCloseBtn = makeDialogButton(ztr("Cancel"), false, this);
    connect(m_cancelCloseBtn, &QPushButton::clicked, this, &DownloadDialog::reject);
    btnRow->addWidget(m_cancelCloseBtn);

    rootLay->addLayout(btnRow);

    m_tickTimer.setInterval(250);
    connect(&m_tickTimer, &QTimer::timeout, this, &DownloadDialog::updateProgress);

    if (m_download) {
        // Qt WebEngine requires accept() or cancel() synchronously inside downloadRequested.
        // Accept into a temporary staging directory and immediately pause so no transfer
        // proceeds until the user clicks Start Download.
        const QString stageDir = m_stagingDir.isValid() ? m_stagingDir.path() : QDir::tempPath();
        m_stagingPath = QDir(stageDir).filePath(m_suggestedName);
        m_download->setDownloadDirectory(stageDir);
        m_download->setDownloadFileName(m_suggestedName);
        m_download->accept();
        m_download->pause();

        connect(m_download, &QWebEngineDownloadRequest::totalBytesChanged, this, [this]() {
            if (!m_download)
                return;
            const qint64 total = m_download->totalBytes();
            m_sizeInfoLbl->setText(ztr("Size: %1").arg(total > 0 ? formatByteSize(total) : ztr("Unknown")));
            if (m_userConfirmed)
                updateProgress();
            else
                m_downloadedLbl->setText(total > 0 ? QStringLiteral("0 B / %1 (0%)").arg(formatByteSize(total)) : QStringLiteral("0 B"));
        });
        connect(m_download, &QWebEngineDownloadRequest::receivedBytesChanged, this, &DownloadDialog::updateProgress);
        connect(m_download, &QWebEngineDownloadRequest::stateChanged, this, &DownloadDialog::updateProgress);
        connect(m_download, &QWebEngineDownloadRequest::isFinishedChanged, this, &DownloadDialog::updateProgress);
        connect(m_download, &QObject::destroyed, this, [this]() {
            m_tickTimer.stop();
            if (!m_downloadDone && !m_finished) {
                m_downloadDone = true;
                m_downloadSuccess = false;
                m_downloadError = ztr("Download was interrupted.");
                finishDownloadUi(false, m_downloadError);
            }
        });
    }

    adjustSize();
}

DownloadDialog::~DownloadDialog()
{
    m_tickTimer.stop();
    if (m_download && !m_download->isFinished())
        m_download->cancel();
}

void DownloadDialog::reject()
{
    m_tickTimer.stop();
    if (m_download && !m_download->isFinished())
        m_download->cancel();
    PlatinumDialog::reject();
}

void DownloadDialog::browseDestination()
{
    const QString initial = m_customPathEdit->text().trimmed().isEmpty()
        ? m_suggestedName
        : m_customPathEdit->text().trimmed();
    const QString picked = platinumSaveFileName(this, ztr("Save File"), initial);
    if (!picked.isEmpty()) {
        m_saveCustomRadio->setChecked(true);
        m_customPathEdit->setText(picked);
    }
}

QString DownloadDialog::resolvedTargetPath() const
{
    if (m_saveTempRadio->isChecked())
        return QDir(QDir::tempPath()).filePath(m_suggestedName);
    const QString path = m_customPathEdit->text().trimmed();
    if (path.isEmpty()) {
        QString dlDir = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
        if (dlDir.isEmpty())
            dlDir = QDir::homePath();
        return QDir(dlDir).filePath(m_suggestedName);
    }
    const QFileInfo fi(path);
    if (fi.isDir())
        return QDir(fi.absoluteFilePath()).filePath(m_suggestedName);
    return fi.absoluteFilePath();
}

void DownloadDialog::startDownload()
{
    if (m_userConfirmed)
        return;

    m_userConfirmed = true;
    m_targetPath = resolvedTargetPath();

    m_promptBox->setVisible(false);
    m_startBtn->setVisible(false);

    const QFontMetrics fm(font());
    const QString elidedDest = fm.elidedText(m_targetPath, Qt::ElideMiddle, 380);
    m_savingToLbl->setText(ztr("Saving to: %1").arg(elidedDest));
    m_savingToLbl->setToolTip(m_targetPath);
    m_savingToLbl->setVisible(true);

    m_elapsed.start();
    m_tickTimer.start();
    if (m_globeAnim)
        m_globeAnim->setAnimating(true);

    if (m_download && m_download->isPaused())
        m_download->resume();

    updateProgress();
    finalizeIfReady();
    adjustSize();
}

void DownloadDialog::updateProgress()
{
    if (m_finished || m_downloadDone || !m_download)
        return;

    const QString actualStage = QDir(m_download->downloadDirectory()).filePath(m_download->downloadFileName());
    if (!actualStage.isEmpty())
        m_stagingPath = actualStage;

    const auto st = m_download->state();
    if (!m_userConfirmed) {
        if (st == QWebEngineDownloadRequest::DownloadCompleted) {
            m_downloadDone = true;
            m_downloadSuccess = true;
        } else if (st == QWebEngineDownloadRequest::DownloadInterrupted) {
            m_downloadDone = true;
            m_downloadSuccess = false;
            m_downloadError = m_download->interruptReasonString();
            finishDownloadUi(false, m_downloadError);
        } else if (st == QWebEngineDownloadRequest::DownloadCancelled) {
            m_downloadDone = true;
            m_downloadSuccess = false;
            m_downloadError = ztr("Download cancelled.");
            finishDownloadUi(false, m_downloadError);
        }
        return;
    }

    const qint64 received = m_download->receivedBytes();
    const qint64 total = m_download->totalBytes();
    const qint64 elapsedMs = qMax<qint64>(1, m_elapsed.elapsed());
    const qint64 elapsedSecs = elapsedMs / 1000;
    const double bytesPerSec = (double(received) * 1000.0) / double(elapsedMs);

    if (total > 0) {
        const int pct = int(qBound(qint64(0), (received * 100) / total, qint64(100)));
        m_progressBar->setIndeterminate(false);
        m_progressBar->setValue(pct);
        m_downloadedLbl->setText(
            QStringLiteral("%1 / %2 (%3%)")
                .arg(formatByteSize(received), formatByteSize(total))
                .arg(pct));
        setWindowTitle(QStringLiteral("%1% - %2").arg(pct).arg(m_suggestedName));

        if (bytesPerSec > 1.0 && received < total) {
            const qint64 remSecs = qint64(double(total - received) / bytesPerSec);
            m_remainingLbl->setText(formatDownloadTime(remSecs));
        } else if (received >= total) {
            m_remainingLbl->setText(formatDownloadTime(0));
        } else {
            m_remainingLbl->setText(ztr("Calculating..."));
        }
    } else {
        m_progressBar->setIndeterminate(true);
        m_downloadedLbl->setText(formatByteSize(received));
        m_remainingLbl->setText(ztr("Unknown"));
        setWindowTitle(m_suggestedName);
    }

    m_rateLbl->setText(QStringLiteral("%1/s").arg(formatByteSize(qint64(bytesPerSec))));
    m_elapsedLbl->setText(formatDownloadTime(elapsedSecs));

    if (st == QWebEngineDownloadRequest::DownloadCompleted) {
        m_downloadDone = true;
        m_downloadSuccess = true;
        m_tickTimer.stop();
        m_progressBar->setIndeterminate(false);
        m_progressBar->setValue(100);
        m_remainingLbl->setText(formatDownloadTime(0));
        finalizeIfReady();
    } else if (st == QWebEngineDownloadRequest::DownloadInterrupted) {
        m_downloadDone = true;
        m_downloadSuccess = false;
        m_downloadError = m_download->interruptReasonString();
        finishDownloadUi(false, m_downloadError);
    } else if (st == QWebEngineDownloadRequest::DownloadCancelled) {
        m_downloadDone = true;
        m_downloadSuccess = false;
        m_downloadError = ztr("Download cancelled.");
        finishDownloadUi(false, m_downloadError);
    }
}

void DownloadDialog::finalizeIfReady()
{
    if (!m_userConfirmed || !m_downloadDone || m_finished)
        return;

    if (m_downloadSuccess) {
        if (!moveFileWithFallback(m_stagingPath, m_targetPath)) {
            // Fallback to staging path if moving failed
            m_targetPath = m_stagingPath;
        }
        finishDownloadUi(true);
    } else {
        finishDownloadUi(false, m_downloadError);
    }
}

void DownloadDialog::finishDownloadUi(bool success, const QString &errorText)
{
    if (m_finished)
        return;
    m_finished = true;
    m_tickTimer.stop();
    if (m_globeAnim)
        m_globeAnim->setAnimating(false);

    m_progressBar->setIndeterminate(false);
    if (success) {
        m_progressBar->setValue(100);
        m_remainingLbl->setText(formatDownloadTime(0));
        setWindowTitle(ztr("Download Complete - %1").arg(m_suggestedName));
        m_openFileBtn->setVisible(true);
        m_openFolderBtn->setVisible(true);
    } else {
        setWindowTitle(ztr("Download Failed - %1").arg(m_suggestedName));
        if (!errorText.isEmpty())
            m_remainingLbl->setText(errorText);
    }

    m_promptBox->setVisible(false);
    m_startBtn->setVisible(false);
    m_openAfterChk->setEnabled(false);
    m_cancelCloseBtn->setText(ztr("Close"));
    m_cancelCloseBtn->setDefault(true);
    m_cancelCloseBtn->setAutoDefault(true);

    if (success && m_openAfterChk->isChecked() && !m_targetPath.isEmpty())
        openDownloadedFile();
}

void DownloadDialog::openDownloadedFile()
{
    if (!m_targetPath.isEmpty())
        QDesktopServices::openUrl(QUrl::fromLocalFile(m_targetPath));
}

void DownloadDialog::openContainingFolder()
{
    if (!m_targetPath.isEmpty())
        QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(m_targetPath).absolutePath()));
}

namespace {

QString formatHexFingerprint(const QByteArray &digest)
{
    const QByteArray hex = digest.toHex().toUpper();
    QStringList parts;
    parts.reserve(hex.size() / 2);
    for (int i = 0; i + 1 < hex.size(); i += 2)
        parts.append(QString::fromLatin1(hex.mid(i, 2)));
    return parts.join(QLatin1Char(':'));
}

QString formatCertInfoList(const QStringList &values)
{
    return values.isEmpty() ? QStringLiteral("—") : values.join(QStringLiteral(", "));
}

QString formatPublicKeyInfo(const QSslKey &key)
{
    if (key.isNull())
        return QStringLiteral("—");
    QString algo;
    switch (key.algorithm()) {
    case QSsl::Rsa:
        algo = QStringLiteral("RSA");
        break;
    case QSsl::Dsa:
        algo = QStringLiteral("DSA");
        break;
    case QSsl::Ec:
        algo = QStringLiteral("EC");
        break;
    case QSsl::Dh:
        algo = QStringLiteral("DH");
        break;
    default:
        algo = ztr("Unknown");
        break;
    }
    return QStringLiteral("%1 (%2 bits)").arg(algo).arg(key.length());
}

QString formatSslProtocol(QSsl::SslProtocol proto)
{
    switch (proto) {
    case QSsl::TlsV1_2:
        return QStringLiteral("TLS 1.2");
    case QSsl::TlsV1_3:
        return QStringLiteral("TLS 1.3");
    case QSsl::TlsV1_2OrLater:
        return QStringLiteral("TLS 1.2+");
    case QSsl::TlsV1_3OrLater:
        return QStringLiteral("TLS 1.3+");
    case QSsl::DtlsV1_2:
        return QStringLiteral("DTLS 1.2");
    default:
        return ztr("Unknown");
    }
}

QString formatSanMap(const QMultiMap<QSsl::AlternativeNameEntryType, QString> &san)
{
    if (san.isEmpty())
        return QStringLiteral("—");

    QStringList entries;
    const auto appendType = [&san, &entries](QSsl::AlternativeNameEntryType type, const QString &label) {
        const QStringList vals = san.values(type);
        for (const QString &v : vals)
            entries.append(QStringLiteral("%1: %2").arg(label, v));
    };
    appendType(QSsl::DnsEntry, QStringLiteral("DNS"));
    appendType(QSsl::IpAddressEntry, QStringLiteral("IP"));
    appendType(QSsl::EmailEntry, QStringLiteral("Email"));
    return entries.isEmpty() ? QStringLiteral("—") : entries.join(QStringLiteral(", "));
}

QString formatValidityRange(const QDateTime &dt)
{
    if (!dt.isValid())
        return QStringLiteral("—");
    return QStringLiteral("%1 (%2)")
        .arg(dt.toLocalTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")),
             dt.toUTC().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss 'UTC'")));
}

}  // namespace

CertificateInfoDialog::CertificateInfoDialog(
    const QString &title,
    const QString &url,
    QWidget *parent)
    : PlatinumDialog(parent)
{
    setWindowTitle(ztr("Page & Security Info"));
    const QPixmap certPm = makeAlertIcon(QStringLiteral("certificate"), 48);
    setWindowIcon(QIcon(certPm));
    setMinimumSize(540, 440);
    resize(580, 480);

    auto *lay = new QVBoxLayout(this);
    lay->setContentsMargins(14, 14, 14, 12);
    lay->setSpacing(10);

    auto *topRow = new QHBoxLayout();
    topRow->setSpacing(12);

    auto *iconLbl = new QLabel(this);
    iconLbl->setPixmap(certPm);
    iconLbl->setFixedSize(48, 48);
    topRow->addWidget(iconLbl, 0, Qt::AlignTop);

    auto *metaCol = new QVBoxLayout();
    metaCol->setSpacing(4);

    auto *titleLbl = new QLabel(title.isEmpty() ? ztr("(Untitled)") : title, this);
    titleLbl->setFont(zacosFont(QStringLiteral("ZacOS 9 System"), 12, true));
    titleLbl->setWordWrap(true);
    metaCol->addWidget(titleLbl);

    auto *locLbl = new QLabel(ztr("Location: %1").arg(url), this);
    locLbl->setFont(zacosFont(QStringLiteral("ZacOS 9 Views"), 9));
    locLbl->setWordWrap(true);
    locLbl->setTextInteractionFlags(Qt::TextSelectableByMouse);
    metaCol->addWidget(locLbl);

    QString secSummary;
    if (url.startsWith(QLatin1String("https://")))
        secSummary = ztr("Encrypted connection (HTTPS) — Security padlock is locked.");
    else if (url.startsWith(QLatin1String("about:")))
        secSummary = ztr("Built-in Netzac Navigator local document.");
    else
        secSummary = ztr("Unencrypted connection (HTTP) — Information sent over this connection is not encrypted.");

    auto *secLbl = new QLabel(secSummary, this);
    secLbl->setWordWrap(true);
    metaCol->addWidget(secLbl);

    topRow->addLayout(metaCol, 1);
    lay->addLayout(topRow);

    auto *sep = new QFrame(this);
    sep->setFrameShape(QFrame::HLine);
    lay->addWidget(sep);

    auto *detailsHdr = new QLabel(ztr("Certificate Details:"), this);
    detailsHdr->setFont(zacosFont(QStringLiteral("ZacOS 9 System"), 12, true));
    lay->addWidget(detailsHdr);

    m_detailsEdit = new QPlainTextEdit(this);
    m_detailsEdit->setReadOnly(true);
    m_detailsEdit->setLineWrapMode(QPlainTextEdit::WidgetWidth);
    QFont monoFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    monoFont.setPixelSize(11);
    m_detailsEdit->setFont(monoFont);
    lay->addWidget(m_detailsEdit, 1);

    auto *btnRow = new QHBoxLayout();
    m_copyBtn = makeDialogButton(ztr("Copy Details"), false, this);
    m_copyBtn->setMinimumWidth(110);
    connect(m_copyBtn, &QPushButton::clicked, this, [this]() {
        if (QClipboard *cb = QApplication::clipboard())
            cb->setText(m_detailsEdit->toPlainText());
    });
    btnRow->addWidget(m_copyBtn);
    btnRow->addStretch(1);

    QPushButton *closeBtn = makeDialogButton(ztr("Close"), true, this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnRow->addWidget(closeBtn);
    lay->addLayout(btnRow);

    m_timeoutTimer.setSingleShot(true);
    m_timeoutTimer.setInterval(8000);
    connect(&m_timeoutTimer, &QTimer::timeout, this, [this]() {
        if (m_lookupFinished)
            return;
        const QString target = m_port == 443
            ? m_host
            : QStringLiteral("%1:%2").arg(m_host).arg(m_port);
        finishLookupWithError(ztr("Timed out while retrieving certificate from %1.").arg(target));
    });

    const QUrl parsed(url);
    if (parsed.scheme().compare(QLatin1String("https"), Qt::CaseInsensitive) == 0
        && !parsed.host().isEmpty()) {
        startCertificateLookup(parsed);
    } else {
        m_lookupFinished = true;
        QStringList lines;
        lines << ztr("Title: %1").arg(title.isEmpty() ? ztr("(Untitled)") : title);
        lines << ztr("Location: %1").arg(url);
        lines << ztr("Protocol: %1").arg(parsed.scheme().isEmpty() ? ztr("Unknown") : parsed.scheme().toUpper());
        lines << QString();
        lines << ztr("No TLS/SSL certificate is used for this page.");
        m_detailsEdit->setPlainText(lines.join(QLatin1Char('\n')));
    }
}

CertificateInfoDialog::~CertificateInfoDialog()
{
    m_timeoutTimer.stop();
    if (m_socket)
        m_socket->abort();
}

void CertificateInfoDialog::startCertificateLookup(const QUrl &url)
{
    m_host = url.host();
    m_port = static_cast<quint16>(url.port(443));
    const QString hostPort = m_port == 443
        ? m_host
        : QStringLiteral("%1:%2").arg(m_host).arg(m_port);

    if (!QSslSocket::supportsSsl()) {
        m_lookupFinished = true;
        m_detailsEdit->setPlainText(ztr("TLS/SSL certificate inspection is unavailable on this system."));
        return;
    }

    m_detailsEdit->setPlainText(ztr("Inspecting TLS certificate for %1...").arg(hostPort));

    m_socket = new QSslSocket(this);
    connect(m_socket, &QSslSocket::sslErrors, this, [this](const QList<QSslError> &errors) {
        for (const QSslError &err : errors) {
            const QString msg = err.errorString().trimmed();
            if (!msg.isEmpty() && !m_sslWarnings.contains(msg))
                m_sslWarnings.append(msg);
        }
        if (m_socket)
            m_socket->ignoreSslErrors();
    });
    connect(m_socket, &QSslSocket::encrypted, this, &CertificateInfoDialog::displayCertificateResult);
    connect(m_socket, &QAbstractSocket::errorOccurred, this, [this](QAbstractSocket::SocketError) {
        if (m_lookupFinished || !m_socket)
            return;
        if (!m_socket->peerCertificate().isNull())
            displayCertificateResult();
        else
            finishLookupWithError(m_socket->errorString());
    });

    m_timeoutTimer.start();
    m_socket->connectToHostEncrypted(m_host, m_port);
}

void CertificateInfoDialog::finishLookupWithError(const QString &reason)
{
    if (m_lookupFinished)
        return;
    m_lookupFinished = true;
    m_timeoutTimer.stop();
    if (m_socket)
        m_socket->abort();

    QStringList lines;
    lines << ztr("Host: %1").arg(m_host);
    lines << ztr("Port: %1").arg(m_port);
    lines << QString();
    lines << ztr("Could not retrieve TLS certificate details: %1").arg(reason);
    if (!m_sslWarnings.isEmpty()) {
        lines << QString();
        lines << ztr("TLS Warnings: %1").arg(m_sslWarnings.join(QStringLiteral("; ")));
    }
    m_detailsEdit->setPlainText(lines.join(QLatin1Char('\n')));
}

void CertificateInfoDialog::displayCertificateResult()
{
    if (m_lookupFinished || !m_socket)
        return;
    m_lookupFinished = true;
    m_timeoutTimer.stop();

    const QSslCertificate leaf = m_socket->peerCertificate();
    const QList<QSslCertificate> chain = m_socket->peerCertificateChain();
    const QSslCipher cipher = m_socket->sessionCipher();
    const QSsl::SslProtocol proto = m_socket->sessionProtocol();
    m_socket->disconnectFromHost();

    QStringList lines;
    lines << ztr("=== Connection ===");
    lines << ztr("Host: %1").arg(m_host);
    lines << ztr("Port: %1").arg(m_port);
    lines << ztr("Protocol: %1").arg(formatSslProtocol(proto));
    if (!cipher.isNull()) {
        lines << ztr("Cipher Suite: %1 (%2 bits)").arg(cipher.name()).arg(cipher.usedBits());
        if (!cipher.encryptionMethod().isEmpty() || !cipher.authenticationMethod().isEmpty()) {
            lines << ztr("Cipher Details: Encryption=%1, Auth=%2, KeyExchange=%3")
                         .arg(cipher.encryptionMethod(),
                              cipher.authenticationMethod(),
                              cipher.keyExchangeMethod());
        }
    }
    if (m_sslWarnings.isEmpty()) {
        lines << ztr("Verification: Trusted certificate chain");
    } else {
        lines << ztr("Verification: Warnings detected — %1").arg(m_sslWarnings.join(QStringLiteral("; ")));
    }

    if (leaf.isNull()) {
        lines << QString();
        lines << ztr("No peer certificate was presented by the server.");
        m_detailsEdit->setPlainText(lines.join(QLatin1Char('\n')));
        return;
    }

    lines << QString();
    lines << ztr("=== Issued To (Subject) ===");
    lines << ztr("Display Name: %1").arg(leaf.subjectDisplayName());
    lines << ztr("Common Name (CN): %1").arg(formatCertInfoList(leaf.subjectInfo(QSslCertificate::CommonName)));
    lines << ztr("Organization (O): %1").arg(formatCertInfoList(leaf.subjectInfo(QSslCertificate::Organization)));
    lines << ztr("Organizational Unit (OU): %1").arg(formatCertInfoList(leaf.subjectInfo(QSslCertificate::OrganizationalUnitName)));
    lines << ztr("Locality (L): %1").arg(formatCertInfoList(leaf.subjectInfo(QSslCertificate::LocalityName)));
    lines << ztr("State / Province (ST): %1").arg(formatCertInfoList(leaf.subjectInfo(QSslCertificate::StateOrProvinceName)));
    lines << ztr("Country (C): %1").arg(formatCertInfoList(leaf.subjectInfo(QSslCertificate::CountryName)));
    lines << ztr("Subject Alternative Names: %1").arg(formatSanMap(leaf.subjectAlternativeNames()));

    lines << QString();
    lines << ztr("=== Issued By (Issuer) ===");
    lines << ztr("Display Name: %1").arg(leaf.issuerDisplayName());
    lines << ztr("Common Name (CN): %1").arg(formatCertInfoList(leaf.issuerInfo(QSslCertificate::CommonName)));
    lines << ztr("Organization (O): %1").arg(formatCertInfoList(leaf.issuerInfo(QSslCertificate::Organization)));
    lines << ztr("Organizational Unit (OU): %1").arg(formatCertInfoList(leaf.issuerInfo(QSslCertificate::OrganizationalUnitName)));
    lines << ztr("Country (C): %1").arg(formatCertInfoList(leaf.issuerInfo(QSslCertificate::CountryName)));

    const QDateTime now = QDateTime::currentDateTimeUtc();
    const QDateTime eff = leaf.effectiveDate();
    const QDateTime exp = leaf.expiryDate();
    QString validityStatus;
    if (eff.isValid() && now < eff.toUTC())
        validityStatus = ztr("Not yet valid");
    else if (exp.isValid() && now > exp.toUTC())
        validityStatus = ztr("Expired");
    else if (!eff.isValid() || !exp.isValid())
        validityStatus = ztr("Unknown");
    else
        validityStatus = ztr("Valid");

    lines << QString();
    lines << ztr("=== Validity ===");
    lines << ztr("Status: %1").arg(validityStatus);
    lines << ztr("Issued On: %1").arg(formatValidityRange(eff));
    lines << ztr("Expires On: %1").arg(formatValidityRange(exp));

    lines << QString();
    lines << ztr("=== Certificate Properties ===");
    lines << ztr("Version: %1").arg(leaf.version().isEmpty() ? QStringLiteral("—") : QString::fromLatin1(leaf.version()));
    lines << ztr("Serial Number: %1").arg(leaf.serialNumber().isEmpty() ? QStringLiteral("—") : QString::fromLatin1(leaf.serialNumber()));
    lines << ztr("Public Key: %1").arg(formatPublicKeyInfo(leaf.publicKey()));
    lines << ztr("Self-Signed: %1").arg(leaf.isSelfSigned() ? ztr("Yes") : ztr("No"));
    lines << ztr("SHA-256 Fingerprint: %1").arg(formatHexFingerprint(leaf.digest(QCryptographicHash::Sha256)));
    lines << ztr("SHA-1 Fingerprint: %1").arg(formatHexFingerprint(leaf.digest(QCryptographicHash::Sha1)));

    const QList<QSslCertificateExtension> exts = leaf.extensions();
    if (!exts.isEmpty()) {
        lines << QString();
        lines << ztr("=== Extensions ===");
        for (const QSslCertificateExtension &ext : exts) {
            const QString name = ext.name().isEmpty() ? ext.oid() : ext.name();
            lines << QStringLiteral("- %1 (%2)%3")
                         .arg(name, ext.oid(), ext.isCritical() ? QStringLiteral(" [critical]") : QString());
        }
    }

    if (!chain.isEmpty()) {
        lines << QString();
        lines << ztr("=== Certificate Chain (%1) ===").arg(chain.size());
        for (int i = 0; i < chain.size(); ++i) {
            const QSslCertificate &c = chain.at(i);
            lines << QStringLiteral("[%1] %2").arg(i).arg(c.subjectDisplayName());
            lines << QStringLiteral("    %1").arg(ztr("Issuer: %1").arg(c.issuerDisplayName()));
            lines << QStringLiteral("    %1").arg(ztr("Valid: %1 -> %2")
                .arg(c.effectiveDate().toUTC().toString(QStringLiteral("yyyy-MM-dd")),
                     c.expiryDate().toUTC().toString(QStringLiteral("yyyy-MM-dd"))));
            lines << QStringLiteral("    %1").arg(ztr("SHA-256: %1")
                .arg(formatHexFingerprint(c.digest(QCryptographicHash::Sha256))));
        }
    }

    const QString textDump = leaf.toText().trimmed();
    if (!textDump.isEmpty()) {
        lines << QString();
        lines << ztr("=== Raw Certificate Text ===");
        lines << textDump;
    }

    const QByteArray pem = leaf.toPem().trimmed();
    if (!pem.isEmpty()) {
        lines << QString();
        lines << ztr("=== PEM Certificate ===");
        lines << QString::fromLatin1(pem);
    }

    m_detailsEdit->setPlainText(lines.join(QLatin1Char('\n')));
}
