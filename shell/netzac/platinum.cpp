#include "platinum.h"

#include <QApplication>
#include <QFontDatabase>
#include <QImage>
#include <QLinearGradient>
#include <QPen>
#include <QPoint>
#include <QPolygon>
#include <QSet>
#include <QStyle>
#include <QStyleFactory>
#include <QStyleHints>
#include <QStyleOptionButton>
#include <QWidget>

QFont zacosFont(const QString &preferred, int pixelSize, bool bold)
{
    const QStringList famList = QFontDatabase::families();
    const QSet<QString> available(famList.begin(), famList.end());
    QFont font;
    if (available.contains(preferred)) {
        font = QFont(preferred);
    } else if (available.contains(QStringLiteral("ZacOS 9 System"))) {
        font = QFont(QStringLiteral("ZacOS 9 System"));
    } else if (available.contains(QStringLiteral("ZacOS 9 Views"))) {
        font = QFont(QStringLiteral("ZacOS 9 Views"));
    } else {
#if defined(Q_OS_WIN)
        // No ZacOS fonts on Windows: Tahoma follows the Platinum metrics much
        // closer than the generic "Sans Serif" alias.
        font = available.contains(QStringLiteral("Tahoma"))
            ? QFont(QStringLiteral("Tahoma"))
            : QFont(QStringLiteral("Sans Serif"));
#else
        font = QFont(QStringLiteral("Sans Serif"));
#endif
    }
    font.setPixelSize(pixelSize);
    font.setBold(bold);
#if defined(Q_OS_WIN)
    // Windows substitutes a different font for the ZacOS families, and the
    // classic no-antialias strategy turns the small pixel sizes into blobs.
    font.setStyleStrategy(QFont::PreferAntialias);
#else
    font.setStyleStrategy(QFont::NoAntialias);
#endif
    return font;
}

QFont zacosMonoFont(int pixelSize)
{
    const QStringList famList = QFontDatabase::families();
    const QSet<QString> available(famList.begin(), famList.end());
    QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    static constexpr const char *kPreferredMono[] = {
        "ZacOS 9 Mono",
        "ZacOS 9 Fixed",
        "Nimbus Mono PS",
        "Liberation Mono",
        "DejaVu Sans Mono",
    };
    for (const char *candidate : kPreferredMono) {
        const QString name = QString::fromLatin1(candidate);
        if (available.contains(name)) {
            font = QFont(name);
            break;
        }
    }
    font.setFixedPitch(true);
    font.setStyleHint(QFont::Monospace);
    font.setPixelSize(pixelSize);
#if defined(Q_OS_WIN)
    font.setStyleStrategy(QFont::PreferAntialias);
#else
    font.setStyleStrategy(QFont::NoAntialias);
#endif
    return font;
}

namespace {

QString zacos9StyleKey()
{
    const QStringList keys = QStyleFactory::keys();
    for (const QString &key : keys) {
        if (key.compare(QStringLiteral("zacos9"), Qt::CaseInsensitive) == 0)
            return key;
    }
    return QString();
}

QPalette makePlatinumLightPalette()
{
    QPalette pal;
    pal.setColor(QPalette::Window, QColor(QStringLiteral("#dddddd")));
    pal.setColor(QPalette::WindowText, QColor(QStringLiteral("#000000")));
    pal.setColor(QPalette::Base, QColor(QStringLiteral("#ffffff")));
    pal.setColor(QPalette::AlternateBase, QColor(QStringLiteral("#f0f0f0")));
    pal.setColor(QPalette::ToolTipBase, QColor(QStringLiteral("#ffffdc")));
    pal.setColor(QPalette::ToolTipText, QColor(QStringLiteral("#000000")));
    pal.setColor(QPalette::Text, QColor(QStringLiteral("#000000")));
    pal.setColor(QPalette::Button, QColor(QStringLiteral("#cccccc")));
    pal.setColor(QPalette::ButtonText, QColor(QStringLiteral("#000000")));
    pal.setColor(QPalette::BrightText, QColor(QStringLiteral("#ffffff")));
    pal.setColor(QPalette::Light, QColor(QStringLiteral("#ffffff")));
    pal.setColor(QPalette::Midlight, QColor(QStringLiteral("#e6e6e6")));
    pal.setColor(QPalette::Mid, QColor(QStringLiteral("#aaaaaa")));
    pal.setColor(QPalette::Dark, QColor(QStringLiteral("#888888")));
    pal.setColor(QPalette::Shadow, QColor(QStringLiteral("#000000")));
    pal.setColor(QPalette::Highlight, QColor(QStringLiteral("#6666cc")));
    pal.setColor(QPalette::HighlightedText, QColor(QStringLiteral("#ffffff")));
    pal.setColor(QPalette::Link, QColor(QStringLiteral("#0000ee")));
    pal.setColor(QPalette::LinkVisited, QColor(QStringLiteral("#551a8b")));
    const QColor disabledText(QStringLiteral("#777777"));
    pal.setColor(QPalette::Disabled, QPalette::WindowText, disabledText);
    pal.setColor(QPalette::Disabled, QPalette::Text, disabledText);
    pal.setColor(QPalette::Disabled, QPalette::ButtonText, disabledText);
    return pal;
}

QPalette makePlatinumDarkPalette()
{
    QPalette pal;
    pal.setColor(QPalette::Window, QColor(QStringLiteral("#2b2b2b")));
    pal.setColor(QPalette::WindowText, QColor(QStringLiteral("#eeeeee")));
    pal.setColor(QPalette::Base, QColor(QStringLiteral("#1e1e1e")));
    pal.setColor(QPalette::AlternateBase, QColor(QStringLiteral("#262626")));
    pal.setColor(QPalette::ToolTipBase, QColor(QStringLiteral("#333333")));
    pal.setColor(QPalette::ToolTipText, QColor(QStringLiteral("#eeeeee")));
    pal.setColor(QPalette::Text, QColor(QStringLiteral("#eeeeee")));
    pal.setColor(QPalette::Button, QColor(QStringLiteral("#363636")));
    pal.setColor(QPalette::ButtonText, QColor(QStringLiteral("#eeeeee")));
    pal.setColor(QPalette::BrightText, QColor(QStringLiteral("#ffffff")));
    pal.setColor(QPalette::Light, QColor(QStringLiteral("#4c4c4c")));
    pal.setColor(QPalette::Midlight, QColor(QStringLiteral("#3e3e3e")));
    pal.setColor(QPalette::Mid, QColor(QStringLiteral("#262626")));
    pal.setColor(QPalette::Dark, QColor(QStringLiteral("#181818")));
    pal.setColor(QPalette::Shadow, QColor(QStringLiteral("#0c0c0c")));
    pal.setColor(QPalette::Highlight, QColor(QStringLiteral("#6666cc")));
    pal.setColor(QPalette::HighlightedText, QColor(QStringLiteral("#ffffff")));
    pal.setColor(QPalette::Link, QColor(QStringLiteral("#88aaff")));
    pal.setColor(QPalette::LinkVisited, QColor(QStringLiteral("#bb99ff")));
    const QColor disabledText(QStringLiteral("#808080"));
    pal.setColor(QPalette::Disabled, QPalette::WindowText, disabledText);
    pal.setColor(QPalette::Disabled, QPalette::Text, disabledText);
    pal.setColor(QPalette::Disabled, QPalette::ButtonText, disabledText);
    return pal;
}

NetzacPreferences::UiTheme s_activeTheme = NetzacPreferences::UiTheme::Platinum;

}  // namespace

bool hasZacos9Style()
{
    return !zacos9StyleKey().isEmpty();
}

NetzacPreferences::UiTheme activeUiTheme()
{
    return s_activeTheme;
}

void applyZacos9Theme(QApplication &app, NetzacPreferences::UiTheme theme)
{
    static QString s_initialStyle;
    if (s_initialStyle.isEmpty() && app.style())
        s_initialStyle = app.style()->objectName();

    const QString zacosKey = zacos9StyleKey();
    const bool onZacos = !zacosKey.isEmpty();
    NetzacPreferences::UiTheme effective = theme;
    if (!onZacos && effective == NetzacPreferences::UiTheme::Platinum)
        effective = NetzacPreferences::UiTheme::Zacos9;

    s_activeTheme = effective;

    switch (effective) {
    case NetzacPreferences::UiTheme::Platinum:
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
        app.styleHints()->unsetColorScheme();
#endif
        if (onZacos)
            app.setStyle(zacosKey);
        if (app.style()) {
            QPalette pal = app.style()->standardPalette();
            app.style()->polish(pal);
            app.setPalette(pal);
        }
        break;
    case NetzacPreferences::UiTheme::Zacos9:
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
        app.styleHints()->setColorScheme(Qt::ColorScheme::Light);
#endif
        app.setStyle(QStringLiteral("Fusion"));
        app.setPalette(makePlatinumLightPalette());
        app.setFont(zacosFont(QStringLiteral("ZacOS 9 System"), 12));
        break;
    case NetzacPreferences::UiTheme::Light:
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
        app.styleHints()->setColorScheme(Qt::ColorScheme::Light);
#endif
#if defined(Q_OS_WIN)
        if (!s_initialStyle.isEmpty())
            app.setStyle(s_initialStyle);
        if (app.style())
            app.setPalette(app.style()->standardPalette());
#else
        app.setStyle(QStringLiteral("Fusion"));
        app.setPalette(makePlatinumLightPalette());
#endif
        app.setFont(zacosFont(QStringLiteral("ZacOS 9 System"), 12));
        break;
    case NetzacPreferences::UiTheme::Dark:
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
        app.styleHints()->setColorScheme(Qt::ColorScheme::Dark);
#endif
        app.setStyle(QStringLiteral("Fusion"));
        app.setPalette(makePlatinumDarkPalette());
        app.setFont(zacosFont(QStringLiteral("ZacOS 9 System"), 12));
        break;
    }

    for (QWidget *w : QApplication::topLevelWidgets()) {
        if (!w)
            continue;
        w->setStyle(app.style());
        w->setPalette(app.palette());
        w->update();
        for (QWidget *child : w->findChildren<QWidget *>())
            child->update();
    }
}

PlatinumColors platinumColors(const QPalette &palette)
{
    return PlatinumColors{
        palette.color(QPalette::Window),
        palette.color(QPalette::Button),
        palette.color(QPalette::Light),
        palette.color(QPalette::Midlight),
        palette.color(QPalette::Mid),
        palette.color(QPalette::Dark),
        palette.color(QPalette::Shadow),
        palette.color(QPalette::WindowText),
        palette.color(QPalette::Base),
        palette.color(QPalette::Highlight),
        palette.color(QPalette::Disabled, QPalette::WindowText),
    };
}

QPixmap makeDisabledPixmap(const QPixmap &pm)
{
    QImage img = pm.toImage().convertToFormat(QImage::Format_ARGB32);
    for (int y = 0; y < img.height(); ++y) {
        for (int x = 0; x < img.width(); ++x) {
            const QColor c = img.pixelColor(x, y);
            if (c.alpha() == 0)
                continue;
            const int lum = int(0.299 * c.red() + 0.587 * c.green() + 0.114 * c.blue());
            const int muted = 128 + (lum - 128) / 2;
            img.setPixelColor(x, y, QColor(muted, muted, muted, qMin(c.alpha(), 170)));
        }
    }
    return QPixmap::fromImage(img);
}

QPixmap makeNavIcon(const QString &name, int size)
{
    QPixmap pm(size, size);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, false);

    const QColor black(QStringLiteral("#000000"));
    const QColor white(QStringLiteral("#ffffff"));
    // Classic Mac OS 8 Navigator purple & light pink highlight palette
    const QColor purple(QStringLiteral("#685096"));
    const QColor purpleLight(QStringLiteral("#8b72ba"));
    const QColor purpleDark(QStringLiteral("#3b2a5e"));
    const QColor pinkLight(QStringLiteral("#f4c6e7"));
    const QColor pinkSoft(QStringLiteral("#ffe3f5"));
    const QColor gold(QStringLiteral("#f2c230"));
    const QColor goldDark(QStringLiteral("#996600"));
    const QColor red(QStringLiteral("#cc2222"));
    const QColor redLight(QStringLiteral("#ff6655"));
    const QColor grayLight(QStringLiteral("#eeeeee"));
    const QColor grayMid(QStringLiteral("#999999"));

    if (name == QLatin1String("back")) {
        const QPolygon arrow({
            QPoint(2, 10), QPoint(9, 3), QPoint(9, 7),
            QPoint(17, 7), QPoint(17, 13), QPoint(9, 13), QPoint(9, 17),
        });
        p.setPen(QPen(black, 1));
        p.setBrush(purple);
        p.drawPolygon(arrow);
        p.setPen(pinkLight);
        p.drawLine(4, 9, 8, 5);
        p.drawLine(9, 8, 16, 8);
        p.setPen(purpleLight);
        p.drawLine(9, 9, 16, 9);
        p.setPen(purpleDark);
        p.drawLine(9, 12, 16, 12);
    } else if (name == QLatin1String("forward")) {
        const QPolygon arrow({
            QPoint(17, 10), QPoint(10, 3), QPoint(10, 7),
            QPoint(2, 7), QPoint(2, 13), QPoint(10, 13), QPoint(10, 17),
        });
        p.setPen(QPen(black, 1));
        p.setBrush(purple);
        p.drawPolygon(arrow);
        p.setPen(pinkLight);
        p.drawLine(3, 8, 10, 8);
        p.drawLine(11, 5, 15, 9);
        p.setPen(purpleLight);
        p.drawLine(3, 9, 10, 9);
        p.setPen(purpleDark);
        p.drawLine(3, 12, 10, 12);
    } else if (name == QLatin1String("reload")) {
        p.setPen(black);
        p.setBrush(white);
        p.drawRect(5, 3, 10, 14);
        p.setPen(pinkLight);
        p.drawLine(6, 4, 14, 4);
        p.setPen(purpleDark);
        p.drawArc(2, 2, 15, 15, 30 * 16, 140 * 16);
        p.drawArc(2, 2, 15, 15, 210 * 16, 140 * 16);
        p.setBrush(purple);
        p.setPen(black);
        p.drawPolygon(QPolygon({QPoint(15, 5), QPoint(18, 9), QPoint(12, 9)}));
        p.drawPolygon(QPolygon({QPoint(4, 14), QPoint(1, 10), QPoint(7, 10)}));
        p.setPen(pinkLight);
        p.drawPoint(15, 7);
        p.drawPoint(4, 12);
    } else if (name == QLatin1String("home")) {
        p.setPen(black);
        p.setBrush(purpleDark);
        p.drawRect(13, 3, 2, 4);
        const QPolygon roof({QPoint(10, 2), QPoint(2, 10), QPoint(18, 10)});
        p.setBrush(purple);
        p.drawPolygon(roof);
        p.setPen(pinkLight);
        p.drawLine(10, 4, 4, 9);
        p.setPen(black);
        p.setBrush(pinkSoft);
        p.drawRect(4, 10, 12, 8);
        p.setPen(pinkLight);
        p.drawLine(5, 11, 15, 11);
        p.setPen(black);
        p.setBrush(purpleDark);
        p.drawRect(8, 12, 4, 6);
        p.setBrush(purpleLight);
        p.drawRect(5, 12, 2, 2);
    } else if (name == QLatin1String("find")) {
        // Purple/pink magnifying glass over white page
        p.setPen(black);
        p.setBrush(white);
        p.drawRect(3, 2, 11, 14);
        p.setPen(grayMid);
        p.drawLine(5, 5, 11, 5);
        p.drawLine(5, 8, 9, 8);
        p.drawLine(5, 11, 10, 11);
        p.setPen(black);
        p.setBrush(pinkSoft);
        p.drawEllipse(6, 5, 8, 8);
        p.setPen(pinkLight);
        p.drawArc(7, 6, 6, 6, 45 * 16, 120 * 16);
        p.setPen(QPen(purpleDark, 2));
        p.drawLine(13, 12, 17, 16);
        p.setPen(black);
        p.drawRect(6, 5, 8, 8);
    } else if (name == QLatin1String("print")) {
        p.setPen(black);
        p.setBrush(white);
        p.drawRect(5, 2, 10, 6);
        p.setPen(purpleLight);
        p.drawLine(7, 4, 13, 4);
        p.drawLine(7, 6, 11, 6);
        p.setPen(black);
        p.setBrush(pinkSoft);
        p.drawRect(2, 8, 16, 7);
        p.setPen(pinkLight);
        p.drawLine(3, 9, 17, 9);
        p.setPen(black);
        p.setBrush(purple);
        p.drawRect(4, 11, 12, 3);
        p.setBrush(white);
        p.drawRect(5, 13, 10, 5);
        p.setPen(purpleDark);
        p.drawPoint(15, 9);
    } else if (name == QLatin1String("pdf")) {
        // Classic vertical Mac OS dog-eared document with Navigator plum PDF badge
        const QPolygon sheet({
            QPoint(3, 1), QPoint(12, 1), QPoint(16, 5), QPoint(16, 18), QPoint(3, 18),
        });
        p.setPen(black);
        p.setBrush(white);
        p.drawPolygon(sheet);
        p.setPen(pinkSoft);
        p.drawLine(4, 2, 11, 2);
        p.drawLine(4, 2, 4, 17);

        const QPolygon fold({QPoint(12, 1), QPoint(16, 5), QPoint(12, 5)});
        p.setPen(black);
        p.setBrush(QColor(QStringLiteral("#e4daf2")));
        p.drawPolygon(fold);

        p.setPen(purpleLight);
        p.drawLine(5, 4, 10, 4);
        p.drawLine(5, 6, 13, 6);

        // Berry-plum PDF label banner
        p.setPen(black);
        p.setBrush(QColor(QStringLiteral("#8b3a82")));
        p.drawRect(2, 8, 15, 8);
        p.setPen(pinkLight);
        p.drawLine(3, 9, 16, 9);
        p.setPen(purpleDark);
        p.drawLine(3, 15, 16, 15);

        // Crisp 3x5 white pixel letters: P D F
        p.setPen(white);
        // P
        p.drawLine(4, 10, 4, 14);
        p.drawLine(4, 10, 6, 10);
        p.drawLine(4, 12, 6, 12);
        p.drawPoint(6, 11);
        // D
        p.drawLine(8, 10, 8, 14);
        p.drawLine(8, 10, 9, 10);
        p.drawLine(8, 14, 9, 14);
        p.drawLine(10, 11, 10, 13);
        // F
        p.drawLine(12, 10, 12, 14);
        p.drawLine(12, 10, 14, 10);
        p.drawLine(12, 12, 13, 12);
    } else if (name == QLatin1String("security_open") || name == QLatin1String("security_locked")) {
        const bool locked = (name == QLatin1String("security_locked"));
        p.setPen(QPen(black, 1));
        if (locked) {
            p.drawArc(6, 2, 8, 9, 0, 180 * 16);
            p.drawLine(6, 6, 6, 9);
            p.drawLine(14, 6, 14, 9);
            p.setBrush(gold);
            p.drawRect(4, 9, 12, 9);
            p.setPen(goldDark);
            p.drawLine(5, 17, 15, 17);
            p.setPen(black);
            p.drawRect(9, 12, 2, 3);
        } else {
            p.drawArc(4, 2, 8, 8, 0, 180 * 16);
            p.drawLine(4, 6, 4, 10);
            p.setBrush(grayLight);
            p.drawRect(5, 9, 11, 9);
            p.setPen(grayMid);
            p.drawLine(6, 17, 15, 17);
            p.setPen(black);
            p.drawRect(10, 12, 2, 3);
        }
    } else if (name == QLatin1String("calm")) {
        // Serene open-book Calm reading icon in Mac OS 8 Navigator purple/pink palette
        p.setPen(black);
        p.setBrush(purple);
        p.drawRect(2, 6, 16, 10);
        p.setPen(purpleDark);
        p.drawLine(3, 15, 16, 15);
        // Left open page (white with pink highlight and text lines)
        p.setPen(black);
        p.setBrush(white);
        p.drawRect(3, 4, 7, 10);
        p.setPen(pinkLight);
        p.drawLine(4, 5, 9, 5);
        p.setPen(purple);
        p.drawLine(5, 7, 8, 7);
        p.drawLine(5, 9, 8, 9);
        p.drawLine(5, 11, 8, 11);
        // Right open page (soft pink with white highlight and text lines)
        p.setPen(black);
        p.setBrush(pinkSoft);
        p.drawRect(10, 4, 7, 10);
        p.setPen(white);
        p.drawLine(11, 5, 16, 5);
        p.setPen(purple);
        p.drawLine(12, 7, 15, 7);
        p.drawLine(12, 9, 15, 9);
        p.drawLine(12, 11, 15, 11);
        // Center spine shadow & hanging bookmark ribbon
        p.setPen(purpleDark);
        p.drawLine(10, 5, 10, 14);
        p.setPen(black);
        p.setBrush(pinkLight);
        p.drawRect(13, 14, 2, 4);
    } else if (name == QLatin1String("stop")) {
        const QPolygon octPts({
            QPoint(6, 2), QPoint(14, 2),
            QPoint(18, 6), QPoint(18, 14),
            QPoint(14, 18), QPoint(6, 18),
            QPoint(2, 14), QPoint(2, 6),
        });
        p.setPen(black);
        p.setBrush(red);
        p.drawPolygon(octPts);
        p.setPen(redLight);
        p.drawLine(6, 3, 13, 3);
        p.drawLine(3, 6, 3, 13);
        p.setPen(white);
        p.drawLine(7, 7, 13, 13);
        p.drawLine(13, 7, 7, 13);
    }

    p.end();
    return pm;
}

QPixmap makeMiniIcon(const QString &name, int size)
{
    QPixmap pm(size, size);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, false);

    const QColor black(QStringLiteral("#000000"));
    const QColor white(QStringLiteral("#ffffff"));
    const QColor teal(QStringLiteral("#2f8f83"));
    const QColor blue(QStringLiteral("#3366aa"));
    const QColor gold(QStringLiteral("#f2c230"));
    const QColor red(QStringLiteral("#cc3333"));
    const QColor gray(QStringLiteral("#bbbbbb"));

    if (name == QLatin1String("bookmarks")) {
        p.setPen(black);
        p.setBrush(teal);
        p.drawRect(2, 1, 9, 11);
        p.setBrush(red);
        p.drawPolygon(QPolygon({QPoint(5, 2), QPoint(9, 2), QPoint(9, 10), QPoint(7, 8), QPoint(5, 10)}));
    } else if (name == QLatin1String("page")) {
        p.setPen(black);
        p.setBrush(white);
        p.drawRect(2, 1, 9, 12);
        p.setPen(blue);
        p.drawLine(4, 4, 9, 4);
        p.drawLine(4, 6, 9, 6);
        p.drawLine(4, 8, 8, 8);
    } else if (name == QLatin1String("home")) {
        p.setPen(black);
        p.setBrush(blue);
        p.drawPolygon(QPolygon({QPoint(7, 1), QPoint(1, 7), QPoint(13, 7)}));
        p.setBrush(gold);
        p.drawRect(3, 7, 8, 6);
    } else if (name == QLatin1String("search")) {
        p.setPen(black);
        p.setBrush(QColor(QStringLiteral("#aaddff")));
        p.drawEllipse(1, 1, 8, 8);
        p.drawLine(8, 8, 12, 12);
    } else if (name == QLatin1String("guide")) {
        p.setPen(black);
        p.setBrush(QColor(QStringLiteral("#88ccaa")));
        p.drawEllipse(1, 1, 11, 11);
        p.setBrush(red);
        p.drawPolygon(QPolygon({QPoint(9, 4), QPoint(7, 8), QPoint(5, 6)}));
    } else if (name == QLatin1String("folder")) {
        p.setPen(black);
        p.setBrush(gold);
        p.drawRect(1, 3, 5, 2);
        p.drawRect(1, 4, 11, 8);
    } else if (name == QLatin1String("navigator")) {
        p.setPen(black);
        p.setBrush(gold);
        p.drawEllipse(2, 2, 10, 10);
        p.setBrush(white);
        p.drawEllipse(5, 5, 4, 4);
        p.drawLine(7, 0, 7, 13);
        p.drawLine(0, 7, 13, 7);
    } else if (name == QLatin1String("mail")) {
        p.setPen(black);
        p.setBrush(white);
        p.drawRect(1, 3, 12, 8);
        p.drawLine(1, 3, 7, 8);
        p.drawLine(13, 3, 7, 8);
    } else if (name == QLatin1String("collabra")) {
        p.setPen(black);
        p.setBrush(gray);
        p.drawRect(4, 1, 8, 9);
        p.setBrush(white);
        p.drawRect(1, 4, 8, 9);
        p.setPen(blue);
        p.drawLine(3, 7, 7, 7);
        p.drawLine(3, 9, 7, 9);
    } else if (name == QLatin1String("composer")) {
        p.setPen(black);
        p.setBrush(white);
        p.drawRect(2, 2, 8, 10);
        p.setPen(red);
        p.drawLine(12, 2, 6, 10);
        p.setPen(gold);
        p.drawLine(11, 2, 5, 10);
    } else if (name == QLatin1String("security_open") || name == QLatin1String("security_locked")) {
        const bool locked = (name == QLatin1String("security_locked"));
        p.setPen(black);
        if (locked) {
            p.drawArc(4, 1, 6, 6, 0, 180 * 16);
            p.drawLine(4, 4, 4, 6);
            p.drawLine(10, 4, 10, 6);
            p.setBrush(gold);
            p.drawRect(3, 6, 8, 6);
            p.drawPoint(7, 9);
        } else {
            p.drawArc(3, 1, 6, 6, 0, 180 * 16);
            p.drawLine(3, 4, 3, 7);
            p.setBrush(QColor(QStringLiteral("#dddddd")));
            p.drawRect(4, 6, 8, 6);
            p.drawPoint(8, 9);
        }
    }

    p.end();
    return pm;
}

QPixmap makeAlertIcon(const QString &kind, int size)
{
    QPixmap pm(size, size);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, false);

    const QColor black(QStringLiteral("#000000"));
    const QColor white(QStringLiteral("#ffffff"));
    const QColor gold(QStringLiteral("#f2c230"));
    const QColor goldDark(QStringLiteral("#996600"));
    const QColor goldLight(QStringLiteral("#fff4aa"));
    const QColor blue(QStringLiteral("#6666cc"));
    const QColor purple(QStringLiteral("#685096"));
    const QColor purpleLight(QStringLiteral("#8b72ba"));
    const QColor purpleMidLight(QStringLiteral("#a98fd6"));
    const QColor purpleDark(QStringLiteral("#3b2a5e"));
    const QColor pinkLight(QStringLiteral("#f4c6e7"));
    const QColor pinkSoft(QStringLiteral("#ffe3f5"));
    const QColor plumRibbon(QStringLiteral("#8b3a82"));

    if (kind == QLatin1String("certificate")) {
        if (size != 48)
            p.scale(qreal(size) / 48.0, qreal(size) / 48.0);

        // Classic ZacOS 9 / Mac OS 8 vertical dog-eared document paired with a
        // chunky beveled Netscape Navigator purple padlock & gold keyhole.
        // 1. Classic dog-eared document sheet (top-left)
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(QStringLiteral("#7e768e")));
        p.drawPolygon(QPolygon({
            QPoint(7, 5), QPoint(27, 5), QPoint(36, 14), QPoint(36, 45), QPoint(7, 45),
        }));

        const QPolygon docPoly({
            QPoint(5, 3), QPoint(25, 3), QPoint(34, 12), QPoint(34, 43), QPoint(5, 43),
        });
        p.setBrush(white);
        p.drawPolygon(docPoly);
        p.setPen(QColor(QStringLiteral("#ece6f5")));
        p.drawLine(33, 13, 33, 42);
        p.drawLine(6, 42, 33, 42);
        p.setPen(QColor(QStringLiteral("#f6f2fb")));
        p.drawLine(32, 13, 32, 41);
        p.drawLine(6, 41, 32, 41);

        p.setPen(black);
        p.setBrush(Qt::NoBrush);
        p.drawPolygon(docPoly);

        // Folded top-right dog-ear corner
        p.setPen(QColor(QStringLiteral("#b4a6cc")));
        p.drawLine(25, 13, 33, 13);
        const QPolygon foldPoly({QPoint(25, 3), QPoint(34, 12), QPoint(25, 12)});
        p.setPen(black);
        p.setBrush(QColor(QStringLiteral("#e4daf2")));
        p.drawPolygon(foldPoly);
        p.setPen(pinkSoft);
        p.drawLine(26, 6, 26, 11);
        p.drawLine(26, 11, 31, 11);

        // Purple certificate title banner with small gold emblem
        p.setPen(black);
        p.setBrush(purple);
        p.drawRect(8, 7, 14, 5);
        p.setPen(pinkLight);
        p.drawLine(9, 8, 21, 8);
        p.setPen(purpleDark);
        p.drawLine(9, 11, 21, 11);
        p.setPen(Qt::NoPen);
        p.setBrush(gold);
        p.drawRect(9, 8, 3, 3);
        p.setPen(goldLight);
        p.drawPoint(9, 8);

        // Crisp document text lines
        p.setPen(purple);
        p.drawLine(8, 15, 29, 15);
        p.setPen(purpleLight);
        p.drawLine(8, 18, 25, 18);
        p.drawLine(8, 21, 22, 21);
        p.setPen(QColor(QStringLiteral("#b4a2d4")));
        p.drawLine(8, 24, 21, 24);
        p.drawLine(8, 27, 20, 27);

        // Bottom-left verification stamp box
        p.setPen(purpleLight);
        p.setBrush(pinkSoft);
        p.drawRect(8, 32, 11, 6);
        p.setPen(purple);
        p.drawLine(10, 35, 12, 37);
        p.drawLine(12, 37, 16, 33);

        // 2. Chunky Navigator purple padlock (overlapping lower-right)
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(QStringLiteral("#5a4b78")));
        p.drawRect(26, 30, 19, 16);

        // Metallic Platinum shackle arch
        const QPolygon shackle({
            QPoint(27, 28), QPoint(27, 22), QPoint(30, 18), QPoint(37, 18),
            QPoint(40, 22), QPoint(40, 28), QPoint(37, 28), QPoint(37, 23),
            QPoint(35, 21), QPoint(32, 21), QPoint(30, 23), QPoint(30, 28),
        });
        p.setPen(black);
        p.setBrush(QColor(QStringLiteral("#d8d2e6")));
        p.drawPolygon(shackle);
        p.setPen(white);
        p.drawLine(28, 22, 28, 27);
        p.drawLine(30, 19, 36, 19);
        p.drawPoint(29, 20);
        p.setPen(purpleLight);
        p.drawLine(39, 22, 39, 27);
        p.drawLine(31, 20, 36, 20);
        p.setPen(black);
        p.setBrush(Qt::NoBrush);
        p.drawPolygon(shackle);

        // Chamfered beveled padlock body
        const QPolygon lockBody({
            QPoint(25, 28), QPoint(42, 28), QPoint(43, 29), QPoint(43, 43),
            QPoint(42, 44), QPoint(25, 44), QPoint(24, 43), QPoint(24, 29),
        });
        p.setPen(black);
        p.setBrush(purple);
        p.drawPolygon(lockBody);
        p.setPen(pinkLight);
        p.drawLine(25, 29, 41, 29);
        p.drawLine(25, 29, 25, 42);
        p.setPen(purpleLight);
        p.drawLine(26, 30, 40, 30);
        p.drawLine(26, 30, 26, 41);
        p.setPen(purpleDark);
        p.drawLine(26, 43, 42, 43);
        p.drawLine(42, 30, 42, 43);

        // Platinum horizontal grip grooves on flanks
        for (int gy : {33, 36, 39}) {
            p.setPen(purpleDark);
            p.drawLine(26, gy, 29, gy);
            p.drawLine(39, gy, 41, gy);
            p.setPen(purpleLight);
            p.drawLine(26, gy + 1, 29, gy + 1);
            p.drawLine(39, gy + 1, 41, gy + 1);
        }

        // Gold keyhole escutcheon & crisp keyhole
        p.setPen(purpleDark);
        p.setBrush(gold);
        p.drawRect(31, 32, 6, 8);
        p.setPen(goldLight);
        p.drawLine(32, 33, 36, 33);
        p.drawLine(32, 33, 32, 39);
        p.setPen(goldDark);
        p.drawLine(36, 34, 36, 39);
        p.drawLine(32, 39, 36, 39);
        p.setPen(Qt::NoPen);
        p.setBrush(black);
        p.drawRect(33, 34, 3, 2);
        p.drawRect(34, 36, 1, 3);
    } else if (kind == QLatin1String("source")) {
        if (size != 48)
            p.scale(qreal(size) / 48.0, qreal(size) / 48.0);

        // Classic ZacOS 9 vertical dog-eared document paired with a beveled
        // Netscape Navigator purple code badge ("< / >").
        // 1. Dog-eared document sheet (top-left)
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(QStringLiteral("#7e768e")));
        p.drawPolygon(QPolygon({
            QPoint(7, 5), QPoint(27, 5), QPoint(36, 14), QPoint(36, 45), QPoint(7, 45),
        }));

        const QPolygon docPoly({
            QPoint(5, 3), QPoint(25, 3), QPoint(34, 12), QPoint(34, 43), QPoint(5, 43),
        });
        p.setBrush(white);
        p.drawPolygon(docPoly);
        p.setPen(QColor(QStringLiteral("#ece6f5")));
        p.drawLine(33, 13, 33, 42);
        p.drawLine(6, 42, 33, 42);
        p.setPen(QColor(QStringLiteral("#f6f2fb")));
        p.drawLine(32, 13, 32, 41);
        p.drawLine(6, 41, 32, 41);
        p.setPen(black);
        p.setBrush(Qt::NoBrush);
        p.drawPolygon(docPoly);

        // Folded top-right dog-ear corner
        p.setPen(QColor(QStringLiteral("#b4a6cc")));
        p.drawLine(25, 13, 33, 13);
        const QPolygon foldPoly({QPoint(25, 3), QPoint(34, 12), QPoint(25, 12)});
        p.setPen(black);
        p.setBrush(QColor(QStringLiteral("#e4daf2")));
        p.drawPolygon(foldPoly);
        p.setPen(pinkSoft);
        p.drawLine(26, 6, 26, 11);
        p.drawLine(26, 11, 31, 11);

        // Navigator purple header strip & indented code lines
        p.setPen(black);
        p.setBrush(purple);
        p.drawRect(8, 7, 14, 4);
        p.setPen(pinkLight);
        p.drawLine(9, 8, 21, 8);

        p.setPen(purple);
        p.drawLine(8, 14, 24, 14);
        p.setPen(purpleLight);
        p.drawLine(10, 17, 29, 17);
        p.drawLine(10, 20, 26, 20);
        p.setPen(QColor(QStringLiteral("#b4a2d4")));
        p.drawLine(12, 23, 21, 23);
        p.drawLine(10, 26, 18, 26);
        p.setPen(purpleLight);
        p.drawLine(8, 29, 17, 29);
        p.setPen(purple);
        p.drawLine(8, 32, 17, 32);

        // 2. Beveled Navigator purple code badge (lower-right)
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(QStringLiteral("#5a4b78")));
        p.drawRect(21, 25, 25, 21);

        const QPolygon badge({
            QPoint(20, 23), QPoint(43, 23), QPoint(44, 24), QPoint(44, 43),
            QPoint(43, 44), QPoint(20, 44), QPoint(19, 43), QPoint(19, 24),
        });
        p.setPen(black);
        p.setBrush(purple);
        p.drawPolygon(badge);
        p.setPen(pinkLight);
        p.drawLine(20, 24, 42, 24);
        p.drawLine(20, 24, 20, 42);
        p.setPen(purpleLight);
        p.drawLine(21, 25, 41, 25);
        p.drawLine(21, 25, 21, 41);
        p.setPen(purpleDark);
        p.drawLine(21, 43, 43, 43);
        p.drawLine(43, 25, 43, 43);

        // Crisp "< / >" glyph inside the badge
        // Shadow pass
        p.setPen(purpleDark);
        p.drawLine(27, 31, 24, 34);
        p.drawLine(24, 35, 27, 38);
        p.drawLine(34, 30, 30, 39);
        p.drawLine(37, 31, 40, 34);
        p.drawLine(40, 35, 37, 38);

        // "<" and ">" in bright white
        p.setPen(white);
        p.drawLine(26, 30, 23, 33);
        p.drawLine(27, 30, 24, 33);
        p.drawLine(23, 34, 26, 37);
        p.drawLine(24, 34, 27, 37);

        p.drawLine(36, 30, 39, 33);
        p.drawLine(37, 30, 40, 33);
        p.drawLine(39, 34, 36, 37);
        p.drawLine(40, 34, 37, 37);

        // "/" in Navigator gold
        p.setPen(gold);
        p.drawLine(33, 29, 29, 38);
        p.drawLine(34, 29, 30, 38);
        p.setPen(goldLight);
        p.drawPoint(33, 29);
        p.drawPoint(34, 29);
    } else if (kind == QLatin1String("question")) {
        if (size != 32)
            p.scale(qreal(size) / 32.0, qreal(size) / 32.0);

        // Original talking-figure + speech-bubble "?" homage to classic System 1.0
        // alerts, rendered in Netzac's purple & soft-pink Platinum palette.
        // 1. Shoulders & tunic
        const QPolygon bust({
            QPoint(2, 29), QPoint(2, 24), QPoint(4, 21), QPoint(6, 20),
            QPoint(12, 20), QPoint(14, 21), QPoint(16, 24), QPoint(16, 29),
        });
        p.setPen(black);
        p.setBrush(purple);
        p.drawPolygon(bust);
        p.setPen(pinkLight);
        p.drawLine(4, 22, 6, 21);
        p.drawLine(3, 24, 3, 28);
        p.setPen(purpleLight);
        p.drawLine(4, 23, 5, 22);
        p.drawLine(4, 24, 4, 28);
        p.setPen(purpleDark);
        p.drawLine(15, 24, 15, 28);
        p.drawLine(4, 28, 15, 28);

        // Collar V & neck
        p.setPen(Qt::NoPen);
        p.setBrush(pinkSoft);
        p.drawPolygon(QPolygon({QPoint(7, 20), QPoint(11, 20), QPoint(9, 23)}));
        p.setPen(black);
        p.drawRect(7, 18, 4, 2);

        // 2. Friendly 3/4-view head
        const QPolygon head({
            QPoint(5, 11), QPoint(7, 9), QPoint(12, 9), QPoint(14, 11),
            QPoint(14, 17), QPoint(12, 19), QPoint(7, 19), QPoint(5, 17),
        });
        p.setPen(Qt::NoPen);
        p.setBrush(pinkSoft);
        p.drawPolygon(head);

        // Hair cap & highlight
        p.setBrush(purpleDark);
        p.drawPolygon(QPolygon({
            QPoint(5, 15), QPoint(5, 11), QPoint(7, 9), QPoint(12, 9),
            QPoint(14, 11), QPoint(14, 12), QPoint(8, 12), QPoint(7, 13), QPoint(6, 15),
        }));
        p.setPen(purpleLight);
        p.drawLine(7, 10, 11, 10);
        p.drawPoint(6, 11);

        // Cheek highlight & shadow
        p.setPen(white);
        p.drawLine(7, 13, 7, 15);
        p.setPen(pinkLight);
        p.drawLine(7, 18, 12, 18);
        p.drawPoint(13, 16);

        // Eyes, nose & slight smile
        p.setPen(black);
        p.drawLine(9, 13, 9, 14);
        p.drawLine(12, 13, 12, 14);
        p.drawPoint(11, 15);
        p.drawLine(9, 17, 11, 17);

        // Crisp outer head outline
        p.setBrush(Qt::NoBrush);
        p.drawPolygon(head);

        // 3. Chamfered speech bubble with purple "?"
        p.setPen(purpleDark);
        p.drawLine(19, 16, 28, 16);
        p.drawLine(30, 5, 30, 14);
        p.drawPoint(29, 15);

        const QPolygon bubble({
            QPoint(18, 2), QPoint(27, 2), QPoint(29, 4), QPoint(29, 13),
            QPoint(27, 15), QPoint(19, 15), QPoint(15, 17), QPoint(17, 13),
            QPoint(16, 13), QPoint(16, 4),
        });
        p.setPen(black);
        p.setBrush(white);
        p.drawPolygon(bubble);
        p.setPen(pinkSoft);
        p.drawLine(19, 14, 27, 14);
        p.drawLine(28, 5, 28, 13);

        // Bold pixel "?"
        p.setPen(Qt::NoPen);
        p.setBrush(purple);
        p.drawRect(20, 4, 5, 2);
        p.drawRect(19, 5, 2, 2);
        p.drawRect(24, 5, 2, 3);
        p.drawRect(22, 8, 3, 1);
        p.drawRect(22, 9, 2, 2);
        p.drawRect(22, 12, 2, 2);
        p.setPen(purpleLight);
        p.drawLine(21, 4, 23, 4);
        p.setPen(purpleDark);
        p.drawLine(25, 6, 25, 7);
        p.drawPoint(23, 10);
        p.drawPoint(23, 13);
    } else if (kind == QLatin1String("warning")) {
        const QPolygon tri({QPoint(16, 2), QPoint(30, 28), QPoint(2, 28)});
        p.setPen(black);
        p.setBrush(QColor(QStringLiteral("#ffcc00")));
        p.drawPolygon(tri);
        p.setBrush(black);
        p.drawRect(15, 11, 2, 9);
        p.drawRect(15, 22, 2, 3);
    } else {
        p.setPen(black);
        p.setBrush(white);
        p.drawRect(3, 3, 26, 22);
        p.setBrush(QColor(QStringLiteral("#dddddd")));
        p.drawLine(4, 25, 29, 25);
        p.drawLine(29, 4, 29, 25);
        p.setBrush(blue);
        p.drawRect(14, 7, 4, 4);
        p.drawRect(14, 13, 4, 9);
    }

    p.end();
    return pm;
}

void drawPlatinumBevelButton(
    QPainter &painter,
    const QRect &rect,
    const QPalette &palette,
    bool enabled,
    bool down,
    bool hovered,
    const QWidget *widget)
{
    if (s_activeTheme != NetzacPreferences::UiTheme::Zacos9) {
        QStyle *st = widget ? widget->style() : QApplication::style();
        if (!st)
            return;
        QStyleOptionButton opt;
        if (widget)
            opt.initFrom(widget);
        opt.rect = rect;
        opt.palette = palette;
        opt.features = QStyleOptionButton::None;
        opt.state = QStyle::State_None;
        if (enabled)
            opt.state |= QStyle::State_Enabled;
        if (down)
            opt.state |= QStyle::State_Sunken;
        else
            opt.state |= QStyle::State_Raised;
        if (hovered && enabled)
            opt.state |= QStyle::State_MouseOver;
        painter.save();
        st->drawPrimitive(QStyle::PE_PanelButtonCommand, &opt, &painter, widget);
        painter.restore();
        return;
    }

    const int x1 = rect.left();
    const int y1 = rect.top();
    const int x2 = rect.right();
    const int y2 = rect.bottom();
    if (x2 - x1 < 4 || y2 - y1 < 4)
        return;

    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, false);

    if (!enabled) {
        painter.fillRect(rect, QColor(QStringLiteral("#dddddd")));
        painter.setPen(QColor(QStringLiteral("#aaaaaa")));
        painter.drawRect(x1, y1, x2 - x1, y2 - y1);
        painter.restore();
        return;
    }

    QColor fill, outerTl, outerBr, outerMix, innerTl, innerBr, innerMix;
    if (down) {
        fill = QColor(QStringLiteral("#888888"));
        outerTl = QColor(QStringLiteral("#222222"));
        outerBr = QColor(QStringLiteral("#666666"));
        outerMix = QColor(QStringLiteral("#555555"));
        innerTl = QColor(QStringLiteral("#555555"));
        innerBr = QColor(QStringLiteral("#aaaaaa"));
        innerMix = QColor(QStringLiteral("#888888"));
    } else {
        fill = hovered ? QColor(QStringLiteral("#d8d8d8")) : QColor(QStringLiteral("#cccccc"));
        outerTl = QColor(QStringLiteral("#888888"));
        outerBr = QColor(QStringLiteral("#444444"));
        outerMix = QColor(QStringLiteral("#777777"));
        innerTl = QColor(QStringLiteral("#ffffff"));
        innerBr = QColor(QStringLiteral("#999999"));
        innerMix = QColor(QStringLiteral("#cccccc"));
    }

    painter.fillRect(QRect(x1 + 2, y1 + 2, x2 - x1 - 3, y2 - y1 - 3), fill);

    painter.setPen(outerTl);
    painter.drawLine(x1, y1, x2 - 1, y1);
    painter.drawLine(x1, y1, x1, y2 - 1);
    painter.setPen(outerBr);
    painter.drawLine(x1 + 1, y2, x2, y2);
    painter.drawLine(x2, y1 + 1, x2, y2);
    painter.setPen(outerMix);
    painter.drawPoint(x2, y1);
    painter.drawPoint(x1, y2);

    painter.setPen(innerTl);
    painter.drawLine(x1 + 1, y1 + 1, x2 - 2, y1 + 1);
    painter.drawLine(x1 + 1, y1 + 1, x1 + 1, y2 - 2);
    painter.setPen(innerBr);
    painter.drawLine(x1 + 2, y2 - 1, x2 - 1, y2 - 1);
    painter.drawLine(x2 - 1, y1 + 2, x2 - 1, y2 - 1);
    painter.setPen(innerMix);
    painter.drawPoint(x2 - 1, y1 + 1);
    painter.drawPoint(x1 + 1, y2 - 1);

    painter.restore();
}
