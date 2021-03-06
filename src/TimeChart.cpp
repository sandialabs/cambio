/**
 Cambio: a simple program to convert or manipulate gamma spectrum data files.
 Copyright (C) 2015 William Johnson
 
 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.
 
 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.
 
 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */


#include <iostream>

#include <QtCharts/QChart>
#include <QGesture>

#include "cambio/TimeChart.h"

TimeChart::TimeChart( QGraphicsItem *parent, Qt::WindowFlags wFlags )
  : QChart( QChart::ChartTypeCartesian, parent, wFlags )
{
// Seems that QGraphicsView (SpectrumView) does not grab gestures.
// They can only be grabbed here in the QGraphicsWidget (QChart).
  //enabling the bellow grabGestures cause the app to crash on android when you touch the chart
  //grabGesture( Qt::PanGesture );
  //grabGesture( Qt::PinchGesture );
}//TimeChart constructor


TimeChart::~TimeChart()
{
}


bool TimeChart::sceneEvent(QEvent *event)
{
  if( event->type() == QEvent::Gesture )
    return gestureEvent(static_cast<QGestureEvent *>(event));
  return QChart::event(event);
}

bool TimeChart::gestureEvent(QGestureEvent *event)
{
  if( QGesture *gesture = event->gesture(Qt::PanGesture))
  {
    QPanGesture *pan = static_cast<QPanGesture *>(gesture);
    QChart::scroll(-(pan->delta().x()), pan->delta().y());
  }

  if( QGesture *gesture = event->gesture(Qt::PinchGesture) )
  {
    QPinchGesture *pinch = static_cast<QPinchGesture *>(gesture);
    if (pinch->changeFlags() & QPinchGesture::ScaleFactorChanged)
        QChart::zoom(pinch->scaleFactor());
  }

  return true;
}//bool gestureEvent(QGestureEvent *event)
