#include "zacos9theme.h"

#include <QAbstractScrollArea>
#include <QAbstractSpinBox>
#include <QComboBox>
#include <QColor>
#include <QPainter>
#include <QPalette>
#include <QPoint>
#include <QStyleFactory>
#include <QStyleOption>
#include <QStyleOptionButton>
#include <QStyleOptionFrame>
#include <QStyleOptionSlider>
#include <QWidget>
#include <algorithm>
#include <cstring>

namespace {

// Exact constants from Zac-OS-9 lib/widgets.h & lib/draw.h
constexpr int kButtonH = 20;
constexpr int kButtonRing = 3;
constexpr int kButtonMinW = 58;
constexpr int kCheckboxSize = 12;
constexpr int kRadioSize = 12;
constexpr int kSbWidth = 16;
constexpr int kSbArrow = 14;
constexpr int kSbThumb = 15;
constexpr int kSbEnd = kSbArrow + 2;
constexpr int kSbSliderMin = kSbThumb + 2;
constexpr int kPopupArrowsW = 21;

inline QColor grayColor(int n)
{
    const int v = std::clamp(n, 0, 15) * 0x11;
    return QColor(v, v, v);
}

inline int hexNibble(char ch)
{
    if (ch >= '0' && ch <= '9')
        return ch - '0';
    if (ch >= 'a' && ch <= 'f')
        return ch - 'a' + 10;
    if (ch >= 'A' && ch <= 'F')
        return ch - 'A' + 10;
    return -1;
}

struct NineSlice {
    int left;
    int right;
    int height;
    const char *rows[26];
};

// Exact 9-slice push button art from Zac-OS-9 lib/buttons_data.h
constexpr NineSlice kBtnStandard = {
    8,
    8,
    20,
    {
        "..2000000000002..",
        ".0bdddddddddddb0.",
        "2bffffffffffffdb2",
        "0dffdddddddddda70",
        "0dfddddddddddda70",
        "0dfddddddddddda70",
        "0dfddddddddddda70",
        "0dfddddddddddda70",
        "0dfddddddddddda70",
        "0dfddddddddddda70",
        "0dfddddddddddda70",
        "0dfddddddddddda70",
        "0dfddddddddddda70",
        "0dfddddddddddda70",
        "0dfddddddddddda70",
        "0dfddddddddddda70",
        "0dfddddddddddaa70",
        "2bdaaaaaaaaaaa772",
        ".0b7777777777770.",
        "..2000000000002..",
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
    },
};

constexpr NineSlice kBtnStandardPressed = {
    8,
    8,
    20,
    {
        "..2000000000002..",
        ".044444444444440.",
        "24455555555555672",
        "04556666666666780",
        "04566666666666780",
        "04566666666666780",
        "04566666666666780",
        "04566666666666780",
        "04566666666666780",
        "04566666666666780",
        "04566666666666780",
        "04566666666666780",
        "04566666666666780",
        "04566666666666780",
        "04566666666666780",
        "04566666666666780",
        "04566666666667780",
        "24677777777777882",
        ".078888888888880.",
        "..2000000000002..",
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
    },
};

constexpr NineSlice kBtnDefault = {
    11,
    11,
    26,
    {
        "...20000000000000002...",
        "..0ddddddddddddddddc0..",
        ".0ddaaaaaaaaaaaaaaaab0.",
        "2dda720000000000027aa82",
        "0da70bdddddddddddb07a70",
        "0da2bffffffffffffdb2a70",
        "0da0dffdddddddddda70a70",
        "0da0dfddddddddddda70a70",
        "0da0dfddddddddddda70a70",
        "0da0dfddddddddddda70a70",
        "0da0dfddddddddddda70a70",
        "0da0dfddddddddddda70a70",
        "0da0dfddddddddddda70a70",
        "0da0dfddddddddddda70a70",
        "0da0dfddddddddddda70a70",
        "0da0dfddddddddddda70a70",
        "0da0dfddddddddddda70a70",
        "0da0dfddddddddddda70a70",
        "0da0dfddddddddddda70a70",
        "0da0dfddddddddddaa70a70",
        "0da2bdaaaaaaaaaaa772a70",
        "0da70b77777777777707a70",
        "2caa720000000000027a872",
        ".0baaaaaaaaaaaaaaaa870.",
        "..0877777777777777770..",
        "...20000000000000002...",
    },
};

constexpr NineSlice kBtnDefaultPressed = {
    11,
    11,
    26,
    {
        "...20000000000000002...",
        "..0ddddddddddddddddc0..",
        ".0ddaaaaaaaaaaaaaaaab0.",
        "2dda720000000000027aa82",
        "0da70444444444444407a70",
        "0da24455555555555672a70",
        "0da04556666666666780a70",
        "0da04566666666666780a70",
        "0da04566666666666780a70",
        "0da04566666666666780a70",
        "0da04566666666666780a70",
        "0da04566666666666780a70",
        "0da04566666666666780a70",
        "0da04566666666666780a70",
        "0da04566666666666780a70",
        "0da04566666666666780a70",
        "0da04566666666666780a70",
        "0da04566666666666780a70",
        "0da04566666666666780a70",
        "0da04566666666667780a70",
        "0da24677777777777882a70",
        "0da70788888888888807a70",
        "2caa720000000000027a872",
        ".0baaaaaaaaaaaaaaaa870.",
        "..0877777777777777770..",
        "...20000000000000002...",
    },
};

void paintNine(QPainter &p, const NineSlice &ns, int x, int y, int w)
{
    if (w <= 0)
        return;
    const int lw = std::min(ns.left, w / 2);
    const int rw = std::min(ns.right, w - lw);
    const int mid = std::max(0, w - lw - rw);
    const int rightStart = ns.left + 1;

    for (int row = 0; row < ns.height; ++row) {
        const char *str = ns.rows[row];
        if (!str)
            continue;
        const int srcLen = static_cast<int>(std::strlen(str));

        for (int i = 0; i < lw && i < srcLen; ++i) {
            const int n = hexNibble(str[i]);
            if (n >= 0) {
                p.setPen(grayColor(n));
                p.drawPoint(x + i, y + row);
            }
        }

        if (mid > 0 && ns.left < srcLen) {
            const int n = hexNibble(str[ns.left]);
            if (n >= 0)
                p.fillRect(x + lw, y + row, mid, 1, grayColor(n));
        }

        for (int i = 0; i < rw; ++i) {
            const int k = rightStart + i + (ns.right - rw);
            if (k >= 0 && k < srcLen) {
                const int n = hexNibble(str[k]);
                if (n >= 0) {
                    p.setPen(grayColor(n));
                    p.drawPoint(x + lw + mid + i, y + row);
                }
            }
        }
    }
}

QColor checkGrey(char ch)
{
    switch (ch) {
    case 'K': return QColor(0x00, 0x00, 0x00);
    case 'W': return QColor(0xFF, 0xFF, 0xFF);
    case '5': return grayColor(0x5);
    case '7': return grayColor(0x7);
    case '8': return grayColor(0x8);
    case '9': return grayColor(0x9);
    case 'a': return grayColor(0xA);
    default: return grayColor(0xD);
    }
}

void paintCheckbox(QPainter &p, int x, int y, int checkState, bool pressed, bool enabled)
{
    static const char *const kCheckOff[12] = {
        "KKKKKKKKKKKK",
        "KWWWWWWWWWdK",
        "KWdddddddd8K",
        "KWdddddddd8K",
        "KWdddddddd8K",
        "KWdddddddd8K",
        "KWdddddddd8K",
        "KWdddddddd8K",
        "KWdddddddd8K",
        "KWdddddddd8K",
        "Kd888888888K",
        "KKKKKKKKKKKK",
    };
    static const char *const kCheckOn[12] = {
        "KKKKKKKKKKKK..",
        "KWWWWWWWWWdKK.",
        "KWddddddddKK7a",
        "KWdddddddKKKa.",
        "KWddddddKK5K..",
        "KWKKdddKK77K..",
        "KWdKKdKK7a8K..",
        "KWdaKKK7ad8K..",
        "KWddaK7add8K..",
        "KWddd7addd8K..",
        "Kd888888888K..",
        "KKKKKKKKKKKK..",
    };

    const char *const *rows = (checkState == 1) ? kCheckOn : kCheckOff;
    for (int j = 0; j < kCheckboxSize; ++j) {
        const char *row = rows[j];
        for (int i = 0; row[i]; ++i) {
            const char ch = row[i];
            if (ch == '.')
                continue;
            const bool bevelHi = (ch == 'W');
            const bool bevelLo = (ch == '8' && (j == 10 || i == 10));
            const bool face = (ch == 'd');
            QColor v = checkGrey(ch);
            if (pressed && enabled) {
                v = bevelHi ? grayColor(0x5)
                            : face ? grayColor(0x7)
                                   : bevelLo ? grayColor(0x9)
                                             : v;
            } else if (!enabled) {
                v = (ch == 'K') ? grayColor(0x8) : grayColor(0xD);
            }
            p.setPen(v);
            p.drawPoint(x + i, y + j);
        }
    }

    if (checkState == 2) {
        const QColor v = enabled ? QColor(0x00, 0x00, 0x00) : grayColor(0x8);
        p.fillRect(x + 2, y + 5, 8, 2, v);
    }
}

QColor radioGrey(char ch)
{
    switch (ch) {
    case 'K': return QColor(0x00, 0x00, 0x00);
    case 'W': return QColor(0xFF, 0xFF, 0xFF);
    case '3': return grayColor(0x3);
    case '5': return grayColor(0x5);
    case '6': return grayColor(0x6);
    case '8': return grayColor(0x8);
    case '9': return grayColor(0x9);
    case 'A': return grayColor(0xA);
    case 'B': return grayColor(0xB);
    case 'C': return grayColor(0xC);
    case 'E': return grayColor(0xE);
    default: return grayColor(0xD);
    }
}

void paintRadio(QPainter &p, int x, int y, bool on, bool pressed, bool enabled)
{
    static const char *const kRadioOff[12] = {
        "....5KK5....",
        "..K6   C5K..",
        ".KC EWWW 9K.",
        ".6 EWWEE C5.",
        "5 EWWEE  C95",
        "K WWEE  CC9K",
        "K WEE  CCB9K",
        "5CWE  CCBB95",
        ".6   CCBB95.",
        ".K9CCCBB99K.",
        "..K699995K..",
        "....5KK5....",
    };
    static const char *const kRadioOn[12] = {
        "....3KK3....",
        "..3K56665K..",
        ".35688899AK.",
        ".K68KKKKAA5.",
        "358KKKKKKAC5",
        "K68KKKKKKCCK",
        "K68KKKKKKC K",
        "369KKKKKK W5",
        ".39AKKKK W5.",
        ".3AAACC WWK.",
        "..35CC W5K..",
        "....5KK5....",
    };

    const char *const *rows = on ? kRadioOn : kRadioOff;
    for (int j = 0; j < kRadioSize; ++j) {
        const char *row = rows[j];
        for (int i = 0; i < kRadioSize && row[i]; ++i) {
            const char ch = row[i];
            if (ch == '.')
                continue;
            QColor v = radioGrey(ch);
            if (pressed && enabled) {
                v = QColor(
                    (v.red() / 2) | 0x20,
                    (v.green() / 2) | 0x20,
                    (v.blue() / 2) | 0x20);
            } else if (!enabled) {
                v = (ch == 'K' || ch == '3' || ch == '5' || ch == '6')
                    ? grayColor(0x8)
                    : grayColor(0xD);
            }
            p.setPen(v);
            p.drawPoint(x + i, y + j);
        }
    }
}

void paintEditFrameOutline(QPainter &p, const QRect &outerRect)
{
    if (outerRect.width() < 4 || outerRect.height() < 4)
        return;
    const int x0 = outerRect.left() + 1;
    const int y0 = outerRect.top() + 1;
    const int x1 = outerRect.right() - 1;
    const int y1 = outerRect.bottom() - 1;

    // #888 outer top/left, #FFF outer bottom/right, #000 border
    p.setPen(grayColor(0x8));
    p.drawLine(x0 - 1, y0 - 1, x1, y0 - 1);
    p.drawLine(x0 - 1, y0 - 1, x0 - 1, y1);

    p.setPen(QColor(0xFF, 0xFF, 0xFF));
    p.drawLine(x0, y1 + 1, x1 + 1, y1 + 1);
    p.drawLine(x1 + 1, y0, x1 + 1, y1 + 1);

    p.setPen(QColor(0x00, 0x00, 0x00));
    p.drawRect(x0, y0, x1 - x0, y1 - y0);
}

struct SbAccent {
    QColor corner{0xEE, 0xEE, 0xEE};
    QColor gripHi{0xEE, 0xEE, 0xEE};
    QColor light{0xCC, 0xCC, 0xFF};
    QColor body{0x99, 0x99, 0xFF};
    QColor dark{0x66, 0x66, 0xCC};
    QColor shadow{0x33, 0x33, 0x99};
};

struct SbGeom {
    bool vertical = false;
    bool enabled = false;
    int length = 0;
    int groove0 = 0;
    int grooveLen = 0;
    int slider = kSbSliderMin;
    int thumbLen = kSbThumb;
    int subLine = 0;
    int range = 0;
};

SbGeom computeSbGeom(const QStyleOptionSlider *sb)
{
    SbGeom g;
    g.vertical = (sb->orientation == Qt::Vertical);
    g.length = g.vertical ? sb->rect.height() : sb->rect.width();
    g.enabled = (sb->state & QStyle::State_Enabled) && sb->maximum > sb->minimum;

    const int incSep = g.length - 2 - kSbArrow;
    const int decSep = kSbArrow + 1;
    g.groove0 = decSep;
    const int end = incSep;
    const int track = end - g.groove0 - 1;

    const int visible = std::max(1, sb->pageStep);
    const int total = std::max(1, sb->maximum - sb->minimum + sb->pageStep);
    if (track > kSbThumb + 1 && total > 0) {
        const long long len = (static_cast<long long>(track) * std::min(visible, total)) / total;
        g.thumbLen = static_cast<int>(std::clamp<long long>(len, kSbThumb, std::max(kSbThumb, track - 1)));
    } else {
        g.thumbLen = kSbThumb;
    }

    g.range = std::max(0, end - g.groove0 - (g.thumbLen + 2));
    g.slider = g.thumbLen + 2;
    g.grooveLen = std::max(0, g.range + g.slider);
    g.subLine = 0;
    return g;
}

class SbPainter {
public:
    SbPainter(QPainter &p, int x, int y, bool vertical)
        : m_p(p)
        , m_x(x)
        , m_y(y)
        , m_vertical(vertical)
    {
    }

    void put(int a, int b, const QColor &c)
    {
        m_p.setPen(c);
        if (m_vertical)
            m_p.drawPoint(m_x + b, m_y + a);
        else
            m_p.drawPoint(m_x + a, m_y + b);
    }

    void run(int a0, int a1, int b, const QColor &c)
    {
        if (a1 < a0)
            return;
        if (m_vertical)
            m_p.fillRect(m_x + b, m_y + a0, 1, a1 - a0 + 1, c);
        else
            m_p.fillRect(m_x + a0, m_y + b, a1 - a0 + 1, 1, c);
    }

    void col(int a, int b0, int b1, const QColor &c)
    {
        if (b1 < b0)
            return;
        if (m_vertical)
            m_p.fillRect(m_x + b0, m_y + a, b1 - b0 + 1, 1, c);
        else
            m_p.fillRect(m_x + a, m_y + b0, 1, b1 - b0 + 1, c);
    }

    void arrowGlyph(int boxA, int dir, const QColor &c)
    {
        static constexpr int widths[8] = {1, 2, 3, 4, 4, 3, 2, 1};
        const int off = boxA + (kSbArrow - 4) / 2;
        for (int i = 0; i < 8; ++i) {
            for (int k = 0; k < widths[i]; ++k) {
                const int a = (dir < 0) ? (off + 3 - k) : (off + k);
                put(a, 4 + i, c);
            }
        }
    }

    void arrowBox(int boxA, int dir, bool enabled)
    {
        if (!enabled) {
            for (int b = 1; b <= kSbArrow; ++b)
                run(boxA, boxA + kSbArrow - 1, b, grayColor(0xE));
            arrowGlyph(boxA, dir, grayColor(0x8));
            return;
        }
        for (int b = 1; b <= kSbArrow; ++b)
            run(boxA, boxA + kSbArrow - 1, b, grayColor(0xD));
        run(boxA, boxA + kSbArrow - 2, 1, QColor(0xFF, 0xFF, 0xFF));
        col(boxA, 1, kSbArrow - 1, QColor(0xFF, 0xFF, 0xFF));
        col(boxA + kSbArrow - 1, 2, kSbArrow, grayColor(0xB));
        run(boxA + 1, boxA + kSbArrow - 1, kSbArrow, grayColor(0xB));
        arrowGlyph(boxA, dir, QColor(0x00, 0x00, 0x00));
    }

    void track(int a0, int a1)
    {
        if (a1 < a0)
            return;
        run(a0, a1, 1, grayColor(0x7));
        run(a0, a1, 2, grayColor(0x8));
        for (int b = 3; b <= 12; ++b)
            run(a0, a1, b, grayColor(0xA));
        run(a0, a1, 13, grayColor(0xB));
        run(a0, a1, 14, grayColor(0xC));
    }

    void trackShadow(int a, int aMax)
    {
        if (a <= aMax)
            col(a, 1, 13, grayColor(0x7));
        if (a + 1 <= aMax)
            col(a + 1, 2, 12, grayColor(0x8));
    }

    void thumb(int lineA, int len, const SbAccent &ac)
    {
        const int t0 = lineA + 1;
        const int t1 = lineA + len;
        col(lineA, 1, kSbArrow, QColor(0x00, 0x00, 0x00));
        col(lineA + len + 1, 1, kSbArrow, QColor(0x00, 0x00, 0x00));

        put(t0, 1, ac.corner);
        run(t0 + 1, t1 - 1, 1, ac.light);
        put(t1, 1, ac.body);
        for (int b = 2; b <= kSbArrow - 1; ++b) {
            put(t0, b, ac.light);
            run(t0 + 1, t1 - 1, b, ac.body);
            put(t1, b, ac.dark);
        }
        put(t0, kSbArrow, ac.body);
        run(t0 + 1, t1, kSbArrow, ac.dark);

        for (int r = 0; r < 4; ++r) {
            const int la = t0 + (len - 8) / 2 + 2 * r;
            put(la, 4, ac.gripHi);
            col(la, 5, 10, ac.light);
            col(la + 1, 5, 11, ac.shadow);
        }
    }

private:
    QPainter &m_p;
    int m_x;
    int m_y;
    bool m_vertical;
};

void paintZacosScrollBar(QPainter &p, const QStyleOptionSlider *sb)
{
    const QRect r = sb->rect;
    if (r.width() < 4 || r.height() < 4)
        return;

    const SbGeom g = computeSbGeom(sb);
    const int L = g.length;
    if (L < 4)
        return;

    SbPainter s(p, r.left(), r.top(), g.vertical);
    const int incSep = L - 2 - kSbArrow;
    const int decSep = kSbArrow + 1;
    const QColor black(0x00, 0x00, 0x00);
    const QColor line = g.enabled ? black : grayColor(0x5);

    s.run(0, L - 1, 0, black);
    s.run(0, L - 1, kSbWidth - 1, black);
    s.col(0, 0, kSbWidth - 1, black);
    s.col(L - 1, 0, kSbWidth - 1, black);

    if (L < 2 * kSbEnd + 2) {
        for (int b = 1; b <= kSbArrow; ++b)
            s.run(1, L - 2, b, grayColor(0xE));
        return;
    }

    s.col(decSep, 1, kSbArrow, line);
    s.col(incSep, 1, kSbArrow, line);
    s.arrowBox(1, -1, g.enabled);
    s.arrowBox(incSep + 1, +1, g.enabled);

    const int t0 = g.groove0 + 1;
    const int t1 = incSep - 1;
    if (!g.enabled) {
        for (int b = 1; b <= kSbArrow; ++b)
            s.run(t0, t1, b, grayColor(0xE));
        return;
    }

    s.track(t0, t1);
    const int thumbPos = std::clamp(
        QStyle::sliderPositionFromValue(
            sb->minimum, sb->maximum, sb->sliderPosition, g.range, sb->upsideDown),
        0,
        g.range);
    const int lineA = g.groove0 + thumbPos;
    if (lineA > g.groove0)
        s.trackShadow(t0, lineA - 1);

    const int maxDrawableThumb = std::max(0, t1 - lineA);
    if (maxDrawableThumb >= 10) {
        const int drawLen = std::min(g.thumbLen, maxDrawableThumb);
        const SbAccent accent;
        s.thumb(lineA, drawLen, accent);
        s.trackShadow(lineA + drawLen + 2, t1);
    }
}

bool isInsideCompositeControl(const QWidget *widget)
{
    if (!widget)
        return false;
    const QWidget *parent = widget->parentWidget();
    if (!parent)
        return false;
    return qobject_cast<const QAbstractScrollArea *>(parent) != nullptr
        || qobject_cast<const QComboBox *>(parent) != nullptr
        || qobject_cast<const QAbstractSpinBox *>(parent) != nullptr;
}

}  // namespace

NetzacZacos9Style::NetzacZacos9Style()
    : QProxyStyle(QStyleFactory::create(QStringLiteral("Fusion")))
{
    setObjectName(QStringLiteral("netzac-zacos9"));
}

void NetzacZacos9Style::polish(QPalette &palette)
{
    QProxyStyle::polish(palette);
    const QColor face(0xDD, 0xDD, 0xDD);
    const QColor base(0xFF, 0xFF, 0xFF);
    const QColor ink(0x00, 0x00, 0x00);
    const QColor dim(0x88, 0x88, 0x88);
    const QColor highlight(0x66, 0x66, 0xCC);
    const QColor highlightInk(0xFF, 0xFF, 0xFF);

    palette.setColor(QPalette::Window, face);
    palette.setColor(QPalette::Button, face);
    palette.setColor(QPalette::Base, base);
    palette.setColor(QPalette::AlternateBase, QColor(0xEE, 0xEE, 0xEE));
    palette.setColor(QPalette::WindowText, ink);
    palette.setColor(QPalette::ButtonText, ink);
    palette.setColor(QPalette::Text, ink);
    palette.setColor(QPalette::BrightText, base);
    palette.setColor(QPalette::ToolTipBase, QColor(0xFF, 0xFF, 0xCC));
    palette.setColor(QPalette::ToolTipText, ink);
    palette.setColor(QPalette::Highlight, highlight);
    palette.setColor(QPalette::HighlightedText, highlightInk);
    palette.setColor(QPalette::Link, QColor(0x00, 0x00, 0xEE));
    palette.setColor(QPalette::LinkVisited, QColor(0x55, 0x1A, 0x8B));
    palette.setColor(QPalette::Light, base);
    palette.setColor(QPalette::Midlight, QColor(0xEE, 0xEE, 0xEE));
    palette.setColor(QPalette::Dark, dim);
    palette.setColor(QPalette::Mid, QColor(0xBB, 0xBB, 0xBB));
    palette.setColor(QPalette::Shadow, ink);

    for (const QPalette::ColorRole role :
         {QPalette::WindowText, QPalette::ButtonText, QPalette::Text}) {
        palette.setColor(QPalette::Disabled, role, dim);
    }
}

int NetzacZacos9Style::pixelMetric(
    PixelMetric metric,
    const QStyleOption *option,
    const QWidget *widget) const
{
    switch (metric) {
    case PM_ScrollBarExtent:
        return kSbWidth;
    case PM_ScrollBarSliderMin:
        return kSbSliderMin;
    case PM_IndicatorWidth:
    case PM_IndicatorHeight:
        return kCheckboxSize;
    case PM_ExclusiveIndicatorWidth:
    case PM_ExclusiveIndicatorHeight:
        return kRadioSize;
    default:
        return QProxyStyle::pixelMetric(metric, option, widget);
    }
}

QSize NetzacZacos9Style::sizeFromContents(
    ContentsType type,
    const QStyleOption *option,
    const QSize &contentsSize,
    const QWidget *widget) const
{
    QSize s = QProxyStyle::sizeFromContents(type, option, contentsSize, widget);
    if (type == CT_PushButton) {
        const auto *bo = qstyleoption_cast<const QStyleOptionButton *>(option);
        const bool isDefault = bo && (bo->features & QStyleOptionButton::DefaultButton);
        const int minH = kButtonH + (isDefault ? 2 * kButtonRing : 0);
        s.setHeight(std::max(s.height(), minH));
        s.setWidth(std::max(s.width(), kButtonMinW));
    }
    return s;
}

void NetzacZacos9Style::drawPrimitive(
    PrimitiveElement element,
    const QStyleOption *option,
    QPainter *painter,
    const QWidget *widget) const
{
    if (!option || !painter)
        return;

    const QRect r = option->rect;
    const bool enabled = (option->state & State_Enabled) != 0;
    const bool pressed = (option->state & State_Sunken) != 0;

    switch (element) {
    case PE_PanelTipLabel:
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, false);
        painter->fillRect(r, option->palette.color(QPalette::ToolTipBase));
        painter->setPen(option->palette.color(QPalette::ToolTipText));
        painter->drawRect(r.adjusted(0, 0, -1, -1));
        painter->restore();
        return;

    case PE_IndicatorCheckBox: {
        const int state = (option->state & State_NoChange) ? 2
            : (option->state & State_On)                 ? 1
                                                         : 0;
        const int x = r.left() + (r.width() - kCheckboxSize) / 2;
        const int y = r.top() + (r.height() - kCheckboxSize) / 2;
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, false);
        paintCheckbox(*painter, x, y, state, pressed, enabled);
        painter->restore();
        return;
    }

    case PE_IndicatorRadioButton: {
        const int x = r.left() + (r.width() - kRadioSize) / 2;
        const int y = r.top() + (r.height() - kRadioSize) / 2;
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, false);
        paintRadio(*painter, x, y, (option->state & State_On) != 0, pressed, enabled);
        painter->restore();
        return;
    }

    case PE_PanelLineEdit: {
        if (isInsideCompositeControl(widget))
            break;
        const auto *fo = qstyleoption_cast<const QStyleOptionFrame *>(option);
        const bool hasFrame = !fo || fo->lineWidth > 0;
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, false);
        if (hasFrame && r.width() >= 4 && r.height() >= 4) {
            painter->fillRect(
                r.adjusted(2, 2, -2, -2), option->palette.color(QPalette::Base));
            paintEditFrameOutline(*painter, r);
        } else {
            painter->fillRect(r, option->palette.color(QPalette::Base));
        }
        painter->restore();
        return;
    }

    case PE_FrameLineEdit: {
        if (isInsideCompositeControl(widget))
            break;
        const auto *fo = qstyleoption_cast<const QStyleOptionFrame *>(option);
        if (fo && fo->lineWidth <= 0)
            return;
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, false);
        paintEditFrameOutline(*painter, r);
        painter->restore();
        return;
    }

    default:
        break;
    }

    QProxyStyle::drawPrimitive(element, option, painter, widget);
}

void NetzacZacos9Style::drawControl(
    ControlElement element,
    const QStyleOption *option,
    QPainter *painter,
    const QWidget *widget) const
{
    if (!option || !painter)
        return;

    if (element == CE_PushButtonBevel) {
        const auto *bo = qstyleoption_cast<const QStyleOptionButton *>(option);
        if (bo && (bo->features & QStyleOptionButton::Flat)
            && !(bo->state & (State_Sunken | State_On))) {
            return;
        }
        const bool isDefault = bo && (bo->features & QStyleOptionButton::DefaultButton);
        const bool enabled = (option->state & State_Enabled) != 0;
        const bool pressed = enabled && (option->state & (State_Sunken | State_On));
        const QRect r = option->rect;
        const int pad = isDefault ? kButtonRing : 0;
        const int y = std::max((r.height() - kButtonH) / 2, pad);
        const int w = std::max(r.width() - 2 * pad, 8);

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, false);
        if (isDefault) {
            paintNine(
                *painter,
                pressed ? kBtnDefaultPressed : kBtnDefault,
                r.left(),
                r.top() + y - kButtonRing,
                w + 2 * kButtonRing);
        } else {
            paintNine(
                *painter,
                pressed ? kBtnStandardPressed : kBtnStandard,
                r.left(),
                r.top() + y,
                w);
        }
        painter->restore();
        return;
    }

    if (element == CE_PushButtonLabel) {
        const auto *bo = qstyleoption_cast<const QStyleOptionButton *>(option);
        if (bo && (bo->state & State_Enabled) && (bo->state & (State_Sunken | State_On))) {
            QStyleOptionButton opt(*bo);
            opt.palette.setColor(QPalette::ButtonText, Qt::white);
            QProxyStyle::drawControl(element, &opt, painter, widget);
            return;
        }
    }

    QProxyStyle::drawControl(element, option, painter, widget);
}

QRect NetzacZacos9Style::subControlRect(
    ComplexControl cc,
    const QStyleOptionComplex *opt,
    SubControl sc,
    const QWidget *widget) const
{
    const auto *sb = (cc == CC_ScrollBar)
        ? qstyleoption_cast<const QStyleOptionSlider *>(opt)
        : nullptr;
    if (!sb)
        return QProxyStyle::subControlRect(cc, opt, sc, widget);

    const bool vertical = (sb->orientation == Qt::Vertical);
    const QRect r = sb->rect;
    const SbGeom g = computeSbGeom(sb);
    auto along = [&](int a0, int len) {
        return vertical ? QRect(r.x(), r.y() + a0, r.width(), len)
                        : QRect(r.x() + a0, r.y(), len, r.height());
    };

    const int travel = (g.range > 0 && sb->maximum > sb->minimum)
        ? QStyle::sliderPositionFromValue(
              sb->minimum, sb->maximum, sb->sliderPosition, g.range, sb->upsideDown)
        : 0;
    const int thumb0 = g.groove0 + travel;

    switch (sc) {
    case SC_ScrollBarSubLine:
        return along(g.subLine, kSbEnd);
    case SC_ScrollBarAddLine:
        return along(g.length - kSbEnd, kSbEnd);
    case SC_ScrollBarGroove:
        return along(g.groove0, g.grooveLen);
    case SC_ScrollBarSlider:
        return along(thumb0, g.slider);
    case SC_ScrollBarSubPage:
        return along(g.groove0, std::max(0, thumb0 - g.groove0));
    case SC_ScrollBarAddPage:
        return along(
            thumb0 + g.slider,
            std::max(0, g.groove0 + g.grooveLen - (thumb0 + g.slider)));
    default:
        return QRect();
    }
}

void NetzacZacos9Style::drawComplexControl(
    ComplexControl cc,
    const QStyleOptionComplex *opt,
    QPainter *painter,
    const QWidget *widget) const
{
    const auto *sb = (cc == CC_ScrollBar)
        ? qstyleoption_cast<const QStyleOptionSlider *>(opt)
        : nullptr;
    if (!sb || !painter) {
        QProxyStyle::drawComplexControl(cc, opt, painter, widget);
        return;
    }

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, false);
    paintZacosScrollBar(*painter, sb);
    painter->restore();
}

void drawZacos9PopupButton(QPainter &painter, const QRect &rect, bool enabled, bool down)
{
    const int x = rect.left();
    const int y = rect.top();
    const int x1 = rect.right();
    const int y1 = rect.bottom();
    if (x1 - x < kPopupArrowsW + 6 || y1 - y < 10)
        return;

    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, false);

    const QColor ink = enabled ? QColor(0x00, 0x00, 0x00) : grayColor(0x8);
    const QColor face = (down && enabled) ? grayColor(0xC) : grayColor(0xD);
    painter.fillRect(x + 1, y + 1, x1 - x - 1, y1 - y - 1, face);

    const QColor frame = enabled ? QColor(0x00, 0x00, 0x00) : grayColor(0x8);
    painter.setPen(frame);
    painter.drawLine(x + 3, y, x1 - 3, y);
    painter.drawLine(x + 3, y1, x1 - 3, y1);
    painter.drawLine(x, y + 3, x, y1 - 3);
    painter.drawLine(x1, y + 3, x1, y1 - 3);

    const int corners[4][2] = {{x, y}, {x1, y}, {x, y1}, {x1, y1}};
    for (int i = 0; i < 4; ++i) {
        const int cx = corners[i][0];
        const int cy = corners[i][1];
        const int dx = (cx == x) ? 1 : -1;
        const int dy = (cy == y) ? 1 : -1;
        painter.setPen(frame);
        painter.drawPoint(cx + dx, cy + dy);
        painter.setPen(enabled ? grayColor(0x2) : grayColor(0x8));
        painter.drawPoint(cx + 2 * dx, cy);
        painter.drawPoint(cx, cy + 2 * dy);
    }

    const int div = x1 - kPopupArrowsW;
    if (enabled) {
        const QColor hi = down ? grayColor(0x8) : QColor(0xFF, 0xFF, 0xFF);
        const QColor lo = down ? QColor(0xFF, 0xFF, 0xFF) : grayColor(0xA);
        painter.setPen(hi);
        painter.drawLine(x + 2, y + 1, div - 1, y + 1);
        painter.drawLine(x + 1, y + 2, x + 1, y1 - 2);
        painter.setPen(lo);
        painter.drawLine(x + 2, y1 - 1, div, y1 - 1);
        painter.drawLine(div, y + 2, div, y1 - 2);

        const int ax = div + 2;
        painter.setPen(grayColor(0xB));
        painter.drawPoint(div + 1, y1 - 1);
        painter.drawPoint(x1 - 2, y + 1);

        painter.setPen(hi);
        painter.drawLine(ax, y + 2, x1 - 3, y + 2);
        painter.drawLine(ax, y + 2, ax, y1 - 3);

        painter.setPen(lo);
        painter.drawLine(x1 - 2, y + 3, x1 - 2, y1 - 3);
        painter.drawLine(ax + 1, y1 - 2, x1 - 3, y1 - 2);
        painter.drawPoint(x1 - 1, y + 2);

        painter.setPen(grayColor(0x7));
        painter.drawLine(x1 - 1, y + 3, x1 - 1, y1 - 2);
        painter.drawLine(ax, y1 - 1, x1 - 2, y1 - 1);
        painter.drawLine(x1 - 2, y1 - 2, x1 - 1, y1 - 2);
    } else {
        painter.setPen(grayColor(0xA));
        painter.drawLine(div, y + 2, div, y1 - 2);
    }

    const int mid = x1 - 11;
    const int cy = (y + y1) / 2;
    painter.setPen(ink);
    for (int i = 0; i < 4; ++i) {
        painter.drawLine(mid - i, cy - 5 + i, mid + i, cy - 5 + i);
        painter.drawLine(mid - i, cy + 5 - i, mid + i, cy + 5 - i);
    }

    painter.restore();
}
