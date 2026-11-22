#include "webview.h"

#include <QContextMenuEvent>
#include <QUuid>
#include <QWebEngineContextMenuRequest>
#include <QWebEngineProfile>
#include <QWebEngineScript>
#include <QWebEngineScriptCollection>

namespace {

const QString &popupTokenPrefix()
{
    static const QString prefix = QStringLiteral("__NETZAC_POPUP_%1__:")
        .arg(QUuid::createUuid().toString(QUuid::Id128));
    return prefix;
}

QString platinumScrollbarCss()
{
    return QStringLiteral(R"CSS(
*, *::before, *::after {
  scrollbar-width: auto !important;
  scrollbar-color: auto !important;
}
::-webkit-scrollbar {
  width: 16px !important;
  height: 16px !important;
  background-color: #cccccc !important;
}
::-webkit-scrollbar-track {
  background-color: #cccccc !important;
  border: 1px solid #000000 !important;
  box-shadow: inset 1px 1px 0 #999999, inset -1px -1px 0 #eeeeee !important;
}
::-webkit-scrollbar-thumb {
  background-color: #ccccdd !important;
  border: 1px solid #000000 !important;
  box-shadow: inset 1px 1px 0 #ffffff, inset -1px -1px 0 #777788 !important;
  background-repeat: no-repeat !important;
  background-position: center !important;
}
::-webkit-scrollbar-thumb:vertical {
  min-height: 24px !important;
  background-image: url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' width='8' height='8'%3E%3Crect x='0' y='0' width='8' height='1' fill='%23ffffff'/%3E%3Crect x='0' y='1' width='8' height='1' fill='%23666677'/%3E%3Crect x='0' y='3' width='8' height='1' fill='%23ffffff'/%3E%3Crect x='0' y='4' width='8' height='1' fill='%23666677'/%3E%3Crect x='0' y='6' width='8' height='1' fill='%23ffffff'/%3E%3Crect x='0' y='7' width='8' height='1' fill='%23666677'/%3E%3C/svg%3E") !important;
}
::-webkit-scrollbar-thumb:horizontal {
  min-width: 24px !important;
  background-image: url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' width='8' height='8'%3E%3Crect x='0' y='0' width='1' height='8' fill='%23ffffff'/%3E%3Crect x='1' y='0' width='1' height='8' fill='%23666677'/%3E%3Crect x='3' y='0' width='1' height='8' fill='%23ffffff'/%3E%3Crect x='4' y='0' width='1' height='8' fill='%23666677'/%3E%3Crect x='6' y='0' width='1' height='8' fill='%23ffffff'/%3E%3Crect x='7' y='0' width='1' height='8' fill='%23666677'/%3E%3C/svg%3E") !important;
}
::-webkit-scrollbar-thumb:hover {
  background-color: #d8d8ea !important;
}
::-webkit-scrollbar-thumb:active {
  background-color: #9999bb !important;
  box-shadow: inset 1px 1px 0 #666688, inset -1px -1px 0 #ddddee !important;
}
::-webkit-scrollbar-button:single-button {
  display: block !important;
  width: 16px !important;
  height: 16px !important;
  background-color: #dddddd !important;
  border: 1px solid #000000 !important;
  box-shadow: inset 1px 1px 0 #ffffff, inset -1px -1px 0 #888888 !important;
  background-repeat: no-repeat !important;
  background-position: center !important;
}
::-webkit-scrollbar-button:single-button:active {
  background-color: #bbbbcc !important;
  box-shadow: inset 1px 1px 0 #777777, inset -1px -1px 0 #eeeeee !important;
}
::-webkit-scrollbar-button:single-button:vertical:decrement {
  background-image: url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' width='9' height='9'%3E%3Cpolygon points='4,2 8,6 0,6' fill='%23000000'/%3E%3C/svg%3E") !important;
}
::-webkit-scrollbar-button:single-button:vertical:increment {
  background-image: url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' width='9' height='9'%3E%3Cpolygon points='0,3 8,3 4,7' fill='%23000000'/%3E%3C/svg%3E") !important;
}
::-webkit-scrollbar-button:single-button:horizontal:decrement {
  background-image: url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' width='9' height='9'%3E%3Cpolygon points='2,4 6,0 6,8' fill='%23000000'/%3E%3C/svg%3E") !important;
}
::-webkit-scrollbar-button:single-button:horizontal:increment {
  background-image: url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' width='9' height='9'%3E%3Cpolygon points='3,0 7,4 3,8' fill='%23000000'/%3E%3C/svg%3E") !important;
}
::-webkit-scrollbar-corner {
  background-color: #dddddd !important;
  border: 1px solid #000000 !important;
  box-shadow: inset 1px 1px 0 #ffffff, inset -1px -1px 0 #888888 !important;
}
)CSS");
}

}  // namespace

void installNetzacScripts(QWebEngineProfile *profile)
{
    if (!profile)
        return;

    QWebEngineScriptCollection *scripts = profile->scripts();
    if (!scripts)
        return;

    const QString scrollbarScriptName = QStringLiteral("netzac-platinum-scrollbars");
    if (scripts->find(scrollbarScriptName).isEmpty()) {
        QString css = platinumScrollbarCss();
        css.replace(QLatin1Char('\\'), QStringLiteral("\\\\"));
        css.replace(QLatin1Char('`'), QStringLiteral("\\`"));
        css.replace(QStringLiteral("${"), QStringLiteral("\\${"));

        const QString js = QStringLiteral(
            "(function() {\n"
            "  var id = 'zacos9-platinum-scrollbars';\n"
            "  var css = `%1`;\n"
            "  function ensureStyle() {\n"
            "    if (document.getElementById(id)) return;\n"
            "    var target = document.head || document.documentElement;\n"
            "    if (!target) return;\n"
            "    var st = document.createElement('style');\n"
            "    st.id = id;\n"
            "    st.textContent = css;\n"
            "    target.appendChild(st);\n"
            "  }\n"
            "  ensureStyle();\n"
            "  if (document.readyState === 'loading') {\n"
            "    document.addEventListener('DOMContentLoaded', ensureStyle, { once: true });\n"
            "  }\n"
            "})();\n").arg(css);

        QWebEngineScript script;
        script.setName(scrollbarScriptName);
        script.setSourceCode(js);
        script.setInjectionPoint(QWebEngineScript::DocumentCreation);
        script.setRunsOnSubFrames(true);
        script.setWorldId(QWebEngineScript::ApplicationWorld);
        scripts->insert(script);
    }

    const QString popupScriptName = QStringLiteral("netzac-window-open-hook");
    if (scripts->find(popupScriptName).isEmpty()) {
        QString prefix = popupTokenPrefix();
        prefix.replace(QLatin1Char('\''), QStringLiteral("\\'"));
        const QString js = QStringLiteral(
            "(function() {\n"
            "  if (window.__netzacOpenHooked) return;\n"
            "  window.__netzacOpenHooked = true;\n"
            "  var origOpen = window.open;\n"
            "  var prefix = '%1';\n"
            "  window.open = function(url, target, features) {\n"
            "    if (typeof features === 'string' && features.length > 0) {\n"
            "      console.debug(prefix + features);\n"
            "    }\n"
            "    return origOpen.apply(this, arguments);\n"
            "  };\n"
            "})();\n").arg(prefix);

        QWebEngineScript script;
        script.setName(popupScriptName);
        script.setSourceCode(js);
        script.setInjectionPoint(QWebEngineScript::DocumentCreation);
        script.setRunsOnSubFrames(true);
        script.setWorldId(QWebEngineScript::MainWorld);
        scripts->insert(script);
    }
}

NetzacWebPage::NetzacWebPage(QWebEngineProfile *profile, QObject *parent)
    : QWebEnginePage(profile, parent)
{
}

void NetzacWebPage::setBuiltInPage(bool isBuiltIn)
{
    m_isBuiltInPage = isBuiltIn;
}

QString NetzacWebPage::takePendingPopupFeatures()
{
    QString out = m_pendingPopupFeatures;
    m_pendingPopupFeatures.clear();
    return out;
}

void NetzacWebPage::javaScriptConsoleMessage(
    JavaScriptConsoleMessageLevel level,
    const QString &message,
    int lineNumber,
    const QString &sourceID)
{
    const QString &prefix = popupTokenPrefix();
    if (message.startsWith(prefix)) {
        m_pendingPopupFeatures = message.mid(prefix.size()).trimmed();
        return;
    }
    QWebEnginePage::javaScriptConsoleMessage(level, message, lineNumber, sourceID);
}

bool NetzacWebPage::acceptNavigationRequest(
    const QUrl &url,
    NavigationType type,
    bool isMainFrame)
{
    if (isMainFrame) {
        const QString urlStr = url.toString();
        const QString scheme = url.scheme().toLower();
        if (m_isBuiltInPage) {
            const bool stillBuiltinDoc = (type != NavigationTypeLinkClicked)
                && (urlStr == QLatin1String("https://localhost/")
                    || scheme == QLatin1String("data")
                    || scheme == QLatin1String("about"));
            if (!stillBuiltinDoc)
                m_isBuiltInPage = false;
        }
        emit mainFrameNavigationRequested(url, type);
    }
    return QWebEnginePage::acceptNavigationRequest(url, type, isMainFrame);
}

NetzacWebView::NetzacWebView(QWidget *parent)
    : QWebEngineView(parent)
{
}

void NetzacWebView::setWindowCreator(WindowCreator creator)
{
    m_windowCreator = std::move(creator);
}

QWebEngineView *NetzacWebView::createWindow(QWebEnginePage::WebWindowType type)
{
    if (m_windowCreator)
        return m_windowCreator(type);
    return nullptr;
}

void NetzacWebView::contextMenuEvent(QContextMenuEvent *event)
{
    event->accept();
    emit contextMenuRequested(event->globalPos(), lastContextMenuRequest());
}
