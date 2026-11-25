#include "calmmode.h"

#include "webview.h"

#include <QFile>
#include <QPointer>
#include <QVariant>
#include <QVariantMap>
#include <QWebEngineScript>

namespace {

QString loadReadabilitySource()
{
    static const QString cached = []() {
        QFile f(QStringLiteral(":/netzac/Readability.js"));
        if (!f.open(QIODevice::ReadOnly))
            return QString();
        return QString::fromUtf8(f.readAll());
    }();
    return cached;
}

int clampMaxImageWidth(int maxWidth)
{
    return qBound(240, maxWidth, 1920);
}

QString jsQuote(const QString &input)
{
    QString out;
    out.reserve(input.size() + 32);
    out.append(QLatin1Char('"'));
    for (const QChar ch : input) {
        switch (ch.unicode()) {
        case u'\\':
            out.append(QStringLiteral("\\\\"));
            break;
        case u'"':
            out.append(QStringLiteral("\\\""));
            break;
        case u'\n':
            out.append(QStringLiteral("\\n"));
            break;
        case u'\r':
            out.append(QStringLiteral("\\r"));
            break;
        case u'\t':
            out.append(QStringLiteral("\\t"));
            break;
        case 0x2028:
            out.append(QStringLiteral("\\u2028"));
            break;
        case 0x2029:
            out.append(QStringLiteral("\\u2029"));
            break;
        default:
            out.append(ch);
            break;
        }
    }
    out.append(QLatin1Char('"'));
    return out;
}

QString jsBool(bool value)
{
    return value ? QStringLiteral("true") : QStringLiteral("false");
}

}  // namespace

CalmConfig::CalmConfig()
    : excludedHosts(CalmPolicy::defaultExcludedHosts())
{
}

CalmPolicy::CalmPolicy()
    : m_excludedHosts(defaultExcludedHosts())
{
}

CalmPolicy::CalmPolicy(const QStringList &excludedHosts)
{
    setExcludedHosts(excludedHosts);
}

QStringList CalmPolicy::defaultExcludedHosts()
{
    return {QStringLiteral("github.com")};
}

QString CalmPolicy::normalizeHost(const QString &host)
{
    QString norm = host.trimmed().toLower();
    while (norm.endsWith(QLatin1Char('.')))
        norm.chop(1);
    return norm;
}

bool CalmPolicy::hostMatchesDomain(const QString &normalizedHost, const QString &normalizedDomain)
{
    if (normalizedHost.isEmpty() || normalizedDomain.isEmpty())
        return false;
    return normalizedHost == normalizedDomain
        || normalizedHost.endsWith(QLatin1Char('.') + normalizedDomain);
}

void CalmPolicy::setExcludedHosts(const QStringList &hosts)
{
    m_excludedHosts.clear();
    for (const QString &h : hosts)
        addExcludedHost(h);
}

void CalmPolicy::addExcludedHost(const QString &host)
{
    const QString norm = normalizeHost(host);
    if (!norm.isEmpty() && !m_excludedHosts.contains(norm))
        m_excludedHosts.append(norm);
}

bool CalmPolicy::isExcludedUrl(const QUrl &url) const
{
    if (!url.isValid())
        return true;

    const QString scheme = url.scheme().toLower();
    if (scheme != QLatin1String("http")
        && scheme != QLatin1String("https")
        && scheme != QLatin1String("file")) {
        return true;
    }

    if (scheme == QLatin1String("file"))
        return false;

    const QString host = normalizeHost(url.host());
    if (host.isEmpty())
        return true;

    for (const QString &domain : m_excludedHosts) {
        if (hostMatchesDomain(host, domain))
            return true;
    }
    return false;
}

QString CalmDomInjector::buildCalmCss(const CalmConfig &config)
{
    const int capped = clampMaxImageWidth(config.maxImageWidth);
    QString css = QString::fromUtf8(R"CALM_CSS(
html {
  background-color: #f6f4ee !important;
  overflow-x: hidden !important;
  overflow-y: auto !important;
}
body {
  background-color: #f6f4ee !important;
  color: #1f2328 !important;
  font-family: Georgia, 'Liberation Serif', 'Nimbus Roman', serif !important;
  font-size: 17px !important;
  line-height: 1.65 !important;
  max-width: 800px !important;
  width: auto !important;
  min-width: 0 !important;
  height: auto !important;
  margin: 0 auto !important;
  padding: 24px 28px 40px 28px !important;
  box-sizing: border-box !important;
  overflow-x: hidden !important;
  overflow-y: visible !important;
  float: none !important;
  position: static !important;
}
body * {
  max-width: 100% !important;
}
main, article, section, aside, nav, header, footer, div, figure {
  position: static !important;
  float: none !important;
  transform: none !important;
  top: auto !important;
  right: auto !important;
  bottom: auto !important;
  left: auto !important;
  z-index: auto !important;
  height: auto !important;
  min-height: 0 !important;
  max-height: none !important;
  overflow: visible !important;
  box-sizing: border-box !important;
}
main, article, section, aside, header, footer, div {
  display: block !important;
  width: auto !important;
  margin-left: 0 !important;
  margin-right: 0 !important;
  margin-top: 0 !important;
}
main, article, section, aside, nav, header, footer, div, p, ul, ol, li, dl, dt, dd, span, figure, figcaption {
  background-color: transparent !important;
  background-image: none !important;
  color: inherit !important;
}
main, article, section {
  max-width: 100% !important;
}
h1, h2, h3, h4, h5, h6 {
  color: #16181d !important;
  line-height: 1.3 !important;
  margin-top: 1.35em !important;
  margin-bottom: 0.45em !important;
}
p, li, blockquote, dd, dt {
  max-width: 72ch;
}
a {
  color: #1b4f9c !important;
  text-decoration: underline !important;
}
a:visited {
  color: #5b317d !important;
}
pre, code, kbd, samp {
  font-family: 'Nimbus Mono PS', 'Liberation Mono', monospace !important;
  background-color: #edeae0 !important;
  color: #1f2328 !important;
  border-radius: 2px;
}
pre {
  padding: 10px 12px !important;
  overflow-x: auto !important;
  max-width: 100% !important;
  border: 1px solid #d5d0c4 !important;
  white-space: pre !important;
}
blockquote {
  margin: 1em 0 !important;
  padding: 4px 14px !important;
  border-left: 3px solid #9d9689 !important;
  background-color: #efece3 !important;
  color: #1f2328 !important;
}
hr {
  border: 0 !important;
  border-top: 1px solid #c9c3b8 !important;
  margin: 1.5em 0 !important;
}
img, picture, svg {
  position: static !important;
  float: none !important;
  transform: none !important;
  max-width: min(100%, __MAX_IMG_WIDTH__px) !important;
  height: auto !important;
  box-sizing: border-box !important;
  vertical-align: middle;
}
figure {
  max-width: 100% !important;
  margin: 1em 0 !important;
}
figcaption, .netzac-calm-reader-meta {
  font-size: 0.9em !important;
  color: #4b525c !important;
  margin-top: 0.35em !important;
}
.netzac-calm-reader-header h1 {
  margin-top: 0 !important;
}
)CALM_CSS");

    if (config.removeIframes) {
        css.append(QStringLiteral("iframe { display: none !important; }\n"));
    } else {
        css.append(QStringLiteral("iframe { max-width: 100% !important; }\n"));
    }

    if (config.removeVideos) {
        css.append(QStringLiteral("video { display: none !important; }\n"));
    } else {
        css.append(QStringLiteral(
            "video { max-width: min(100%, __MAX_IMG_WIDTH__px) !important; height: auto !important; }\n"));
    }

    if (config.removeAds) {
        css.append(QStringLiteral(
            "[data-ad], [data-ad-slot], [data-ad-client], ins.adsbygoogle, "
            "[aria-modal=\"true\"], [role=\"dialog\"], [role=\"alertdialog\"] { display: none !important; }\n"));
    }

    if (config.normalizeTables) {
        css.append(QString::fromUtf8(R"CALM_TBL(
table {
  width: auto !important;
  max-width: 100% !important;
  border-collapse: collapse !important;
  border-spacing: 0 !important;
  margin: 1em 0 !important;
  font-size: 0.95em !important;
  line-height: 1.45 !important;
}
table th, table td {
  border: 1px solid #c5c0b5 !important;
  padding: 6px 10px !important;
  text-align: left !important;
  vertical-align: top !important;
  overflow-wrap: break-word !important;
}
table th {
  background-color: #eae6dc !important;
  color: #16181d !important;
  font-weight: 600 !important;
}
table caption {
  text-align: left !important;
  font-weight: 600 !important;
  margin-bottom: 0.4em !important;
}
.netzac-calm-table-wrap {
  display: block !important;
  width: 100% !important;
  max-width: 100% !important;
  overflow-x: auto !important;
  margin: 1em 0 !important;
}
.netzac-calm-table-wrap > table {
  margin: 0 !important;
}
)CALM_TBL"));
    }

    css.replace(QStringLiteral("__MAX_IMG_WIDTH__"), QString::number(capped));
    return css;
}

QString CalmDomInjector::buildReaderCss(const CalmConfig &config)
{
    const int capped = clampMaxImageWidth(config.maxImageWidth);
    QString css = QString::fromUtf8(R"CALM_READER_CSS(
html, body {
  background-color: #f6f4ee !important;
  color: #1f2328 !important;
  margin: 0 !important;
  padding: 0 !important;
  width: 100% !important;
  min-width: 0 !important;
  height: auto !important;
  overflow-x: hidden !important;
  overflow-y: auto !important;
}
#netzac-calm-reader {
  display: block !important;
  max-width: 720px !important;
  width: 100% !important;
  margin: 0 auto !important;
  padding: 36px 28px 64px 28px !important;
  box-sizing: border-box !important;
  font-family: Georgia, 'Liberation Serif', 'Nimbus Roman', serif !important;
  font-size: 18px !important;
  line-height: 1.72 !important;
  color: #1f2328 !important;
  word-wrap: break-word !important;
  overflow-wrap: break-word !important;
}
#netzac-calm-reader * {
  max-width: 100% !important;
  box-sizing: border-box !important;
}
.netzac-calm-reader-header {
  display: block !important;
  margin: 0 0 1.25em 0 !important;
}
.netzac-calm-reader-site {
  font-family: 'ZacOS 9 Views', 'DejaVu Sans', sans-serif !important;
  font-size: 0.78em !important;
  font-weight: 600 !important;
  letter-spacing: 0.04em !important;
  text-transform: uppercase !important;
  color: #5b626c !important;
  margin: 0 0 0.45em 0 !important;
}
.netzac-calm-reader-header h1 {
  font-size: 1.95em !important;
  line-height: 1.22 !important;
  font-weight: 700 !important;
  color: #14171c !important;
  margin: 0 0 0.45em 0 !important;
}
.netzac-calm-reader-meta {
  font-family: 'ZacOS 9 Views', 'DejaVu Sans', sans-serif !important;
  font-size: 0.88em !important;
  line-height: 1.45 !important;
  color: #555c66 !important;
  margin: 0 0 1.1em 0 !important;
}
.netzac-calm-reader-header hr {
  border: 0 !important;
  border-top: 1px solid #d2ccc0 !important;
  margin: 0 0 1.6em 0 !important;
}
.netzac-calm-reader-content {
  display: block !important;
}
.netzac-calm-reader-content p {
  margin: 0 0 1.15em 0 !important;
}
.netzac-calm-reader-content h1,
.netzac-calm-reader-content h2,
.netzac-calm-reader-content h3,
.netzac-calm-reader-content h4,
.netzac-calm-reader-content h5,
.netzac-calm-reader-content h6 {
  color: #14171c !important;
  line-height: 1.3 !important;
  font-weight: 700 !important;
  margin: 1.55em 0 0.55em 0 !important;
}
.netzac-calm-reader-content h2 { font-size: 1.45em !important; }
.netzac-calm-reader-content h3 { font-size: 1.22em !important; }
.netzac-calm-reader-content h4 { font-size: 1.08em !important; }
.netzac-calm-reader-content a {
  color: #1b4f9c !important;
  text-decoration: underline !important;
}
.netzac-calm-reader-content a:visited {
  color: #5b317d !important;
}
.netzac-calm-reader-content ul,
.netzac-calm-reader-content ol {
  margin: 0 0 1.15em 0 !important;
  padding-left: 1.65em !important;
}
.netzac-calm-reader-content li {
  margin: 0.3em 0 !important;
}
.netzac-calm-reader-content dl {
  margin: 0 0 1.15em 0 !important;
}
.netzac-calm-reader-content dt {
  font-weight: 700 !important;
  margin-top: 0.6em !important;
}
.netzac-calm-reader-content dd {
  margin: 0.2em 0 0.6em 1.5em !important;
}
.netzac-calm-reader-content blockquote {
  margin: 1.25em 0 !important;
  padding: 8px 16px !important;
  border-left: 3px solid #9d9689 !important;
  background-color: #efece3 !important;
  color: #2b3036 !important;
}
.netzac-calm-reader-content blockquote p:last-child {
  margin-bottom: 0 !important;
}
.netzac-calm-reader-content pre,
.netzac-calm-reader-content code,
.netzac-calm-reader-content kbd,
.netzac-calm-reader-content samp {
  font-family: 'Nimbus Mono PS', 'Liberation Mono', monospace !important;
  background-color: #edeae0 !important;
  color: #1f2328 !important;
  border-radius: 2px !important;
}
.netzac-calm-reader-content code {
  padding: 1px 4px !important;
  font-size: 0.88em !important;
}
.netzac-calm-reader-content pre {
  padding: 12px 14px !important;
  overflow-x: auto !important;
  border: 1px solid #d5d0c4 !important;
  white-space: pre !important;
  line-height: 1.45 !important;
  margin: 1.2em 0 !important;
}
.netzac-calm-reader-content pre code {
  padding: 0 !important;
  background: transparent !important;
  font-size: 0.88em !important;
}
.netzac-calm-reader-content hr {
  border: 0 !important;
  border-top: 1px solid #d2ccc0 !important;
  margin: 1.6em 0 !important;
}
.netzac-calm-reader-content figure {
  margin: 1.4em 0 !important;
  padding: 0 !important;
}
.netzac-calm-reader-content figcaption {
  font-size: 0.88em !important;
  color: #555c66 !important;
  margin-top: 0.45em !important;
  line-height: 1.45 !important;
}
.netzac-calm-reader-content img,
.netzac-calm-reader-content picture {
  display: block !important;
  max-width: min(100%, __MAX_IMG_WIDTH__px) !important;
  height: auto !important;
  margin: 1.2em auto !important;
  border-radius: 2px !important;
}
.netzac-calm-reader-content figure img,
.netzac-calm-reader-content picture img {
  margin: 0 auto !important;
}
.netzac-calm-reader-content svg {
  max-width: min(100%, __MAX_IMG_WIDTH__px) !important;
  height: auto !important;
}
)CALM_READER_CSS");

    if (config.removeIframes) {
        css.append(QStringLiteral("#netzac-calm-reader iframe { display: none !important; }\n"));
    } else {
        css.append(QStringLiteral("#netzac-calm-reader iframe { max-width: 100% !important; }\n"));
    }

    if (config.removeVideos) {
        css.append(QStringLiteral("#netzac-calm-reader video { display: none !important; }\n"));
    } else {
        css.append(QStringLiteral(
            "#netzac-calm-reader video { display: block !important; max-width: min(100%, __MAX_IMG_WIDTH__px) !important; height: auto !important; margin: 1.2em auto !important; }\n"));
    }

    if (config.normalizeTables) {
        css.append(QString::fromUtf8(R"CALM_READER_TBL(
#netzac-calm-reader table {
  width: auto !important;
  max-width: 100% !important;
  border-collapse: collapse !important;
  border-spacing: 0 !important;
  margin: 1.2em 0 !important;
  font-size: 0.92em !important;
  line-height: 1.45 !important;
}
#netzac-calm-reader table th,
#netzac-calm-reader table td {
  border: 1px solid #c5c0b5 !important;
  padding: 6px 10px !important;
  text-align: left !important;
  vertical-align: top !important;
  overflow-wrap: break-word !important;
}
#netzac-calm-reader table th {
  background-color: #eae6dc !important;
  color: #14171c !important;
  font-weight: 600 !important;
}
#netzac-calm-reader table caption {
  text-align: left !important;
  font-weight: 600 !important;
  margin-bottom: 0.4em !important;
}
#netzac-calm-reader .netzac-calm-table-wrap {
  display: block !important;
  width: 100% !important;
  max-width: 100% !important;
  overflow-x: auto !important;
  margin: 1.2em 0 !important;
}
#netzac-calm-reader .netzac-calm-table-wrap > table {
  margin: 0 !important;
}
)CALM_READER_TBL"));
    }

    css.replace(QStringLiteral("__MAX_IMG_WIDTH__"), QString::number(capped));
    return css;
}

QString CalmDomInjector::buildApplyScript(const CalmConfig &config)
{
    const int capped = clampMaxImageWidth(config.maxImageWidth);
    const QString quotedCss = jsQuote(buildCalmCss(config));
    const QString quotedReaderCss = jsQuote(buildReaderCss(config));
    const QString readabilitySrc = (config.layoutMode == CalmLayoutMode::ReaderFocus)
        ? loadReadabilitySource()
        : QString();

    QString script = QString::fromUtf8(R"CALM_JS(
(() => {
  const STYLE_ID = "netzac-calm-style";
  const JS_DETECTION = __JS_DETECTION__;
  const LAYOUT_MODE = __LAYOUT_MODE__;
  const REMOVE_SCRIPTS = __REMOVE_SCRIPTS__;
  const REMOVE_IFRAMES = __REMOVE_IFRAMES__;
  const REMOVE_VIDEOS = __REMOVE_VIDEOS__;
  const REMOVE_ADS = __REMOVE_ADS__;
  const NORMALIZE_TABLES = __NORMALIZE_TABLES__;
  const MAX_IMG_WIDTH = __MAX_IMG_WIDTH__;
  const CALM_CSS = __CALM_CSS__;
  const READER_CSS = __READER_CSS__;

  if (!document.documentElement || !document.body) {
    return { status: "error", removed: 0, imagesCapped: 0, tablesNormalized: 0, readerFocus: false };
  }

  // Idempotency guard: never apply Calm formatting twice to the same document.
  if (document.documentElement.dataset.netzacCalm === "1" && document.getElementById(STYLE_ID)) {
    return { status: "already-applied", removed: 0, imagesCapped: 0, tablesNormalized: 0, readerFocus: false };
  }

  // Configurable JavaScript / interactive web app detection BEFORE any DOM or style mutations.
  const DATA_SCRIPT_TYPES = new Set([
    "application/json",
    "application/ld+json",
    "application/geo+json",
    "text/template",
    "text/html",
    "text/plain"
  ]);

  const hasExecutableScripts = () => {
    const scripts = document.querySelectorAll("script");
    for (let i = 0; i < scripts.length; ++i) {
      const s = scripts[i];
      const src = s.getAttribute("src");
      if (src !== null && src.trim().length > 0) {
        return true;
      }
      const rawType = (s.getAttribute("type") || "").trim().toLowerCase().split(";")[0].trim();
      if (rawType && DATA_SCRIPT_TYPES.has(rawType)) {
        continue;
      }
      if ((s.textContent || "").trim().length > 0) {
        return true;
      }
    }
    return false;
  };

  const isInteractiveWebApp = () => {
    const bodyTextLen = (document.body.innerText || "").replace(/\s+/g, " ").trim().length;
    let pTextLen = 0;
    const paragraphs = document.querySelectorAll("p");
    for (let i = 0; i < paragraphs.length; ++i) {
      const txt = (paragraphs[i].textContent || "").trim();
      if (txt.length >= 20) {
        pTextLen += txt.length;
      }
    }

    let hasLargeCanvas = false;
    const minCanvasArea = Math.max(160000, window.innerWidth * window.innerHeight * 0.35);
    const canvases = document.querySelectorAll("canvas");
    for (let i = 0; i < canvases.length; ++i) {
      const r = canvases[i].getBoundingClientRect();
      if (r.width * r.height >= minCanvasArea) {
        hasLargeCanvas = true;
        break;
      }
    }

    const hasAppRole = document.body.getAttribute("role") === "application"
      || document.querySelector('[role="application"], [contenteditable="true"]') !== null;

    if (bodyTextLen < 60 && pTextLen === 0) {
      return true;
    }
    if (hasLargeCanvas && pTextLen < 120) {
      return true;
    }
    if (hasAppRole && pTextLen < 150 && bodyTextLen < 350) {
      return true;
    }
    return false;
  };

  if (JS_DETECTION === 2 && hasExecutableScripts()) {
    return { status: "skipped-js", removed: 0, imagesCapped: 0, tablesNormalized: 0, readerFocus: false };
  }
  if (JS_DETECTION === 1 && hasExecutableScripts() && isInteractiveWebApp()) {
    return { status: "skipped-webapp", removed: 0, imagesCapped: 0, tablesNormalized: 0, readerFocus: false };
  }

  // Rescue lazy-loaded images before extracting or stripping DOM.
  const lazyAttrs = ["data-src", "data-lazy-src", "data-original", "data-src-medium", "data-url"];
  for (let i = 0; i < document.images.length; ++i) {
    const img = document.images[i];
    const src = (img.getAttribute("src") || "").trim();
    const needsRescue = !src || /^(?:data:image\/(?:gif|svg\+xml)|about:blank)/i.test(src);
    if (needsRescue) {
      for (let j = 0; j < lazyAttrs.length; ++j) {
        const candidate = (img.getAttribute(lazyAttrs[j]) || "").trim();
        if (candidate && !/^data:image\/gif/i.test(candidate)) {
          img.setAttribute("src", candidate);
          break;
        }
      }
    }
    const lazySrcset = (img.getAttribute("data-srcset") || img.getAttribute("data-lazy-srcset") || "").trim();
    if (!img.getAttribute("srcset") && lazySrcset) {
      img.setAttribute("srcset", lazySrcset);
    }
  }

  const NON_VISUAL_TAGS = new Set([
    "STYLE", "LINK", "META", "TITLE", "NOSCRIPT", "TEMPLATE",
    "DATALIST", "SOURCE", "TRACK", "PARAM", "AREA", "BR", "WBR", "SCRIPT"
  ]);
  const SEMANTIC_PROTECTED_TAGS = new Set(["BODY", "MAIN", "ARTICLE", "SECTION"]);
  const DISTRACTION_RE = /(?:^|[\s_-])(?:ad|ads|advert|advertisement|adsense|adserver|doubleclick|dfp|taboola|outbrain|mgid|teads|publicidade|publi|anuncio|anuncios|patrocinado|patrocinio|propaganda|sponsored|sponsor|promo|promotion|gdpr|paywall|popup|modal|cookie(?:banner|notice|consent|bar|popup|modal|overlay|wall)|cookies?(?=$|[\s_-](?:banner|notice|consent|bar|popup|modal|overlay|wall|policy|box|msg|message))|consent(?:[-_]?(?:banner|modal|popup|bar))?|newsletter(?:[-_]?(?:signup|box|banner|modal))?|glb[-_]?ad|cn[-_]?ad|floating[-_]?player|sticky[-_]?(?:banner|ad|footer|bar)|ad[-_]?(?:slot|unit|box|banner|container|wrapper|feedback)|banner[-_]?ad|sponsored[-_]?(?:post|content|block|link))(?=$|[\s_-])/i;

  const isHiddenElement = (el) => {
    if (el.hasAttribute("hidden") || el.getAttribute("aria-hidden") === "true") {
      return true;
    }
    if (el.style) {
      if (el.style.display === "none" || el.style.visibility === "hidden") {
        return true;
      }
    }
    const cs = window.getComputedStyle ? window.getComputedStyle(el) : null;
    if (cs && (cs.display === "none" || cs.visibility === "hidden")) {
      return true;
    }
    return false;
  };

  const isDistractionContainer = (el, tag) => {
    if (tag === "DIALOG") {
      return true;
    }
    if (tag === "INS" && typeof el.className === "string" && /\badsbygoogle\b/i.test(el.className)) {
      return true;
    }
    if (el.hasAttribute("data-ad") || el.hasAttribute("data-ad-slot")
        || el.hasAttribute("data-ad-client") || el.hasAttribute("data-sponsored")) {
      return true;
    }
    if (el.getAttribute("aria-modal") === "true") {
      return true;
    }
    const role = (el.getAttribute("role") || "").trim().toLowerCase();
    if (role === "dialog" || role === "alertdialog") {
      return true;
    }
    const testId = el.getAttribute("data-testid") || "";
    if (testId && DISTRACTION_RE.test(testId)) {
      return true;
    }
    const idStr = el.id || "";
    const clsStr = typeof el.className === "string" ? el.className : (el.getAttribute("class") || "");
    return (idStr && DISTRACTION_RE.test(idStr)) || (clsStr && DISTRACTION_RE.test(clsStr));
  };

  let removedCount = 0;
  let usedReaderFocus = false;
)CALM_JS" R"CALM_JS(
  // Firefox Reader View / Chrome-style main article extraction via Mozilla Readability.
  if (LAYOUT_MODE === 1) {
    const loadReadability = () => {
      const module = undefined;
      const exports = undefined;
      __READABILITY_SRC__
      return typeof Readability === "function" ? Readability : null;
    };

    const ReadabilityCtor = loadReadability();
    if (ReadabilityCtor) {
      // Mark computed-hidden elements on the live DOM before cloning so Readability ignores hidden drawers/menus.
      const markedHidden = [];
      const markStack = [];
      const liveBodyChildren = document.body.children;
      for (let i = liveBodyChildren.length - 1; i >= 0; --i) {
        markStack.push(liveBodyChildren[i]);
      }
      while (markStack.length > 0) {
        const el = markStack.pop();
        const tag = el.tagName ? el.tagName.toUpperCase() : "";
        if (!tag || NON_VISUAL_TAGS.has(tag)) continue;
        const protectsHidden = (tag === "MAIN" || tag === "ARTICLE" || tag === "BODY")
          || (el.querySelector && el.querySelector("main, article") !== null);
        if (!protectsHidden && isHiddenElement(el)) {
          el.setAttribute("data-netzac-calm-hidden", "1");
          markedHidden.push(el);
          continue;
        }
        const ch = el.children;
        for (let i = ch.length - 1; i >= 0; --i) {
          markStack.push(ch[i]);
        }
      }

      const docClone = document.cloneNode(true);

      for (let i = 0; i < markedHidden.length; ++i) {
        markedHidden[i].removeAttribute("data-netzac-calm-hidden");
      }

      if (docClone.head && document.baseURI && /^https?:/i.test(document.baseURI)
          && !docClone.querySelector("base[href]")) {
        const baseEl = docClone.createElement("base");
        baseEl.setAttribute("href", document.baseURI);
        docClone.head.insertBefore(baseEl, docClone.head.firstChild);
      }

      const hiddenInClone = docClone.querySelectorAll('[data-netzac-calm-hidden="1"]');
      for (let i = 0; i < hiddenInClone.length; ++i) {
        hiddenInClone[i].remove();
      }

      if (REMOVE_IFRAMES && docClone.body) {
        const cloneIframes = docClone.body.querySelectorAll("iframe");
        for (let i = 0; i < cloneIframes.length; ++i) {
          cloneIframes[i].remove();
        }
      }
      if (REMOVE_VIDEOS && docClone.body) {
        const cloneVideos = docClone.body.querySelectorAll("video");
        for (let i = 0; i < cloneVideos.length; ++i) {
          cloneVideos[i].remove();
        }
      }
      if (REMOVE_ADS && docClone.body) {
        const cloneToRemove = [];
        const cloneStack = [];
        const cloneChildren = docClone.body.children;
        for (let i = cloneChildren.length - 1; i >= 0; --i) {
          cloneStack.push(cloneChildren[i]);
        }
        while (cloneStack.length > 0) {
          const el = cloneStack.pop();
          const tag = el.tagName ? el.tagName.toUpperCase() : "";
          if (!tag || NON_VISUAL_TAGS.has(tag)) continue;
          const protectsFromHeuristics = SEMANTIC_PROTECTED_TAGS.has(tag)
            || (el.querySelector && el.querySelector("main, article") !== null);
          if (!protectsFromHeuristics && isDistractionContainer(el, tag)) {
            cloneToRemove.push(el);
            continue;
          }
          const ch = el.children;
          for (let i = ch.length - 1; i >= 0; --i) {
            cloneStack.push(ch[i]);
          }
        }
        for (let i = 0; i < cloneToRemove.length; ++i) {
          cloneToRemove[i].remove();
        }
      }

      let article = null;
      try {
        article = new ReadabilityCtor(docClone, {
          charThreshold: 200,
          keepClasses: false,
          serializer: (el) => el
        }).parse();
      } catch (e) {
        article = null;
      }

      const articleText = article && article.textContent
        ? article.textContent.replace(/\s+/g, " ").trim()
        : "";

      if (article && article.content && articleText.length >= 160) {
        const contentNode = document.importNode(article.content, true);

        const stripSelectors = [
          "script", "style", "noscript", "form", "button", "input",
          "select", "textarea", "nav", "aside", "footer", "dialog",
          "[aria-modal='true']"
        ];
        if (REMOVE_IFRAMES) stripSelectors.push("iframe");
        if (REMOVE_VIDEOS) stripSelectors.push("video");
        const stripNodes = contentNode.querySelectorAll(stripSelectors.join(", "));
        for (let i = 0; i < stripNodes.length; ++i) {
          stripNodes[i].remove();
          ++removedCount;
        }

        // Remove UI/icon SVGs so small navigation/share glyphs don't scale up as illustrations.
        const svgs = contentNode.querySelectorAll("svg");
        for (let i = 0; i < svgs.length; ++i) {
          const svg = svgs[i];
          const w = parseFloat(svg.getAttribute("width") || "0");
          const h = parseFloat(svg.getAttribute("height") || "0");
          const isHiddenSvg = svg.getAttribute("aria-hidden") === "true";
          const inControl = svg.closest("a, button") !== null;
          const isSmallExplicit = (w > 0 && w <= 64) || (h > 0 && h <= 64);
          const isLikelyIcon = !svg.querySelector("text") && !(w > 80);
          if (isHiddenSvg || inControl || isSmallExplicit || isLikelyIcon) {
            svg.remove();
            ++removedCount;
          }
        }

        // Strip leftover inline styles, site classes, and non-heading IDs inside extracted article.
        const allNodes = contentNode.querySelectorAll("*");
        for (let i = 0; i < allNodes.length; ++i) {
          const el = allNodes[i];
          if (el.hasAttribute("style")) {
            el.removeAttribute("style");
          }
          const cls = el.getAttribute("class");
          if (cls && cls.indexOf("netzac-calm-") === -1) {
            el.removeAttribute("class");
          }
          const tag = el.tagName ? el.tagName.toUpperCase() : "";
          if (el.hasAttribute("id") && !/^H[1-6]$/.test(tag)) {
            el.removeAttribute("id");
          }
        }

        const titleText = (article.title || document.title || "").replace(/\s+/g, " ").trim();

        // Remove duplicate leading H1/H2 that repeats the header title.
        if (titleText) {
          const normTitle = titleText.toLowerCase();
          const headings = contentNode.querySelectorAll("h1, h2");
          for (let i = 0; i < headings.length && i < 2; ++i) {
            const h = headings[i];
            const hText = (h.textContent || "").replace(/\s+/g, " ").trim().toLowerCase();
            if (hText === normTitle) {
              h.remove();
              ++removedCount;
              break;
            }
          }
        }
)CALM_JS" R"CALM_JS(
        // Remove small link-only clutter boxes (related links, share lists, tag pills).
        const boxCandidates = contentNode.querySelectorAll("div, section, ul, ol");
        for (let i = boxCandidates.length - 1; i >= 0; --i) {
          const box = boxCandidates[i];
          if (!box.parentNode) continue;
          const boxText = (box.textContent || "").replace(/\s+/g, " ").trim();
          if (!boxText || boxText.length >= 300) continue;
          let hasLongParagraph = false;
          const ps = box.querySelectorAll("p");
          for (let j = 0; j < ps.length; ++j) {
            if ((ps[j].textContent || "").replace(/\s+/g, " ").trim().length >= 80) {
              hasLongParagraph = true;
              break;
            }
          }
          if (hasLongParagraph) continue;
          const links = box.querySelectorAll("a");
          if (links.length < 3) continue;
          let linkChars = 0;
          for (let j = 0; j < links.length; ++j) {
            linkChars += (links[j].textContent || "").replace(/\s+/g, " ").trim().length;
          }
          if (linkChars / boxText.length >= 0.85) {
            box.remove();
            ++removedCount;
          }
        }

        // Bottom-up prune empty wrapper nodes left after cleaning.
        const pruneNodes = contentNode.querySelectorAll("div, section, p, span, figure, header, ul, ol");
        for (let i = pruneNodes.length - 1; i >= 0; --i) {
          const n = pruneNodes[i];
          if (!n.parentNode) continue;
          const hasText = (n.textContent || "").replace(/\s+/g, "").length > 0;
          const hasMedia = n.querySelector("img, picture, svg, table, pre, hr, iframe, video") !== null;
          if (!hasText && !hasMedia) {
            n.remove();
          }
        }

        const readerRoot = document.createElement("article");
        readerRoot.id = "netzac-calm-reader";
        if (article.dir) readerRoot.setAttribute("dir", article.dir);
        if (article.lang) readerRoot.setAttribute("lang", article.lang);

        const metaParts = [];
        const byline = (article.byline || "").replace(/\s+/g, " ").trim();
        if (byline && byline.length <= 160) {
          metaParts.push(byline);
        }
        if (article.publishedTime) {
          const rawPub = String(article.publishedTime).trim();
          let pubStr = rawPub;
          const parsedDate = new Date(rawPub);
          if (!Number.isNaN(parsedDate.getTime())) {
            try {
              pubStr = parsedDate.toLocaleDateString();
            } catch (e) {
              pubStr = rawPub;
            }
          }
          if (pubStr && metaParts.indexOf(pubStr) === -1) {
            metaParts.push(pubStr);
          }
        }

        const siteName = (article.siteName || "").replace(/\s+/g, " ").trim();
        if (titleText || siteName || metaParts.length > 0) {
          const hdr = document.createElement("header");
          hdr.className = "netzac-calm-reader-header";
          if (siteName) {
            const siteEl = document.createElement("div");
            siteEl.className = "netzac-calm-reader-site";
            siteEl.textContent = siteName;
            hdr.appendChild(siteEl);
          }
          if (titleText) {
            const h1 = document.createElement("h1");
            h1.textContent = titleText;
            hdr.appendChild(h1);
          }
          if (metaParts.length > 0) {
            const metaDiv = document.createElement("div");
            metaDiv.className = "netzac-calm-reader-meta";
            metaDiv.textContent = metaParts.join(" — ");
            hdr.appendChild(metaDiv);
          }
          hdr.appendChild(document.createElement("hr"));
          readerRoot.appendChild(hdr);
        }

        const contentDiv = document.createElement("div");
        contentDiv.className = "netzac-calm-reader-content";
        contentDiv.appendChild(contentNode);
        readerRoot.appendChild(contentDiv);

        // Remove/disable all original page stylesheets so site CSS never mixes with Reader View.
        const oldStyles = document.querySelectorAll('link[rel~="stylesheet"], style');
        for (let i = 0; i < oldStyles.length; ++i) {
          const st = oldStyles[i];
          if (st.id === STYLE_ID) continue;
          if (st.tagName === "LINK") {
            st.disabled = true;
          }
          st.remove();
          ++removedCount;
        }
        document.documentElement.removeAttribute("style");
        document.documentElement.removeAttribute("class");
        document.body.removeAttribute("style");
        document.body.removeAttribute("class");

        if (REMOVE_SCRIPTS) {
          const scriptNodes = document.querySelectorAll("script, link[rel='preload'][as='script'], link[rel='modulepreload']");
          for (let i = 0; i < scriptNodes.length; ++i) {
            scriptNodes[i].remove();
            ++removedCount;
          }
        }

        removedCount += Math.max(0, document.body.children.length);
        document.body.replaceChildren(readerRoot);
        usedReaderFocus = true;
        window.scrollTo(0, 0);
      }
    }
  }

  // CleanPage pipeline (used when CleanPage is selected or ReaderFocus falls back on non-article pages).
  if (!usedReaderFocus) {
    if (REMOVE_SCRIPTS) {
      const scriptNodes = document.querySelectorAll("script, link[rel='preload'][as='script'], link[rel='modulepreload']");
      for (let i = 0; i < scriptNodes.length; ++i) {
        scriptNodes[i].remove();
        ++removedCount;
      }
    }

    if (REMOVE_VIDEOS) {
      const videos = document.querySelectorAll("video");
      for (let i = 0; i < videos.length; ++i) {
        const v = videos[i];
        try {
          v.pause();
          v.muted = true;
        } catch (e) {}
        v.removeAttribute("src");
        v.removeAttribute("autoplay");
        const sources = v.querySelectorAll("source");
        for (let j = 0; j < sources.length; ++j) {
          sources[j].remove();
        }
        try {
          v.load();
        } catch (e) {}
        v.remove();
        ++removedCount;
      }
    }

    // Batched read phase over document.body (skipping doomed subtrees via explicit stack).
    if (REMOVE_IFRAMES || REMOVE_ADS) {
      const toRemove = [];
      const stack = [];
      const bodyChildren = document.body.children;
      for (let i = bodyChildren.length - 1; i >= 0; --i) {
        stack.push(bodyChildren[i]);
      }

      while (stack.length > 0) {
        const el = stack.pop();
        const tag = el.tagName ? el.tagName.toUpperCase() : "";
        if (!tag || NON_VISUAL_TAGS.has(tag)) {
          continue;
        }

        if (REMOVE_IFRAMES && tag === "IFRAME") {
          toRemove.push(el);
          continue;
        }

        if (REMOVE_ADS) {
          const protectsFromHeuristics = SEMANTIC_PROTECTED_TAGS.has(tag)
            || (el.querySelector && el.querySelector("main, article") !== null);
          const protectsHidden = (tag === "MAIN" || tag === "ARTICLE" || tag === "BODY")
            || (el.querySelector && el.querySelector("main, article") !== null);

          if ((!protectsHidden && isHiddenElement(el))
              || (!protectsFromHeuristics && isDistractionContainer(el, tag))) {
            toRemove.push(el);
            continue;
          }
        }

        const children = el.children;
        for (let i = children.length - 1; i >= 0; --i) {
          stack.push(children[i]);
        }
      }

      for (let i = 0; i < toRemove.length; ++i) {
        toRemove[i].remove();
        ++removedCount;
      }
    }
)CALM_JS" R"CALM_JS(
    // Neutralize negative margins in inline styles to prevent overlapping blocks.
    const inlineMarginEls = document.body.querySelectorAll("[style]");
    for (let i = 0; i < inlineMarginEls.length; ++i) {
      const el = inlineMarginEls[i];
      const st = el.style;
      if (!st) continue;
      for (const prop of ["margin-top", "margin-right", "margin-bottom", "margin-left"]) {
        const val = parseFloat(st.getPropertyValue(prop));
        if (Number.isFinite(val) && val < 0) {
          st.removeProperty(prop);
        }
      }
    }
  }

  // Inject modular Calm/Reader CSS and mark document as processed.
  const styleEl = document.createElement("style");
  styleEl.id = STYLE_ID;
  styleEl.textContent = usedReaderFocus ? READER_CSS : CALM_CSS;
  (document.head || document.documentElement).appendChild(styleEl);
  document.documentElement.dataset.netzacCalm = "1";

  // Enforce maxImageWidth cleanly while preserving proportional height.
  const parsePx = (raw) => {
    if (!raw) return 0;
    const trimmed = String(raw).trim();
    if (!trimmed || trimmed.endsWith("%") || trimmed.endsWith("vw")) return 0;
    const num = parseFloat(trimmed);
    return Number.isFinite(num) && num > 0 ? num : 0;
  };

  let imagesCapped = 0;
  const imgs = document.images;
  for (let i = 0; i < imgs.length; ++i) {
    const img = imgs[i];
    const rawAttrW = img.getAttribute("width");
    const rawAttrH = img.getAttribute("height");
    const attrW = parsePx(rawAttrW);
    const attrH = parsePx(rawAttrH);
    const rawStyleW = img.style ? img.style.width : "";
    const rawStyleH = img.style ? img.style.height : "";
    const inlineW = parsePx(rawStyleW);
    const inlineH = parsePx(rawStyleH);

    const hasPctWidth = (rawAttrW && rawAttrW.indexOf("%") !== -1)
      || (rawStyleW && (rawStyleW.indexOf("%") !== -1 || rawStyleW.indexOf("vw") !== -1));
    const hasExplicitDim = rawAttrW !== null || rawAttrH !== null
      || Boolean(rawStyleW) || Boolean(rawStyleH);

    const exceeds = attrW > MAX_IMG_WIDTH
      || inlineW > MAX_IMG_WIDTH
      || hasPctWidth
      || (img.naturalWidth > MAX_IMG_WIDTH && hasExplicitDim)
      || (!attrW && !inlineW && (attrH > MAX_IMG_WIDTH || inlineH > MAX_IMG_WIDTH));

    if (exceeds) {
      if (img.hasAttribute("width")) img.removeAttribute("width");
      if (img.hasAttribute("height")) img.removeAttribute("height");
      if (img.style) {
        img.style.removeProperty("width");
        img.style.removeProperty("height");
        img.style.removeProperty("min-width");
      }
      ++imagesCapped;
    }
  }

  // Organize tables cleanly when enabled.
  let tablesNormalized = 0;
  if (NORMALIZE_TABLES) {
    const tables = document.querySelectorAll("table");
    tablesNormalized = tables.length;
    for (let i = 0; i < tables.length; ++i) {
      const tbl = tables[i];
      tbl.removeAttribute("cellpadding");
      tbl.removeAttribute("cellspacing");
      tbl.removeAttribute("border");
      const tblAttrW = tbl.getAttribute("width");
      if (tblAttrW !== null) {
        const px = parsePx(tblAttrW);
        if (px > MAX_IMG_WIDTH || tblAttrW.indexOf("%") !== -1) {
          tbl.removeAttribute("width");
        }
      }
      if (tbl.style) {
        tbl.style.removeProperty("min-width");
        const sw = tbl.style.width;
        if (sw && (parsePx(sw) > MAX_IMG_WIDTH || sw.indexOf("%") !== -1)) {
          tbl.style.removeProperty("width");
        }
      }
    }

    const wrapTargets = [];
    for (let i = 0; i < tables.length; ++i) {
      const tbl = tables[i];
      if (tbl.closest(".netzac-calm-table-wrap")) continue;
      if (tbl.parentElement && tbl.parentElement.closest("table")) continue;
      const availW = tbl.parentElement ? tbl.parentElement.clientWidth : document.body.clientWidth;
      if (availW > 0 && tbl.scrollWidth > availW + 2) {
        wrapTargets.push(tbl);
      }
    }
    for (let i = 0; i < wrapTargets.length; ++i) {
      const tbl = wrapTargets[i];
      if (!tbl.parentNode) continue;
      const wrap = document.createElement("div");
      wrap.className = "netzac-calm-table-wrap";
      tbl.parentNode.insertBefore(wrap, tbl);
      wrap.appendChild(tbl);
    }
  }

  return {
    status: "applied",
    removed: removedCount,
    imagesCapped: imagesCapped,
    tablesNormalized: tablesNormalized,
    readerFocus: usedReaderFocus
  };
})()
)CALM_JS");

    script.replace(QStringLiteral("__JS_DETECTION__"), QString::number(int(config.jsDetection)));
    script.replace(QStringLiteral("__LAYOUT_MODE__"), QString::number(int(config.layoutMode)));
    script.replace(QStringLiteral("__REMOVE_SCRIPTS__"), jsBool(config.removeScripts));
    script.replace(QStringLiteral("__REMOVE_IFRAMES__"), jsBool(config.removeIframes));
    script.replace(QStringLiteral("__REMOVE_VIDEOS__"), jsBool(config.removeVideos));
    script.replace(QStringLiteral("__REMOVE_ADS__"), jsBool(config.removeAds));
    script.replace(QStringLiteral("__NORMALIZE_TABLES__"), jsBool(config.normalizeTables));
    script.replace(QStringLiteral("__MAX_IMG_WIDTH__"), QString::number(capped));
    script.replace(QStringLiteral("__CALM_CSS__"), quotedCss);
    script.replace(QStringLiteral("__READER_CSS__"), quotedReaderCss);
    script.replace(QStringLiteral("__READABILITY_SRC__"), readabilitySrc);
    return script;
}

QString CalmDomInjector::buildRevertScript()
{
    return QString::fromUtf8(R"CALM_JS(
(() => {
  let reverted = false;
  const styleEl = document.getElementById("netzac-calm-style");
  if (styleEl) {
    styleEl.remove();
    reverted = true;
  }
  if (document.documentElement && document.documentElement.dataset.netzacCalm) {
    delete document.documentElement.dataset.netzacCalm;
    reverted = true;
  }
  return { reverted: reverted };
})()
)CALM_JS");
}

CalmModeController::CalmModeController(
    NetzacWebPage *page,
    const CalmConfig &config,
    QObject *parent)
    : QObject(parent)
    , m_page(page)
    , m_config(config)
    , m_policy(config.excludedHosts)
{
    m_config.maxImageWidth = clampMaxImageWidth(m_config.maxImageWidth);
    qRegisterMetaType<CalmModeController::Status>("CalmModeController::Status");
    qRegisterMetaType<CalmModeController::EvaluationReport>("CalmModeController::EvaluationReport");

    if (m_page) {
        connect(
            m_page,
            &NetzacWebPage::mainFrameNavigationRequested,
            this,
            &CalmModeController::onMainFrameNavigationRequested);
        connect(
            m_page,
            &QWebEnginePage::loadFinished,
            this,
            &CalmModeController::onLoadFinished);
    }
}

void CalmModeController::setConfig(const CalmConfig &config)
{
    CalmConfig normalized = config;
    normalized.maxImageWidth = clampMaxImageWidth(normalized.maxImageWidth);
    m_policy.setExcludedHosts(normalized.excludedHosts);
    normalized.excludedHosts = m_policy.excludedHosts();

    if (normalized == m_config)
        return;

    m_config = normalized;
    if (!m_page || !m_enabled)
        return;

    if (m_appliedOnCurrentPage) {
        ++m_evalGeneration;
        m_appliedOnCurrentPage = false;
        m_pendingLoadEvaluation = true;
        m_page->triggerAction(QWebEnginePage::Reload);
    } else {
        evaluateCurrentPage();
    }
}

void CalmModeController::setMaxImageWidth(int maxWidth)
{
    CalmConfig updated = m_config;
    updated.maxImageWidth = clampMaxImageWidth(maxWidth);
    setConfig(updated);
}

void CalmModeController::setEnabled(bool enabled)
{
    if (enabled == m_enabled)
        return;

    m_enabled = enabled;
    ++m_evalGeneration;
    emit enabledChanged(m_enabled);

    if (m_enabled) {
        evaluateCurrentPage();
        return;
    }

    m_pendingLoadEvaluation = false;
    const QUrl currentUrl = m_page ? m_page->url() : QUrl();
    if (m_page && m_appliedOnCurrentPage) {
        m_appliedOnCurrentPage = false;
        QPointer<NetzacWebPage> pagePtr(m_page);
        m_page->runJavaScript(
            CalmDomInjector::buildRevertScript(),
            QWebEngineScript::ApplicationWorld,
            [pagePtr](const QVariant &) {
                if (pagePtr)
                    pagePtr->triggerAction(QWebEnginePage::Reload);
            });
    } else {
        m_appliedOnCurrentPage = false;
    }

    EvaluationReport report;
    report.status = Status::Disabled;
    report.url = currentUrl;
    emit evaluationFinished(report);
}

void CalmModeController::toggle()
{
    setEnabled(!m_enabled);
}

void CalmModeController::evaluateCurrentPage()
{
    if (!m_page || !m_enabled)
        return;

    const QUrl currentUrl = m_page->url();
    if (m_page->isBuiltInPage()) {
        m_appliedOnCurrentPage = false;
        EvaluationReport report;
        report.status = Status::SkippedBuiltIn;
        report.url = currentUrl;
        emit evaluationFinished(report);
        return;
    }

    if (m_policy.isExcludedUrl(currentUrl)) {
        m_appliedOnCurrentPage = false;
        EvaluationReport report;
        report.status = Status::ExcludedHost;
        report.url = currentUrl;
        emit evaluationFinished(report);
        return;
    }

    const quint64 gen = ++m_evalGeneration;
    QPointer<CalmModeController> self(this);
    m_page->runJavaScript(
        CalmDomInjector::buildApplyScript(m_config),
        QWebEngineScript::ApplicationWorld,
        [self, gen, currentUrl](const QVariant &result) {
            if (!self || gen != self->m_evalGeneration || !self->m_enabled)
                return;

            const QVariantMap map = result.toMap();
            const QString statusStr = map.value(QStringLiteral("status")).toString();

            EvaluationReport report;
            report.url = currentUrl;
            report.removedNodes = map.value(QStringLiteral("removed")).toInt();
            report.cappedImages = map.value(QStringLiteral("imagesCapped")).toInt();
            report.normalizedTables = map.value(QStringLiteral("tablesNormalized")).toInt();
            report.usedReaderFocus = map.value(QStringLiteral("readerFocus")).toBool();

            if (statusStr == QLatin1String("applied")) {
                report.status = Status::Applied;
                self->m_appliedOnCurrentPage = true;
            } else if (statusStr == QLatin1String("already-applied")) {
                report.status = Status::AlreadyApplied;
                self->m_appliedOnCurrentPage = true;
            } else if (statusStr == QLatin1String("skipped-js")) {
                report.status = Status::SkippedJavaScript;
                self->m_appliedOnCurrentPage = false;
            } else if (statusStr == QLatin1String("skipped-webapp")) {
                report.status = Status::SkippedWebApp;
                self->m_appliedOnCurrentPage = false;
            } else {
                report.status = Status::Error;
                self->m_appliedOnCurrentPage = false;
            }

            emit self->evaluationFinished(report);
        });
}

void CalmModeController::onMainFrameNavigationRequested(
    const QUrl &url,
    QWebEnginePage::NavigationType type)
{
    // Invalidate in-flight evaluations from the previous document; keep target URL untouched.
    ++m_evalGeneration;
    m_appliedOnCurrentPage = false;
    m_pendingLoadEvaluation = m_enabled;
    emit navigationIntercepted(url, type);
}

void CalmModeController::onLoadFinished(bool ok)
{
    m_pendingLoadEvaluation = false;
    if (ok && m_enabled)
        evaluateCurrentPage();
}
