#include "webview.h"

#include <QContextMenuEvent>
#include <QWebEngineContextMenuRequest>
#include <QWebEngineProfile>

NetzacWebPage::NetzacWebPage(QWebEngineProfile *profile, QObject *parent)
    : QWebEnginePage(profile, parent)
{
}

void NetzacWebPage::setBuiltInPage(bool isBuiltIn)
{
    m_isBuiltInPage = isBuiltIn;
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
