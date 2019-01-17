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

#ifndef AxisLabelUtils_H
#define AxisLabelUtils_H

#include <vector>

#include <QString>
#include <QtCharts/QCategoryAxis>

/* The default Qt Chart labels are not what intuitely makes sense to a user
   (ex, 100.1, 232.8, 365.6, etc), and the QValueAxis::applyNiceNumbers()
   both fails to give good values for energy and it mucks with the axis range,
   so in this header/source I use a huristics based approach to generate which
   labels should be created (based on what I used for InterSpec).
   So far implemented for x-axis, but not yet y-axis
 */

struct TickLabel
{
  enum TickLength { Short, Long };
  
  double value;
  TickLength tickLength;
  QString    label;
  
  TickLabel( double v, TickLength length, const QString &l = QString() );
};//struct TickLabel

/** Currently returns both major labels (e.g., labels that should have text)
    and minor ticks (not text, smaller marks) even though I dont have a good
    way to implement drawing the minor ticks.
 */
std::vector<TickLabel> getXAxisLabelTicks( const QT_CHARTS_NAMESPACE::QCategoryAxis *axis, const double widthpx );

/** */
std::vector<TickLabel> getYAxisLabelTicks( const QT_CHARTS_NAMESPACE::QCategoryAxis *axis, const double heightpx );

#endif
