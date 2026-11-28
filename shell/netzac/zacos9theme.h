#pragma once

#include <QProxyStyle>
#include <QRect>

class QPainter;

class NetzacZacos9Style : public QProxyStyle {
public:
    NetzacZacos9Style();

    void polish(QPalette &palette) override;
    int pixelMetric(
        PixelMetric metric,
        const QStyleOption *option = nullptr,
        const QWidget *widget = nullptr) const override;
    QSize sizeFromContents(
        ContentsType type,
        const QStyleOption *option,
        const QSize &contentsSize,
        const QWidget *widget = nullptr) const override;
    void drawPrimitive(
        PrimitiveElement element,
        const QStyleOption *option,
        QPainter *painter,
        const QWidget *widget = nullptr) const override;
    void drawControl(
        ControlElement element,
        const QStyleOption *option,
        QPainter *painter,
        const QWidget *widget = nullptr) const override;
    QRect subControlRect(
        ComplexControl cc,
        const QStyleOptionComplex *opt,
        SubControl sc,
        const QWidget *widget = nullptr) const override;
    void drawComplexControl(
        ComplexControl cc,
        const QStyleOptionComplex *opt,
        QPainter *painter,
        const QWidget *widget = nullptr) const override;
};

void drawZacos9PopupButton(QPainter &painter, const QRect &rect, bool enabled, bool down);
