/*
 * Copyright (C) Wolthera van Hovell tot Westerflier <griffinvalley@gmail.com>, (C) 2016
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */
#include "kis_visual_color_selector.h"

#include <QColor>
#include <QPixmap>
#include <QPainter>
#include <QPainterPath>
#include <QRect>
#include <QVector>
#include <QVBoxLayout>
#include <QList>
#include <QPolygon>
#include <QRect>

#include "KoColorConversions.h"
#include "KoColorDisplayRendererInterface.h"
#include "KoChannelInfo.h"
#include <QPointer>
#include "kis_signal_compressor.h"

struct KisVisualColorSelector::Private
{
    KoColor currentcolor;
    const KoColorSpace *currentCS;
    QList <KisVisualColorSelectorShape*> widgetlist;
    bool updateSelf = false;
    const KoColorDisplayRendererInterface *displayRenderer = 0;
    //Current coordinates.
    QVector <float> currentCoordinates;
};

KisVisualColorSelector::KisVisualColorSelector(QWidget *parent) : QWidget(parent), m_d(new Private)
{

    QVBoxLayout *layout = new QVBoxLayout;
    this->setLayout(layout);
    //m_d->updateSelf = new KisSignalCompressor(100 /* ms */, KisSignalCompressor::POSTPONE, this);
}

KisVisualColorSelector::~KisVisualColorSelector()
{

}

void KisVisualColorSelector::slotSetColor(KoColor c)
{
    if (m_d->updateSelf==false) {
        m_d->currentcolor = c;
        if (m_d->currentCS != c.colorSpace()) {
            slotsetColorSpace(c.colorSpace());
        }
    }
    updateSelectorElements();
}

void KisVisualColorSelector::slotsetColorSpace(const KoColorSpace *cs)
{
    if (m_d->currentCS != cs)
    {
        m_d->currentCS = cs;
        if (this->layout()) {
            qDeleteAll(this->children());
        }
        m_d->widgetlist.clear();
        QHBoxLayout *layout = new QHBoxLayout;
        //redraw all the widgets.
        if (m_d->currentCS->colorChannelCount() == 1) {
            KisVisualRectangleSelectorShape *bar =  new KisVisualRectangleSelectorShape(this, KisVisualRectangleSelectorShape::onedimensional,KisVisualColorSelectorShape::Channel, cs, 0, 0);
            bar->setMaximumWidth(width()*0.1);
            bar->setMaximumHeight(height());
            connect (bar, SIGNAL(sigNewColor(KoColor)), this, SLOT(updateFromWidgets(KoColor)));
            layout->addWidget(bar);
            m_d->widgetlist.append(bar);
        } else if (m_d->currentCS->colorChannelCount() == 3) {
            KisVisualRectangleSelectorShape *bar =  new KisVisualRectangleSelectorShape(this,
                                                                                        KisVisualRectangleSelectorShape::onedimensional,
                                                                                        KisVisualColorSelectorShape::HSL,
                                                                                        cs, 0, 0,
                                                                                        m_d->displayRenderer, width()*0.05,KisVisualRectangleSelectorShape::borderMirrored);
            KisVisualRectangleSelectorShape *block =  new KisVisualRectangleSelectorShape(this, KisVisualRectangleSelectorShape::twodimensional,
                                                                                          KisVisualColorSelectorShape::HSL,
                                                                                          cs, 1, 2,
                                                                                          m_d->displayRenderer);
            bar->setMaximumWidth(width()*0.5);
            bar->setMaximumHeight(height());
            block->setMaximumWidth(width()*0.5);
            block->setMaximumHeight(height());
            bar->setColor(m_d->currentcolor);
            block->setColor(m_d->currentcolor);
            connect (bar, SIGNAL(sigNewColor(KoColor)), block, SLOT(setColorFromSibling(KoColor)));
            connect (block, SIGNAL(sigNewColor(KoColor)), SLOT(updateFromWidgets(KoColor)));
            layout->addWidget(bar);
            layout->addWidget(block);
            m_d->widgetlist.append(bar);
            m_d->widgetlist.append(block);
        } else if (m_d->currentCS->colorChannelCount() == 4) {
            KisVisualRectangleSelectorShape *block =  new KisVisualRectangleSelectorShape(this, KisVisualRectangleSelectorShape::twodimensional,KisVisualColorSelectorShape::Channel, cs, 0, 1);
            KisVisualRectangleSelectorShape *block2 =  new KisVisualRectangleSelectorShape(this, KisVisualRectangleSelectorShape::twodimensional,KisVisualColorSelectorShape::Channel, cs, 2, 3);
            block->setMaximumWidth(width()*0.5);
            block->setMaximumHeight(height());
            block2->setMaximumWidth(width()*0.5);
            block2->setMaximumHeight(height());
            block->setColor(m_d->currentcolor);
            block2->setColor(m_d->currentcolor);
            connect (block, SIGNAL(sigNewColor(KoColor)), block2, SLOT(setColorFromSibling(KoColor)));
            connect (block2, SIGNAL(sigNewColor(KoColor)), SLOT(updateFromWidgets(KoColor)));
            layout->addWidget(block);
            layout->addWidget(block2);
            m_d->widgetlist.append(block);
            m_d->widgetlist.append(block2);
        }
        this->setLayout(layout);
    }

}


void KisVisualColorSelector::setDisplayRenderer (const KoColorDisplayRendererInterface *displayRenderer) {
    m_d->displayRenderer = displayRenderer;
    if (m_d->widgetlist.size()>0) {
        Q_FOREACH (KisVisualColorSelectorShape *shape, m_d->widgetlist) {
            shape->setDisplayRenderer(displayRenderer);
        }
    }
}

void KisVisualColorSelector::updateSelectorElements()
{
    //first lock all elements from sending updates, then update all elements.
    Q_FOREACH (KisVisualColorSelectorShape *shape, m_d->widgetlist) {
        shape->blockSignals(true);
    }

    Q_FOREACH (KisVisualColorSelectorShape *shape, m_d->widgetlist) {
        if (m_d->updateSelf==false) {
            shape->setColor(m_d->currentcolor);
        } else {
            shape->setColorFromSibling(m_d->currentcolor);
        }
    }
    Q_FOREACH (KisVisualColorSelectorShape *shape, m_d->widgetlist) {
        shape->blockSignals(false);
    }

}

void KisVisualColorSelector::updateFromWidgets(KoColor c)
{
    m_d->currentcolor = c;
    Q_EMIT sigNewColor(c);
    m_d->updateSelf = true;
}

void KisVisualColorSelector::leaveEvent(QEvent *)
{
    m_d->updateSelf = false;
}

/*------------Selector shape------------*/
struct KisVisualColorSelectorShape::Private
{
    QPixmap gradient;
    QPixmap fullSelector;
    bool pixmapsNeedUpdate = true;
    QPointF currentCoordinates;
    Dimensions dimension;
    ColorModel model;
    const KoColorSpace *cs;
    KoColor currentColor;
    int channel1;
    int channel2;
    KisSignalCompressor *updateTimer;
    bool mousePressActive = false;
    const KoColorDisplayRendererInterface *displayRenderer = 0;
    qreal hue = 0.0;
    qreal sat = 0.0;

};

KisVisualColorSelectorShape::KisVisualColorSelectorShape(QWidget *parent,
                                                         KisVisualColorSelectorShape::Dimensions dimension,
                                                         KisVisualColorSelectorShape::ColorModel model,
                                                         const KoColorSpace *cs,
                                                         int channel1,
                                                         int channel2,
                                                         const KoColorDisplayRendererInterface *displayRenderer): QWidget(parent), m_d(new Private)
{
    m_d->dimension = dimension;
    m_d->model = model;
    m_d->cs = cs;
    m_d->currentColor = KoColor();
    m_d->currentColor.setOpacity(1.0);
    m_d->currentColor.convertTo(cs);
    int maxchannel = m_d->cs->colorChannelCount()-1;
    m_d->channel1 = qBound(0, channel1, maxchannel);
    m_d->channel2 = qBound(0, channel2, maxchannel);
    this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_d->updateTimer = new KisSignalCompressor(100 /* ms */, KisSignalCompressor::POSTPONE, this);
    setDisplayRenderer(displayRenderer);

}

KisVisualColorSelectorShape::~KisVisualColorSelectorShape()
{

}

void KisVisualColorSelectorShape::updateCursor()
{
    QPointF point1 = convertKoColorToShapeCoordinate(m_d->currentColor);
    if (point1 != m_d->currentCoordinates) {
        m_d->currentCoordinates = point1;
    }
}

QPointF KisVisualColorSelectorShape::getCursorPosition() {
    return m_d->currentCoordinates;
}

void KisVisualColorSelectorShape::setColor(KoColor c)
{
    if (c.colorSpace() != m_d->cs) {
        c.convertTo(m_d->cs);
    }
    m_d->currentColor = c;
    updateCursor();
    convertShapeCoordinateToKoColor(getCursorPosition(), true);
    m_d->pixmapsNeedUpdate = true;
    update();
}

void KisVisualColorSelectorShape::setColorFromSibling(KoColor c)
{
    if (c.colorSpace() != m_d->cs) {
        c.convertTo(m_d->cs);
    }
    m_d->currentColor = c;
    Q_EMIT sigNewColor(c);
    m_d->pixmapsNeedUpdate = true;
    update();
}

void KisVisualColorSelectorShape::setDisplayRenderer (const KoColorDisplayRendererInterface *displayRenderer)
{
    if (displayRenderer) {
        if (m_d->displayRenderer) {
            m_d->displayRenderer->disconnect(this);
        }
        m_d->displayRenderer = displayRenderer;
    } else {
        m_d->displayRenderer = KoDumbColorDisplayRenderer::instance();
    }
    connect(m_d->displayRenderer, SIGNAL(displayConfigurationChanged()),
            SLOT(updateFromChangedDisplayRenderer()), Qt::UniqueConnection);

}

void KisVisualColorSelectorShape::updateFromChangedDisplayRenderer()
{
    qDebug()<<"update from changed display renderer";
    m_d->pixmapsNeedUpdate = true;
    updateCursor();
    //m_d->currentColor = convertShapeCoordinateToKoColor(getCursorPosition());
    update();
}

QColor KisVisualColorSelectorShape::getColorFromConverter(KoColor c){
    QColor col;
    if (m_d->displayRenderer && m_d->displayRenderer->getPaintingColorSpace()==m_d->cs) {
        col = m_d->displayRenderer->toQColor(c);
    } else {
        col = c.toQColor();
    }
    return col;
}

void KisVisualColorSelectorShape::slotSetActiveChannels(int channel1, int channel2)
{
    int maxchannel = m_d->cs->colorChannelCount()-1;
    m_d->channel1 = qBound(0, channel1, maxchannel);
    m_d->channel2 = qBound(0, channel2, maxchannel);
    m_d->pixmapsNeedUpdate = true;
    update();
}

QPixmap KisVisualColorSelectorShape::getPixmap()
{
    if (m_d->pixmapsNeedUpdate == true) {
        m_d->pixmapsNeedUpdate = false;
        m_d->gradient = QPixmap(width(), height());
        m_d->gradient.fill(Qt::black);
        QImage img(width(), height(), QImage::Format_RGB32);
        img.fill(Qt::black);

        for (int y = 0; y<img.height(); y++) {
            for (int x=0; x<img.width(); x++) {
                QPoint widgetPoint(x,y);
                QPointF newcoordinate = convertWidgetCoordinateToShapeCoordinate(widgetPoint);
                KoColor c = convertShapeCoordinateToKoColor(newcoordinate);
                QColor col = getColorFromConverter(c);
                img.setPixel(widgetPoint, col.rgb());
            }
        }

        m_d->gradient = QPixmap::fromImage(img, Qt::AvoidDither);
    }
    return m_d->gradient;
}

KoColor KisVisualColorSelectorShape::convertShapeCoordinateToKoColor(QPointF coordinates, bool cursor)
{
    KoColor c = m_d->currentColor;
    QVector <float> channelValues (c.colorSpace()->channelCount());
    channelValues.fill(1.0);
    c.colorSpace()->normalisedChannelsValue(c.data(), channelValues);
    QVector <qreal> maxvalue(c.colorSpace()->channelCount());
    maxvalue.fill(1.0);
    if (m_d->displayRenderer
            && m_d->displayRenderer->getPaintingColorSpace()==m_d->cs
            && m_d->cs->colorDepthId().id().contains("f")) {
        for (int ch = 0; ch<maxvalue.size(); ch++) {
            KoChannelInfo *channel = m_d->cs->channels()[ch];
            maxvalue[ch] = m_d->displayRenderer->maxVisibleFloatValue(channel);
            channelValues[ch] = channelValues[ch]/(maxvalue[ch]);
        }
    }
    qreal huedivider = 1.0;
    qreal huedivider2 = 1.0;

    if (m_d->channel1==0) {
        huedivider = 360.0;
    }
    if (m_d->channel2==0) {
        huedivider2 = 360.0;
    }
    if (m_d->model != ColorModel::Channel && c.colorSpace()->colorModelId().id() == "RGBA") {
        if (c.colorSpace()->colorModelId().id() == "RGBA") {
            if (m_d->model == ColorModel::HSV){
                /*
                 * RGBToHSV has a undefined hue possibility. This means that hue will be -1.
                 * This can be annoying for dealing with a selector, but I understand it is being
                 * used for the KoColorSelector... For now implement a qMax here.
                 */
                QVector <float> inbetween(3);
                RGBToHSV(channelValues[2],channelValues[1], channelValues[0], &inbetween[0], &inbetween[1], &inbetween[2]);
                inbetween = convertvectorqrealTofloat(getHSX(convertvectorfloatToqreal(inbetween)));
                inbetween[m_d->channel1] = coordinates.x()*huedivider;
                if (m_d->dimension == Dimensions::twodimensional) {
                    inbetween[m_d->channel2] = coordinates.y()*huedivider2;
                }
                if (cursor==true){setHSX(convertvectorfloatToqreal(inbetween));}
                HSVToRGB(qMax(inbetween[0],(float)0.0), inbetween[1], inbetween[2], &channelValues[2], &channelValues[1], &channelValues[0]);
            } else if (m_d->model == ColorModel::HSL) {
                /*
                 * HSLToRGB can give negative values on the grey. I fixed the fromNormalisedChannel function to clamp,
                 * but you might want to manually clamp for floating point values.
                 */
                QVector <float> inbetween(3);
                RGBToHSL(channelValues[2],channelValues[1], channelValues[0], &inbetween[0], &inbetween[1], &inbetween[2]);
                inbetween = convertvectorqrealTofloat(getHSX(convertvectorfloatToqreal(inbetween)));
                inbetween[m_d->channel1] = coordinates.x()*huedivider;
                if (m_d->dimension == Dimensions::twodimensional) {
                    inbetween[m_d->channel2] = coordinates.y()*huedivider2;
                }
                if (cursor==true){setHSX(convertvectorfloatToqreal(inbetween));}
                HSLToRGB(inbetween[0], inbetween[1], inbetween[2],&channelValues[2],&channelValues[1], &channelValues[0]);
            } else if (m_d->model == ColorModel::HSI) {
                /*
                 * HSI is a modified HSY function.
                 */
                QVector <qreal> chan2 = convertvectorfloatToqreal(channelValues);
                QVector <qreal> inbetween(3);
                RGBToHSI(chan2[2],chan2[1], chan2[0], &inbetween[0], &inbetween[1], &inbetween[2]);
                inbetween = getHSX(inbetween);
                inbetween[m_d->channel1] = coordinates.x();
                if (m_d->dimension == Dimensions::twodimensional) {
                    inbetween[m_d->channel2] = coordinates.y();
                }
                if (cursor==true){setHSX(inbetween);}
                HSIToRGB(inbetween[0], inbetween[1], inbetween[2],&chan2[2],&chan2[1], &chan2[0]);
                channelValues = convertvectorqrealTofloat(chan2);
            } else /*if (m_d->model == ColorModel::HSY)*/ {
                /*
                 * HSY is pretty slow to render due being a pretty over-the-top function.
                 * Might be worth investigating whether HCY can be used instead, but I have had
                 * some weird results with that.
                 */
                QVector <qreal> luma= m_d->cs->lumaCoefficients();
                QVector <qreal> chan2 = convertvectorfloatToqreal(channelValues);
                QVector <qreal> inbetween(3);
                RGBToHSY(chan2[2],chan2[1], chan2[0], &inbetween[0], &inbetween[1], &inbetween[2],
                        luma[0], luma[1], luma[2]);
                inbetween = getHSX(inbetween);
                inbetween[m_d->channel1] = coordinates.x();
                if (m_d->dimension == Dimensions::twodimensional) {
                    inbetween[m_d->channel2] = coordinates.y();
                }
                if (cursor==true){setHSX(inbetween);}
                HSYToRGB(inbetween[0], inbetween[1], inbetween[2],&chan2[2],&chan2[1], &chan2[0],
                        luma[0], luma[1], luma[2]);
                channelValues = convertvectorqrealTofloat(chan2);
            }
        }
    } else {
        channelValues[m_d->channel1] = coordinates.x();
        if (m_d->dimension == Dimensions::twodimensional) {
            channelValues[m_d->channel2] = coordinates.y();
        }
    }
    for (int i=0; i<channelValues.size();i++) {
        channelValues[i] = channelValues[i]*(maxvalue[i]);
    }
    c.colorSpace()->fromNormalisedChannelsValue(c.data(), channelValues);
    return c;
}

QPointF KisVisualColorSelectorShape::convertKoColorToShapeCoordinate(KoColor c)
{
    if (c.colorSpace() != m_d->cs) {
        c.convertTo(m_d->cs);
    }
    QVector <float> channelValues (m_d->currentColor.colorSpace()->channelCount());
    channelValues.fill(1.0);
    m_d->cs->normalisedChannelsValue(c.data(), channelValues);
    QVector <qreal> maxvalue(c.colorSpace()->channelCount());
    maxvalue.fill(1.0);
    if (m_d->displayRenderer
            && m_d->displayRenderer->getPaintingColorSpace()==m_d->cs
            && m_d->cs->colorDepthId().id().contains("f")) {
        for (int ch = 0; ch<maxvalue.size(); ch++) {
            KoChannelInfo *channel = m_d->cs->channels()[ch];
            maxvalue[ch] = m_d->displayRenderer->maxVisibleFloatValue(channel);
            channelValues[ch] = channelValues[ch]/(maxvalue[ch]);
        }
    }
    QPointF coordinates(0.0,0.0);
    qreal huedivider = 1.0;
    qreal huedivider2 = 1.0;
    if (m_d->channel1==0) {
        huedivider = 360.0;
    }
    if (m_d->channel2==0) {
        huedivider2 = 360.0;
    }
    if (m_d->model != ColorModel::Channel && c.colorSpace()->colorModelId().id() == "RGBA") {
        if (c.colorSpace()->colorModelId().id() == "RGBA") {
            if (m_d->model == ColorModel::HSV){
                QVector <float> inbetween(3);
                RGBToHSV(channelValues[2],channelValues[1], channelValues[0], &inbetween[0], &inbetween[1], &inbetween[2]);
                inbetween = convertvectorqrealTofloat(getHSX(convertvectorfloatToqreal(inbetween)));
                coordinates.setX(inbetween[m_d->channel1]/huedivider);
                if (m_d->dimension == Dimensions::twodimensional) {
                    coordinates.setY(inbetween[m_d->channel2]/huedivider2);
                }
            } else if (m_d->model == ColorModel::HSL) {
                QVector <float> inbetween(3);
                RGBToHSL(channelValues[2],channelValues[1], channelValues[0], &inbetween[0], &inbetween[1], &inbetween[2]);
                inbetween = convertvectorqrealTofloat(getHSX(convertvectorfloatToqreal(inbetween)));
                coordinates.setX(inbetween[m_d->channel1]/huedivider);
                if (m_d->dimension == Dimensions::twodimensional) {
                    coordinates.setY(inbetween[m_d->channel2]/huedivider2);
                }
            } else if (m_d->model == ColorModel::HSI) {
                QVector <qreal> chan2 = convertvectorfloatToqreal(channelValues);
                QVector <qreal> inbetween(3);
                RGBToHSI(chan2[2],chan2[1], chan2[0], &inbetween[0], &inbetween[1], &inbetween[2]);
                inbetween = getHSX(inbetween);
                coordinates.setX(inbetween[m_d->channel1]);
                if (m_d->dimension == Dimensions::twodimensional) {
                    coordinates.setY(inbetween[m_d->channel2]);
                }
            } else /*if (m_d->model == ColorModel::HSY)*/ {
                QVector <qreal> luma = m_d->cs->lumaCoefficients();
                QVector <qreal> chan2 = convertvectorfloatToqreal(channelValues);
                QVector <qreal> inbetween(3);
                RGBToHSY(chan2[2],chan2[1], chan2[0], &inbetween[0], &inbetween[1], &inbetween[2], luma[0], luma[1], luma[2]);
                inbetween = getHSX(inbetween);
                coordinates.setX(inbetween[m_d->channel1]);
                if (m_d->dimension == Dimensions::twodimensional) {
                    coordinates.setY(inbetween[m_d->channel2]);
                }
            }
        }
    } else {
        coordinates.setX(channelValues[m_d->channel1]);
        if (m_d->dimension == Dimensions::twodimensional) {
            coordinates.setY(channelValues[m_d->channel2]);
        }
    }
    return coordinates;
}

QVector<float> KisVisualColorSelectorShape::convertvectorqrealTofloat(QVector<qreal> real)
{
    QVector <float> vloat(real.size());
    for (int i=0; i<real.size(); i++) {
        vloat[i] = real[i];
    }
    return vloat;
}

QVector<qreal> KisVisualColorSelectorShape::convertvectorfloatToqreal(QVector <float> vloat)
{
    QVector <qreal> real(vloat.size());
    for (int i=0; i<vloat.size(); i++) {
        real[i] = vloat[i];
    }
    return real;
}

void KisVisualColorSelectorShape::mousePressEvent(QMouseEvent *e)
{
    m_d->mousePressActive = true;
}

void KisVisualColorSelectorShape::mouseMoveEvent(QMouseEvent *e)
{
    if (m_d->mousePressActive==true && this->mask().contains(e->pos())) {
        QPointF coordinates = convertWidgetCoordinateToShapeCoordinate(e->pos());
        KoColor col = convertShapeCoordinateToKoColor(coordinates);
        setColor(col);
        if (!m_d->updateTimer->isActive()) {
            Q_EMIT sigNewColor(col);
            m_d->updateTimer->start();
        }
    } else {
        e->ignore();
    }
}

void KisVisualColorSelectorShape::mouseReleaseEvent(QMouseEvent *)
{
    m_d->mousePressActive = false;
}
void KisVisualColorSelectorShape::paintEvent(QPaintEvent*)
{
    QPainter painter(this);

    //check if old and new colors differ.

    if (m_d->pixmapsNeedUpdate) {
        setMask(getMaskMap());
    }
    drawCursor();
    painter.drawPixmap(0,0,m_d->fullSelector);
}

void KisVisualColorSelectorShape::resizeEvent(QResizeEvent *)
{
    m_d->pixmapsNeedUpdate = true;
}

KisVisualColorSelectorShape::Dimensions KisVisualColorSelectorShape::getDimensions()
{
    return m_d->dimension;
}

KisVisualColorSelectorShape::ColorModel KisVisualColorSelectorShape::getColorModel()
{
    return m_d->model;
}

void KisVisualColorSelectorShape::setFullImage(QPixmap full)
{
    m_d->fullSelector = full;
}
KoColor KisVisualColorSelectorShape::getCurrentColor()
{
    return m_d->currentColor;
}

QVector <qreal> KisVisualColorSelectorShape::getHSX(QVector<qreal> hsx)
{
    if ((hsx[2]<=0.0 || hsx[2]>=1.0) && m_d->channel1!=2 && m_d->channel2!=2) {
        hsx[1] = m_d->sat;
        hsx[0]=m_d->hue;
    } else if (hsx[1]<=0.0  && m_d->channel1!=1 && m_d->channel2!=1){
        hsx[0]=m_d->hue;
    }
    return hsx;
}

void KisVisualColorSelectorShape::setHSX(QVector<qreal> hsx)
{
    if (hsx[2]>0.0 && hsx[2<1.0]){
        m_d->sat = hsx[1];
    }
    if (hsx[1]>0.0) {
        m_d->hue = hsx[0];
    }
}

/*-----------Rectangle Shape------------*/

KisVisualRectangleSelectorShape::KisVisualRectangleSelectorShape(QWidget *parent,
                                                                 Dimensions dimension,
                                                                 ColorModel model,
                                                                 const KoColorSpace *cs,
                                                                 int channel1, int channel2,
                                                                 const KoColorDisplayRendererInterface *displayRenderer,
                                                                 int width,
                                                                 singelDTypes d)
    : KisVisualColorSelectorShape(parent, dimension, model, cs, channel1, channel2, displayRenderer)
{
    m_type = d;
    m_barWidth = width;
}

KisVisualRectangleSelectorShape::~KisVisualRectangleSelectorShape()
{

}

void KisVisualRectangleSelectorShape::setBarWidth(int width)
{
    m_barWidth = width;
}

QPointF KisVisualRectangleSelectorShape::convertShapeCoordinateToWidgetCoordinate(QPointF coordinate)
{
    qreal x = width()/2;
    qreal y = height()/2;
    KisVisualColorSelectorShape::Dimensions dimension = getDimensions();
    if (dimension == KisVisualColorSelectorShape::onedimensional) {
        if ( m_type == KisVisualRectangleSelectorShape::vertical) {
            y = coordinate.x()*height();
        } else if (m_type == KisVisualRectangleSelectorShape::horizontal) {
            x = coordinate.x()*width();
        } else if (m_type == KisVisualRectangleSelectorShape::border) {

            QRectF innerRect(m_barWidth/2, m_barWidth/2, width()-m_barWidth, height()-m_barWidth);
            QPointF left (innerRect.left(),innerRect.center().y());
            QList <QLineF> polygonLines;
            polygonLines.append(QLineF(left, innerRect.topLeft()));
            polygonLines.append(QLineF(innerRect.topLeft(), innerRect.topRight()));
            polygonLines.append(QLineF(innerRect.topRight(), innerRect.bottomRight()));
            polygonLines.append(QLineF(innerRect.bottomRight(), innerRect.bottomLeft()));
            polygonLines.append(QLineF(innerRect.bottomLeft(), left));

            qreal totalLength =0.0;
            Q_FOREACH(QLineF line, polygonLines) {
                totalLength += line.length();
            }

            qreal length = coordinate.x()*totalLength;
            QPointF intersect(x,y);
            Q_FOREACH(QLineF line, polygonLines) {
                if (line.length()>length && length>0){
                    intersect = line.pointAt(length/line.length());

                }
                length-=line.length();
            }
            x = qRound(intersect.x());
            y = qRound(intersect.y());

        } else /*if (m_type == KisVisualRectangleSelectorShape::borderMirrored)*/  {

            QRectF innerRect(m_barWidth/2, m_barWidth/2, width()-m_barWidth, height()-m_barWidth);
            QPointF bottom (innerRect.center().x(), innerRect.bottom());
            QList <QLineF> polygonLines;
            polygonLines.append(QLineF(bottom, innerRect.bottomLeft()));
            polygonLines.append(QLineF(innerRect.bottomLeft(), innerRect.topLeft()));
            polygonLines.append(QLineF(innerRect.topLeft(), innerRect.topRight()));
            polygonLines.append(QLineF(innerRect.topRight(), innerRect.bottomRight()));
            polygonLines.append(QLineF(innerRect.bottomRight(), bottom));

            qreal totalLength =0.0;
            Q_FOREACH(QLineF line, polygonLines) {
                totalLength += line.length();
            }

            qreal length = coordinate.x()*(totalLength/2);
            QPointF intersect(x,y);
            if (coordinate.y()==1) {
                for (int i = polygonLines.size()-1; i==0; i--) {
                    QLineF line = polygonLines.at(i);
                    if (line.length()>length && length>0){
                        intersect = line.pointAt(length/line.length());

                    }
                    length-=line.length();
                }
            } else {
                Q_FOREACH(QLineF line, polygonLines) {
                    if (line.length()>length && length>0){
                        intersect = line.pointAt(length/line.length());

                    }
                    length-=line.length();
                }
            }
            x = qRound(intersect.x());
            y = qRound(intersect.y());

        }
    } else {
        x = coordinate.x()*width();
        y = coordinate.y()*height();
    }
    return QPointF(x,y);
}

QPointF KisVisualRectangleSelectorShape::convertWidgetCoordinateToShapeCoordinate(QPoint coordinate)
{
    //default implementation:
    qreal x = 0.5;
    qreal y = 0.5;
    KisVisualColorSelectorShape::Dimensions dimension = getDimensions();
    if (getMaskMap().contains(coordinate)) {
        if (dimension == KisVisualColorSelectorShape::onedimensional ) {
            if (m_type == KisVisualRectangleSelectorShape::vertical) {
                x = (qreal)coordinate.y()/(qreal)height();
            } else if (m_type == KisVisualRectangleSelectorShape::horizontal) {
                x = (qreal)coordinate.x()/(qreal)width();
            } else if (m_type == KisVisualRectangleSelectorShape::border) {
                //border

                QRectF innerRect(m_barWidth, m_barWidth, width()-(m_barWidth*2), height()-(m_barWidth*2));
                QPointF left (innerRect.left(),innerRect.center().y());
                QList <QLineF> polygonLines;
                polygonLines.append(QLineF(left, innerRect.topLeft()));
                polygonLines.append(QLineF(innerRect.topLeft(), innerRect.topRight()));
                polygonLines.append(QLineF(innerRect.topRight(), innerRect.bottomRight()));
                polygonLines.append(QLineF(innerRect.bottomRight(), innerRect.bottomLeft()));
                polygonLines.append(QLineF(innerRect.bottomLeft(), left));

                QLineF radius(coordinate, this->geometry().center());
                QPointF intersect(0.5,0.5);
                qreal length = 0.0;
                qreal totalLength = 0.0;
                bool foundIntersect = false;
                Q_FOREACH(QLineF line, polygonLines) {
                    if (line.intersect(radius,&intersect)==QLineF::BoundedIntersection && foundIntersect==false)
                    {
                        foundIntersect = true;
                        length+=QLineF(line.p1(), intersect).length();

                    }
                    if (foundIntersect==false) {
                        length+=line.length();
                    }
                    totalLength+=line.length();
                }

                x = length/totalLength;

            } else /*if (m_type == KisVisualRectangleSelectorShape::borderMirrored)*/  {
                //border

                QRectF innerRect(m_barWidth, m_barWidth, width()-(m_barWidth*2), height()-(m_barWidth*2));
                QPointF bottom (innerRect.center().x(), innerRect.bottom());
                QList <QLineF> polygonLines;
                polygonLines.append(QLineF(bottom, innerRect.bottomLeft()));
                polygonLines.append(QLineF(innerRect.bottomLeft(), innerRect.topLeft()));
                polygonLines.append(QLineF(innerRect.topLeft(), innerRect.topRight()));
                polygonLines.append(QLineF(innerRect.topRight(), innerRect.bottomRight()));
                polygonLines.append(QLineF(innerRect.bottomRight(), bottom));

                QLineF radius(coordinate, this->geometry().center());
                QPointF intersect(0.5,0.5);
                qreal length = 0.0;
                qreal totalLength = 0.0;
                bool foundIntersect = false;
                Q_FOREACH(QLineF line, polygonLines) {
                    if (line.intersect(radius,&intersect)==QLineF::BoundedIntersection && foundIntersect==false)
                    {
                        foundIntersect = true;
                        length+=QLineF(line.p1(), intersect).length();

                    }
                    if (foundIntersect==false) {
                        length+=line.length();
                    }
                    totalLength+=line.length();
                }
                int halflength = totalLength/2;

                if (length>halflength) {
                    x = (halflength - (length-halflength))/halflength;
                    y = 1.0;
                } else {
                    x = length/halflength;
                    y = 0.0;
                }
            }
        }
        else {
            x = (qreal)coordinate.x()/(qreal)width();
            y = (qreal)coordinate.y()/(qreal)height();
        }
    }
    return QPointF(x, y);
}

QRegion KisVisualRectangleSelectorShape::getMaskMap()
{
    QRegion mask = QRegion(0,0,width(),height());
    if (m_type==KisVisualRectangleSelectorShape::border || m_type==KisVisualRectangleSelectorShape::borderMirrored) {
        mask = mask.subtracted(QRegion(m_barWidth, m_barWidth, width()-(m_barWidth*2), height()-(m_barWidth*2)));
    }
    return mask;
}

void KisVisualRectangleSelectorShape::drawCursor()
{
    QPointF cursorPoint = convertShapeCoordinateToWidgetCoordinate(getCursorPosition());
    QPixmap fullSelector = getPixmap();
    QColor col = getColorFromConverter(getCurrentColor());
    QPainter painter;
    painter.begin(&fullSelector);
    painter.setRenderHint(QPainter::Antialiasing);
    //QPainterPath path;
    QBrush fill;
    fill.setStyle(Qt::SolidPattern);

    int cursorwidth = 5;
    QRect rect(cursorPoint.toPoint().x()-cursorwidth,cursorPoint.toPoint().y()-cursorwidth,
               cursorwidth*2,cursorwidth*2);
    if (m_type==KisVisualRectangleSelectorShape::vertical){
        int x = ( cursorPoint.x()-(width()/2)+1 );
        int y = ( cursorPoint.y()-cursorwidth );
        rect.setCoords(x, y, x+width()-2, y+(cursorwidth*2));
    } else {
        int x = cursorPoint.x()-cursorwidth;
        int y = cursorPoint.y()-(height()/2)+1;
        rect.setCoords(x, y, x+(cursorwidth*2), y+cursorwidth-2);
    }
    QRectF innerRect(m_barWidth, m_barWidth, width()-(m_barWidth*2), height()-(m_barWidth*2));
    if (getDimensions() == KisVisualColorSelectorShape::onedimensional && m_type!=KisVisualRectangleSelectorShape::border && m_type!=KisVisualRectangleSelectorShape::borderMirrored) {
        painter.setPen(Qt::white);
        fill.setColor(Qt::white);
        painter.setBrush(fill);
        painter.drawRect(rect);
        //set filter conversion!
        fill.setColor(col);
        painter.setPen(Qt::black);
        painter.setBrush(fill);
        rect.setCoords(rect.topLeft().x()+1, rect.topLeft().y()+1,
                       rect.topLeft().x()+rect.width()-2, rect.topLeft().y()+rect.height()-2);
        painter.drawRect(rect);

    }else if(m_type==KisVisualRectangleSelectorShape::borderMirrored){
        painter.setPen(Qt::white);
        fill.setColor(Qt::white);
        painter.setBrush(fill);
        painter.drawEllipse(cursorPoint, cursorwidth, cursorwidth);
        QPoint mirror(innerRect.center().x()+(innerRect.center().x()-cursorPoint.x()),cursorPoint.y());
        painter.drawEllipse(mirror, cursorwidth, cursorwidth);
        fill.setColor(col);
        painter.setPen(Qt::black);
        painter.setBrush(fill);
        painter.drawEllipse(cursorPoint, cursorwidth-1, cursorwidth-1);
        painter.drawEllipse(mirror, cursorwidth-1, cursorwidth-1);

    } else {
        painter.setPen(Qt::white);
        fill.setColor(Qt::white);
        painter.setBrush(fill);
        painter.drawEllipse(cursorPoint, cursorwidth, cursorwidth);
        fill.setColor(col);
        painter.setPen(Qt::black);
        painter.setBrush(fill);
        painter.drawEllipse(cursorPoint, cursorwidth-1.0, cursorwidth-1.0);
    }
    painter.end();
    setFullImage(fullSelector);
}