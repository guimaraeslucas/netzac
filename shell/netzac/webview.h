#pragma once

#include <functional>

#include <QPoint>
#include <QUrl>
#include <QWebEnginePage>
#include <QWebEngineView>

class QContextMenuEvent;
class QWebEngineContextMenuRequest;
class QWebEngineProfile;

class NetzacWebPage : public QWebEnginePage {
    Q_OBJECT

public:
    explicit NetzacWebPage(QWebEngineProfile *profile, QObject *parent = nullptr);

    void setBuiltInPage(bool isBuiltIn);
    bool isBuiltInPage() const { return m_isBuiltInPage; }

signals:
    void mainFrameNavigationRequested(const QUrl &url, QWebEnginePage::NavigationType type);

protected:
    bool acceptNavigationRequest(const QUrl &url, NavigationType type, bool isMainFrame) override;

private:
    bool m_isBuiltInPage = false;
};

class NetzacWebView : public QWebEngineView {
    Q_OBJECT

public:
    using WindowCreator = std::function<QWebEngineView *(QWebEnginePage::WebWindowType)>;

    explicit NetzacWebView(QWidget *parent = nullptr);

    void setWindowCreator(WindowCreator creator);

signals:
    void contextMenuRequested(const QPoint &globalPos, QWebEngineContextMenuRequest *request);

protected:
    QWebEngineView *createWindow(QWebEnginePage::WebWindowType type) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

private:
    WindowCreator m_windowCreator;
};
