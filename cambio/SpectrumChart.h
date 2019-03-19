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

#ifndef SpectrumChart_H
#define SpectrumChart_H

#include <QtCharts/QChart>

class QGestureEvent;

QT_CHARTS_USE_NAMESPACE


class SpectrumChart : public QChart
{
  Q_OBJECT
  
public:
  explicit SpectrumChart( QGraphicsItem *parent = nullptr,
                          Qt::WindowFlags wFlags = nullptr );
  ~SpectrumChart();

protected:
  bool sceneEvent(QEvent *event);
  
private:
  bool gestureEvent(QGestureEvent *event);

private:

};//class SpectrumChart

#endif // CHART_H
