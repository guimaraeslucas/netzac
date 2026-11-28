#include "webview.h"

#include "preferences.h"

#include <QApplication>
#include <QContextMenuEvent>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPalette>
#include <QPointer>
#include <QTimer>
#include <QUuid>
#include <QWebEngineContextMenuRequest>
#include <QWebEngineProfile>
#include <QWebEngineScript>
#include <QWebEngineScriptCollection>
#include <QWebEngineSettings>

namespace {

const QString &popupTokenPrefix()
{
    static const QString prefix = QStringLiteral("__NETZAC_POPUP_%1__:")
        .arg(QUuid::createUuid().toString(QUuid::Id128));
    return prefix;
}

const QString &contextMenuTokenPrefix()
{
    static const QString prefix = QStringLiteral("__NETZAC_CTX_%1__:")
        .arg(QUuid::createUuid().toString(QUuid::Id128));
    return prefix;
}

struct PlatinumAccentRamp {
    const char *id;
    const char *gripHi;
    const char *light;
    const char *body;
    const char *dark;
    const char *shadow;
};

PlatinumAccentRamp currentZacosAccentRamp(const QString &preferredAccentId = QString())
{
    static constexpr PlatinumAccentRamp kAccents[] = {
        {"lavender", "#eeeeee", "#ccccff", "#9999ff", "#6666cc", "#333399"},
        {"graphite", "#eeeeee", "#e6e6e6", "#cccccc", "#999999", "#666666"},
        {"gold",     "#eeeeee", "#fff2cc", "#ffe699", "#ccb366", "#998033"},
        {"bronze",   "#eeeeee", "#f4e3d7", "#e8c7b0", "#b5947d", "#82614a"},
        {"ivy",      "#ccffcc", "#66ff99", "#33cc66", "#339966", "#006633"},
        {"ocean",    "#eeeeee", "#cfedfc", "#9edbfa", "#6ba8c7", "#387594"},
        {"rose",     "#eeeeee", "#f9d2df", "#f2a6bf", "#bf738c", "#8c4059"},
        {"plum",     "#eeeeee", "#f0d6f5", "#e0adeb", "#ad7ab8", "#7a4785"},
    };

    const QString overrideId = preferredAccentId.trimmed().toLower();
    if (!overrideId.isEmpty() && overrideId != QLatin1String("system")) {
        for (const PlatinumAccentRamp &ramp : kAccents) {
            if (overrideId == QLatin1String(ramp.id))
                return ramp;
        }
    }

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

    for (const PlatinumAccentRamp &ramp : kAccents) {
        if (accentId == QLatin1String(ramp.id))
            return ramp;
    }
    return kAccents[0];
}

QString platinumScrollbarCss(const QString &accentId = QString())
{
    const PlatinumAccentRamp ac = currentZacosAccentRamp(accentId);
    auto esc = [](const char *hex) {
        return QString::fromLatin1(hex).replace(QLatin1Char('#'), QStringLiteral("%23"));
    };

    // Note: Do NOT set scrollbar-color to explicit colors here; in Chromium, a non-auto
    // scrollbar-color disables all ::-webkit-scrollbar pseudo-elements (hiding arrow buttons
    // and the 3D Platinum thumb/caret). Setting both to 'auto !important' neutralizes page
    // overrides so the full ::-webkit-scrollbar Platinum styling always renders.
    QString css = QStringLiteral(R"CSS(
*, *::before, *::after, html, body {
  scrollbar-width: auto !important;
  scrollbar-color: auto !important;
}
::-webkit-scrollbar {
  width: 16px !important;
  height: 16px !important;
  background-color: #aaaaaa !important;
  -webkit-appearance: none !important;
}
::-webkit-scrollbar-track {
  background-color: #aaaaaa !important;
  border-radius: 0 !important;
}
::-webkit-scrollbar-track:vertical {
  border-left: 1px solid #000000 !important;
  border-right: 1px solid #000000 !important;
  border-top: none !important;
  border-bottom: none !important;
  box-shadow: inset 1px 1px 0 #777777, inset 2px 2px 0 #888888, inset -1px 0 0 #cccccc, inset -2px 0 0 #bbbbbb !important;
}
::-webkit-scrollbar-track:horizontal {
  border-top: 1px solid #000000 !important;
  border-bottom: 1px solid #000000 !important;
  border-left: none !important;
  border-right: none !important;
  box-shadow: inset 1px 1px 0 #777777, inset 2px 2px 0 #888888, inset 0 -1px 0 #cccccc, inset 0 -2px 0 #bbbbbb !important;
}
::-webkit-scrollbar-track-piece {
  background-color: transparent !important;
  border: none !important;
}
::-webkit-scrollbar-thumb {
  background-color: __BODY__ !important;
  border: 1px solid #000000 !important;
  border-radius: 0 !important;
  background-repeat: no-repeat !important;
  background-position: center !important;
}
::-webkit-scrollbar-thumb:vertical {
  min-height: 18px !important;
  box-shadow: inset 1px 1px 0 __LIGHT__, inset -1px -1px 0 __DARK__, 0 1px 0 #777777, 0 2px 0 #888888 !important;
  background-image: url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' width='8' height='8' shape-rendering='crispEdges'%3E%3Crect x='0' y='0' width='1' height='1' fill='__GRIP_HI_ENC__'/%3E%3Crect x='1' y='0' width='6' height='1' fill='__LIGHT_ENC__'/%3E%3Crect x='1' y='1' width='7' height='1' fill='__SHADOW_ENC__'/%3E%3Crect x='0' y='2' width='1' height='1' fill='__GRIP_HI_ENC__'/%3E%3Crect x='1' y='2' width='6' height='1' fill='__LIGHT_ENC__'/%3E%3Crect x='1' y='3' width='7' height='1' fill='__SHADOW_ENC__'/%3E%3Crect x='0' y='4' width='1' height='1' fill='__GRIP_HI_ENC__'/%3E%3Crect x='1' y='4' width='6' height='1' fill='__LIGHT_ENC__'/%3E%3Crect x='1' y='5' width='7' height='1' fill='__SHADOW_ENC__'/%3E%3Crect x='0' y='6' width='1' height='1' fill='__GRIP_HI_ENC__'/%3E%3Crect x='1' y='6' width='6' height='1' fill='__LIGHT_ENC__'/%3E%3Crect x='1' y='7' width='7' height='1' fill='__SHADOW_ENC__'/%3E%3C/svg%3E") !important;
}
::-webkit-scrollbar-thumb:horizontal {
  min-width: 18px !important;
  box-shadow: inset 1px 1px 0 __LIGHT__, inset -1px -1px 0 __DARK__, 1px 0 0 #777777, 2px 0 0 #888888 !important;
  background-image: url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' width='8' height='8' shape-rendering='crispEdges'%3E%3Crect x='0' y='0' width='1' height='1' fill='__GRIP_HI_ENC__'/%3E%3Crect x='0' y='1' width='1' height='6' fill='__LIGHT_ENC__'/%3E%3Crect x='1' y='1' width='1' height='7' fill='__SHADOW_ENC__'/%3E%3Crect x='2' y='0' width='1' height='1' fill='__GRIP_HI_ENC__'/%3E%3Crect x='2' y='1' width='1' height='6' fill='__LIGHT_ENC__'/%3E%3Crect x='3' y='1' width='1' height='7' fill='__SHADOW_ENC__'/%3E%3Crect x='4' y='0' width='1' height='1' fill='__GRIP_HI_ENC__'/%3E%3Crect x='4' y='1' width='1' height='6' fill='__LIGHT_ENC__'/%3E%3Crect x='5' y='1' width='1' height='7' fill='__SHADOW_ENC__'/%3E%3Crect x='6' y='0' width='1' height='1' fill='__GRIP_HI_ENC__'/%3E%3Crect x='6' y='1' width='1' height='6' fill='__LIGHT_ENC__'/%3E%3Crect x='7' y='1' width='1' height='7' fill='__SHADOW_ENC__'/%3E%3C/svg%3E") !important;
}
::-webkit-scrollbar-thumb:hover {
  background-color: __BODY__ !important;
}
::-webkit-scrollbar-thumb:vertical:active {
  background-color: __DARK__ !important;
  box-shadow: inset 1px 1px 0 __SHADOW__, inset -1px -1px 0 __LIGHT__, 0 1px 0 #777777, 0 2px 0 #888888 !important;
}
::-webkit-scrollbar-thumb:horizontal:active {
  background-color: __DARK__ !important;
  box-shadow: inset 1px 1px 0 __SHADOW__, inset -1px -1px 0 __LIGHT__, 1px 0 0 #777777, 2px 0 0 #888888 !important;
}
::-webkit-scrollbar-button {
  display: none !important;
  width: 0 !important;
  height: 0 !important;
}
::-webkit-scrollbar-button:single-button,
::-webkit-scrollbar-button:vertical:start:decrement,
::-webkit-scrollbar-button:vertical:end:increment,
::-webkit-scrollbar-button:horizontal:start:decrement,
::-webkit-scrollbar-button:horizontal:end:increment {
  display: block !important;
  width: 16px !important;
  height: 16px !important;
  background-color: #dddddd !important;
  border: 1px solid #000000 !important;
  border-radius: 0 !important;
  box-shadow: inset 1px 1px 0 #ffffff, inset -1px -1px 0 #bbbbbb !important;
  background-repeat: no-repeat !important;
  background-position: center !important;
}
::-webkit-scrollbar-button:single-button:hover,
::-webkit-scrollbar-button:vertical:start:decrement:hover,
::-webkit-scrollbar-button:vertical:end:increment:hover,
::-webkit-scrollbar-button:horizontal:start:decrement:hover,
::-webkit-scrollbar-button:horizontal:end:increment:hover {
  background-color: #e6e6e6 !important;
}
::-webkit-scrollbar-button:single-button:active,
::-webkit-scrollbar-button:vertical:start:decrement:active,
::-webkit-scrollbar-button:vertical:end:increment:active,
::-webkit-scrollbar-button:horizontal:start:decrement:active,
::-webkit-scrollbar-button:horizontal:end:increment:active {
  background-color: #999999 !important;
  box-shadow: inset 1px 1px 0 #666666, inset -1px -1px 0 #cccccc !important;
}
::-webkit-scrollbar-button:vertical:decrement {
  background-image: url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' width='14' height='14' shape-rendering='crispEdges'%3E%3Crect x='6' y='5' width='2' height='1' fill='%23000000'/%3E%3Crect x='5' y='6' width='4' height='1' fill='%23000000'/%3E%3Crect x='4' y='7' width='6' height='1' fill='%23000000'/%3E%3Crect x='3' y='8' width='8' height='1' fill='%23000000'/%3E%3C/svg%3E") !important;
}
::-webkit-scrollbar-button:vertical:increment {
  background-image: url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' width='14' height='14' shape-rendering='crispEdges'%3E%3Crect x='3' y='5' width='8' height='1' fill='%23000000'/%3E%3Crect x='4' y='6' width='6' height='1' fill='%23000000'/%3E%3Crect x='5' y='7' width='4' height='1' fill='%23000000'/%3E%3Crect x='6' y='8' width='2' height='1' fill='%23000000'/%3E%3C/svg%3E") !important;
}
::-webkit-scrollbar-button:horizontal:decrement {
  background-image: url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' width='14' height='14' shape-rendering='crispEdges'%3E%3Crect x='5' y='6' width='1' height='2' fill='%23000000'/%3E%3Crect x='6' y='5' width='1' height='4' fill='%23000000'/%3E%3Crect x='7' y='4' width='1' height='6' fill='%23000000'/%3E%3Crect x='8' y='3' width='1' height='8' fill='%23000000'/%3E%3C/svg%3E") !important;
}
::-webkit-scrollbar-button:horizontal:increment {
  background-image: url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' width='14' height='14' shape-rendering='crispEdges'%3E%3Crect x='5' y='3' width='1' height='8' fill='%23000000'/%3E%3Crect x='6' y='4' width='1' height='6' fill='%23000000'/%3E%3Crect x='7' y='5' width='1' height='4' fill='%23000000'/%3E%3Crect x='8' y='6' width='1' height='2' fill='%23000000'/%3E%3C/svg%3E") !important;
}
::-webkit-scrollbar-corner {
  background-color: #dddddd !important;
  border: 1px solid #000000 !important;
  box-shadow: inset 1px 1px 0 #ffffff, inset -1px -1px 0 #bbbbbb !important;
}
)CSS");

    css.replace(QStringLiteral("__BODY__"), QString::fromLatin1(ac.body));
    css.replace(QStringLiteral("__LIGHT__"), QString::fromLatin1(ac.light));
    css.replace(QStringLiteral("__DARK__"), QString::fromLatin1(ac.dark));
    css.replace(QStringLiteral("__SHADOW__"), QString::fromLatin1(ac.shadow));
    css.replace(QStringLiteral("__GRIP_HI_ENC__"), esc(ac.gripHi));
    css.replace(QStringLiteral("__LIGHT_ENC__"), esc(ac.light));
    css.replace(QStringLiteral("__SHADOW_ENC__"), esc(ac.shadow));
    return css;
}

QString escapeCssFontFamily(QString family)
{
    family.replace(QLatin1Char('\\'), QStringLiteral("\\\\"));
    family.replace(QLatin1Char('"'), QStringLiteral("\\\""));
    return family;
}

QString escapeJsTemplateLiteral(QString text)
{
    text.replace(QLatin1Char('\\'), QStringLiteral("\\\\"));
    text.replace(QLatin1Char('`'), QStringLiteral("\\`"));
    text.replace(QStringLiteral("${"), QStringLiteral("\\${"));
    return text;
}

QString buildCustomFontsCss(const NetzacPreferences &prefs)
{
    if (!prefs.forceCustomFonts)
        return QString();

    const int sz = qBound(8, prefs.defaultFontSize, 36);
    const int fixedSz = qMax(8, sz - 1);

    const bool preferSerif = (prefs.proportionalFontStyle == NetzacPreferences::ProportionalFontStyle::Serif);
    const QString primaryRaw = preferSerif
        ? (!prefs.serifFont.isEmpty() ? prefs.serifFont : prefs.sansSerifFont)
        : (!prefs.sansSerifFont.isEmpty() ? prefs.sansSerifFont : prefs.serifFont);
    const QString primaryGeneric = (preferSerif || prefs.sansSerifFont.isEmpty())
        ? QStringLiteral("serif")
        : QStringLiteral("sans-serif");

    QString css;
    if (!primaryRaw.isEmpty()) {
        const QString primary = escapeCssFontFamily(primaryRaw);
        css += QStringLiteral(
            "html, body { font-family: \"%1\", %2 !important; }\n"
            "body, p, div, li, td, th, dd, dt, a, blockquote, article, section, main, "
            "aside, header, footer, nav, label, button, input, select, textarea, "
            "h1, h2, h3, h4, h5, h6, "
            "span:not([class*=\"icon\"]):not([class*=\"fa\"]):not([class*=\"glyph\"]) "
            "{ font-family: \"%1\", %2 !important; }\n")
            .arg(primary, primaryGeneric);
    }

    if (!prefs.serifFont.isEmpty() && !prefs.sansSerifFont.isEmpty()) {
        if (!preferSerif) {
            css += QStringLiteral("blockquote, q, cite { font-family: \"%1\", serif !important; }\n")
                .arg(escapeCssFontFamily(prefs.serifFont));
        } else {
            css += QStringLiteral("nav, button, input, select, label { font-family: \"%1\", sans-serif !important; }\n")
                .arg(escapeCssFontFamily(prefs.sansSerifFont));
        }
    }

    if (!prefs.fixedFont.isEmpty()) {
        css += QStringLiteral("pre, code, kbd, samp, tt, var, pre *, code * { font-family: \"%1\", monospace !important; }\n")
            .arg(escapeCssFontFamily(prefs.fixedFont));
    }

    css += QStringLiteral(
        "html, body { font-size: %1px !important; }\n"
        "p, li, td, th, dd, dt, blockquote { font-size: %1px !important; }\n"
        "pre, code, kbd, samp, tt { font-size: %2px !important; }\n")
        .arg(sz)
        .arg(fixedSz);

    return css;
}

}  // namespace

void installNetzacScripts(QWebEngineProfile *profile, const NetzacPreferences &prefs)
{
    if (!profile)
        return;

    QWebEngineScriptCollection *scripts = profile->scripts();
    if (!scripts)
        return;

    const QString scrollbarScriptName = QStringLiteral("netzac-platinum-scrollbars");
    for (const QWebEngineScript &old : scripts->find(scrollbarScriptName))
        scripts->remove(old);
    if (prefs.enablePlatinumScrollbars) {
        const QString css = escapeJsTemplateLiteral(platinumScrollbarCss(prefs.scrollbarAccent));

        // Inject the Platinum scrollbar style at the earliest possible moment. At
        // DocumentCreation the parser has usually not created <html>/<head> yet, so
        // waiting for DOMContentLoaded would show native scrollbars until the page
        // finishes parsing. A MutationObserver on the document plus a pre-paint
        // requestAnimationFrame loop apply the style as soon as <html> exists.
        const QString js = QStringLiteral(
            "(function() {\n"
            "  if (window.__netzacScrollbarCleanup) { window.__netzacScrollbarCleanup(); }\n"
            "  var id = 'zacos9-platinum-scrollbars';\n"
            "  var css = `%1`;\n"
            "  var guard = null;\n"
            "  var waiter = null;\n"
            "  var stopped = false;\n"
            "  function apply() {\n"
            "    if (stopped) return true;\n"
            "    if (document.getElementById(id)) return true;\n"
            "    var parent = document.head || document.documentElement;\n"
            "    if (!parent) return false;\n"
            "    var st = document.createElement('style');\n"
            "    st.id = id;\n"
            "    st.textContent = css;\n"
            "    parent.appendChild(st);\n"
            "    return true;\n"
            "  }\n"
            "  function watch() {\n"
            "    if (stopped || guard || typeof MutationObserver === 'undefined') return;\n"
            "    var head = document.head;\n"
            "    if (!head) return;\n"
            "    guard = new MutationObserver(function() {\n"
            "      if (!stopped && !document.getElementById(id)) apply();\n"
            "    });\n"
            "    guard.observe(head, { childList: true });\n"
            "  }\n"
            "  function attempt() {\n"
            "    if (stopped) return true;\n"
            "    if (!apply()) return false;\n"
            "    if (waiter) { waiter.disconnect(); waiter = null; }\n"
            "    watch();\n"
            "    return true;\n"
            "  }\n"
            "  window.__netzacScrollbarCleanup = function() {\n"
            "    stopped = true;\n"
            "    if (guard) { guard.disconnect(); guard = null; }\n"
            "    if (waiter) { waiter.disconnect(); waiter = null; }\n"
            "    var el = document.getElementById(id);\n"
            "    if (el && el.parentNode) el.parentNode.removeChild(el);\n"
            "  };\n"
            "  if (!attempt() && typeof MutationObserver !== 'undefined') {\n"
            "    waiter = new MutationObserver(function() { attempt(); });\n"
            "    waiter.observe(document, { childList: true, subtree: true });\n"
            "  }\n"
            "  document.addEventListener('readystatechange', attempt);\n"
            "  document.addEventListener('DOMContentLoaded', attempt);\n"
            "  window.addEventListener('load', attempt);\n"
            "  var frames = 0;\n"
            "  (function raf() {\n"
            "    if (stopped || document.getElementById(id)) return;\n"
            "    attempt();\n"
            "    if (!stopped && ++frames < 300) requestAnimationFrame(raf);\n"
            "  })();\n"
            "})();\n").arg(css);

        QWebEngineScript script;
        script.setName(scrollbarScriptName);
        script.setSourceCode(js);
        script.setInjectionPoint(QWebEngineScript::DocumentCreation);
        script.setRunsOnSubFrames(true);
        script.setWorldId(QWebEngineScript::ApplicationWorld);
        scripts->insert(script);
    }

    const QString fontsScriptName = QStringLiteral("netzac-custom-fonts");
    for (const QWebEngineScript &old : scripts->find(fontsScriptName))
        scripts->remove(old);
    if (prefs.forceCustomFonts) {
        const QString fontCss = escapeJsTemplateLiteral(buildCustomFontsCss(prefs));
        const QString js = QStringLiteral(
            "(function() {\n"
            "  if (location.origin === 'https://localhost' && location.pathname === '/') return;\n"
            "  if (window.__netzacCustomFontsCleanup) { window.__netzacCustomFontsCleanup(); }\n"
            "  var id = 'zacos9-custom-fonts';\n"
            "  var css = `%1`;\n"
            "  var guard = null;\n"
            "  var waiter = null;\n"
            "  var stopped = false;\n"
            "  function apply() {\n"
            "    if (stopped) return true;\n"
            "    if (document.getElementById(id)) return true;\n"
            "    var parent = document.head || document.documentElement;\n"
            "    if (!parent) return false;\n"
            "    var st = document.createElement('style');\n"
            "    st.id = id;\n"
            "    st.textContent = css;\n"
            "    parent.insertBefore(st, parent.firstChild);\n"
            "    return true;\n"
            "  }\n"
            "  function watch() {\n"
            "    if (stopped || guard || typeof MutationObserver === 'undefined') return;\n"
            "    var head = document.head;\n"
            "    if (!head) return;\n"
            "    guard = new MutationObserver(function() {\n"
            "      if (!stopped && !document.getElementById(id)) apply();\n"
            "    });\n"
            "    guard.observe(head, { childList: true });\n"
            "  }\n"
            "  function attempt() {\n"
            "    if (stopped) return true;\n"
            "    if (!apply()) return false;\n"
            "    if (waiter) { waiter.disconnect(); waiter = null; }\n"
            "    watch();\n"
            "    return true;\n"
            "  }\n"
            "  window.__netzacCustomFontsCleanup = function() {\n"
            "    stopped = true;\n"
            "    if (guard) { guard.disconnect(); guard = null; }\n"
            "    if (waiter) { waiter.disconnect(); waiter = null; }\n"
            "    var el = document.getElementById(id);\n"
            "    if (el && el.parentNode) el.parentNode.removeChild(el);\n"
            "  };\n"
            "  if (!attempt() && typeof MutationObserver !== 'undefined') {\n"
            "    waiter = new MutationObserver(function() { attempt(); });\n"
            "    waiter.observe(document, { childList: true, subtree: true });\n"
            "  }\n"
            "  document.addEventListener('readystatechange', attempt);\n"
            "  document.addEventListener('DOMContentLoaded', attempt);\n"
            "  window.addEventListener('load', attempt);\n"
            "  var frames = 0;\n"
            "  (function raf() {\n"
            "    if (stopped || document.getElementById(id)) return;\n"
            "    attempt();\n"
            "    if (!stopped && ++frames < 300) requestAnimationFrame(raf);\n"
            "  })();\n"
            "})();\n").arg(fontCss);

        QWebEngineScript script;
        script.setName(fontsScriptName);
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

    const QString ctxGuardName = QStringLiteral("netzac-contextmenu-guard");
    if (scripts->find(ctxGuardName).isEmpty()) {
        QString prefix = contextMenuTokenPrefix();
        prefix.replace(QLatin1Char('\''), QStringLiteral("\\'"));
        const QString js = QStringLiteral(
            "(function() {\n"
            "  if (window.__netzacCtxGuardInstalled) return;\n"
            "  window.__netzacCtxGuardInstalled = true;\n"
            "  window.__netzacCtxPrevented = false;\n"
            "  window.__netzacCtxPreventedAt = 0;\n"
            "  var prefix = '%1';\n"
            "  window.addEventListener('contextmenu', function(e) {\n"
            "    window.__netzacCtxPrevented = false;\n"
            "    window.__netzacCtxPreventedAt = 0;\n"
            "    Promise.resolve().then(function() {\n"
            "      var prevented = !!e.defaultPrevented;\n"
            "      window.__netzacCtxPrevented = prevented;\n"
            "      window.__netzacCtxPreventedAt = prevented ? performance.now() : 0;\n"
            "      console.debug(prefix + (prevented ? '1' : '0'));\n"
            "    });\n"
            "  }, true);\n"
            "})();\n").arg(prefix);

        QWebEngineScript script;
        script.setName(ctxGuardName);
        script.setSourceCode(js);
        script.setInjectionPoint(QWebEngineScript::DocumentCreation);
        script.setRunsOnSubFrames(true);
        script.setWorldId(QWebEngineScript::ApplicationWorld);
        scripts->insert(script);
    }
}

void applyAppearanceToPage(QWebEnginePage *page, const NetzacPreferences &prefs)
{
    if (!page)
        return;

    prefs.applyFontSettings(page->settings());

    const QString sbEnabled = prefs.enablePlatinumScrollbars ? QStringLiteral("true") : QStringLiteral("false");
    const QString sbCss = prefs.enablePlatinumScrollbars
        ? escapeJsTemplateLiteral(platinumScrollbarCss(prefs.scrollbarAccent))
        : QString();
    const QString fontsEnabled = prefs.forceCustomFonts ? QStringLiteral("true") : QStringLiteral("false");
    const QString fontsCss = prefs.forceCustomFonts
        ? escapeJsTemplateLiteral(buildCustomFontsCss(prefs))
        : QString();

    const QString js = QStringLiteral(
        "(function() {\n"
        "  var sbId = 'zacos9-platinum-scrollbars';\n"
        "  if (%1) {\n"
        "    var sbEl = document.getElementById(sbId);\n"
        "    if (sbEl) {\n"
        "      sbEl.textContent = `%2`;\n"
        "    } else {\n"
        "      var p = document.head || document.documentElement;\n"
        "      if (p) {\n"
        "        sbEl = document.createElement('style');\n"
        "        sbEl.id = sbId;\n"
        "        sbEl.textContent = `%2`;\n"
        "        p.appendChild(sbEl);\n"
        "      }\n"
        "    }\n"
        "  } else {\n"
        "    if (window.__netzacScrollbarCleanup) { window.__netzacScrollbarCleanup(); }\n"
        "    var oldSb = document.getElementById(sbId);\n"
        "    if (oldSb && oldSb.parentNode) oldSb.parentNode.removeChild(oldSb);\n"
        "  }\n"
        "  var fontId = 'zacos9-custom-fonts';\n"
        "  var isStart = (location.origin === 'https://localhost' && location.pathname === '/');\n"
        "  if (%3 && !isStart) {\n"
        "    var fEl = document.getElementById(fontId);\n"
        "    if (fEl) {\n"
        "      fEl.textContent = `%4`;\n"
        "    } else {\n"
        "      var p2 = document.head || document.documentElement;\n"
        "      if (p2) {\n"
        "        fEl = document.createElement('style');\n"
        "        fEl.id = fontId;\n"
        "        fEl.textContent = `%4`;\n"
        "        p2.insertBefore(fEl, p2.firstChild);\n"
        "      }\n"
        "    }\n"
        "  } else {\n"
        "    if (window.__netzacCustomFontsCleanup) { window.__netzacCustomFontsCleanup(); }\n"
        "    var oldF = document.getElementById(fontId);\n"
        "    if (oldF && oldF.parentNode) oldF.parentNode.removeChild(oldF);\n"
        "  }\n"
        "})();\n")
        .arg(sbEnabled, sbCss, fontsEnabled, fontsCss);

    page->runJavaScript(js, QWebEngineScript::ApplicationWorld);
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

bool NetzacWebPage::wasContextMenuPreventedRecently() const
{
    return m_lastContextMenuPrevented
        && m_contextMenuPreventedTimer.isValid()
        && m_contextMenuPreventedTimer.elapsed() < 500;
}

void NetzacWebPage::resetContextMenuPrevented()
{
    m_lastContextMenuPrevented = false;
    m_contextMenuPreventedTimer.invalidate();
}

void NetzacWebPage::javaScriptConsoleMessage(
    JavaScriptConsoleMessageLevel level,
    const QString &message,
    int lineNumber,
    const QString &sourceID)
{
    const QString &ctxPrefix = contextMenuTokenPrefix();
    if (message.startsWith(ctxPrefix)) {
        const QString payload = message.mid(ctxPrefix.size()).trimmed();
        m_lastContextMenuPrevented = (payload == QLatin1String("1"));
        m_contextMenuPreventedTimer.restart();
        return;
    }
    const QString &prefix = popupTokenPrefix();
    if (message.startsWith(prefix)) {
        m_pendingPopupFeatures = message.mid(prefix.size()).trimmed();
        return;
    }
    static constexpr const char *kCredPrefix = "__NETZAC_CRED__:";
    if (message.startsWith(QLatin1String(kCredPrefix))) {
        const QByteArray jsonBytes = message.mid(int(strlen(kCredPrefix))).toUtf8();
        const QJsonDocument doc = QJsonDocument::fromJson(jsonBytes);
        if (doc.isObject()) {
            const QJsonObject obj = doc.object();
            const QString host = obj.value(QStringLiteral("host")).toString().trimmed();
            const QString user = obj.value(QStringLiteral("user")).toString().trimmed();
            const QString pass = obj.value(QStringLiteral("pass")).toString();
            if (!host.isEmpty() && !user.isEmpty() && !pass.isEmpty())
                emit credentialSubmitted(host, user, pass);
        }
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
        resetContextMenuPrevented();
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
    QWebEngineContextMenuRequest *req = lastContextMenuRequest();
    if (!req || event->spontaneous()) {
        event->ignore();
        return;
    }

    auto *netzacPage = qobject_cast<NetzacWebPage *>(page());
    if (netzacPage && netzacPage->wasContextMenuPreventedRecently()) {
        netzacPage->resetContextMenuPrevented();
        event->accept();
        return;
    }

    WebContextMenuData data;
    data.globalPos = event->globalPos();
    data.linkUrl = req->linkUrl();
    data.mediaType = req->mediaType();
    data.mediaUrl = req->mediaUrl();
    data.isContentEditable = req->isContentEditable();
    data.selectedText = req->selectedText();

    event->accept();

    if (netzacPage
        && !netzacPage->isBuiltInPage()
        && netzacPage->settings()->testAttribute(QWebEngineSettings::JavascriptEnabled)) {
        QPointer<NetzacWebView> viewPtr(this);
        QPointer<NetzacWebPage> pagePtr(netzacPage);
        netzacPage->runJavaScript(
            QStringLiteral(
                "(function(){"
                "var p = !!window.__netzacCtxPrevented "
                "&& (performance.now() - (window.__netzacCtxPreventedAt || 0)) < 500;"
                "window.__netzacCtxPrevented = false;"
                "window.__netzacCtxPreventedAt = 0;"
                "return p;"
                "})()"),
            QWebEngineScript::ApplicationWorld,
            [viewPtr, pagePtr, data](const QVariant &result) {
                if (!viewPtr)
                    return;
                if (result.toBool()) {
                    if (pagePtr)
                        pagePtr->resetContextMenuPrevented();
                    return;
                }
                if (pagePtr && pagePtr->wasContextMenuPreventedRecently()) {
                    pagePtr->resetContextMenuPrevented();
                    return;
                }
                emit viewPtr->contextMenuRequested(data);
            });
        return;
    }

    QPointer<NetzacWebView> viewPtr(this);
    QTimer::singleShot(0, this, [viewPtr, data]() {
        if (viewPtr)
            emit viewPtr->contextMenuRequested(data);
    });
}
