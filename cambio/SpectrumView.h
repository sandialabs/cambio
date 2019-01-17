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


#ifndef CHARTVIEW_H
#define CHARTVIEW_H

#include <memory>

#include <QtCharts/QChart>

#include <QGraphicsView>


class QDropEvent;
class ChartViewImp;
class QGestureEvent;
class QGraphicsScene;
class QChartViewImp;
class SpectrumChart;

QT_CHARTS_BEGIN_NAMESPACE
  class QCategoryAxis;
  class QLineSeries;
  class QAbstractAxis;
QT_CHARTS_END_NAMESPACE

class Measurement;
class ChartViewImp;
class BackgroundImageImp;

//REPLACE_SERIES_ON_UPDATE is set to true then a new QLineSeries will be allocated with
//  the new points.  Note that I originally implemented allocating the new
//  series to avoid the overhead of appending the list of points (and
//  consequently re-drawing) when adding each new point - but I have since
//  found QXYSeries::replace(QList<QPointF> points), which also avoid this
//  overhead - so I'm just leaving in the old implementation until I test
//  the QXYSeries::replace method a bit more
#define REPLACE_SERIES_ON_UPDATE 0


//Blarg, apparently updating the chart is incredibly slow, so as an experiment
//  will try ignoring all pending Qt::GestureUpdated events when we proccess
//  one of these events.  It does seem to help.  But probably the real bottleneck
//  is in changing the x-axis range.  Perhaps xRangeChanged() should be optimized
//  so the x-series only gets just the displayed data
#define KEEP_TOUCH_EVENTS_FROM_BUILDING_UP 1



class SpectrumView : public QGraphicsView
{
  Q_OBJECT
  
public:
  explicit SpectrumView( QWidget *parent = nullptr );
  ~SpectrumView();

  bool isLogY() const;
  void setLogY( const bool log );
  
  virtual void paintEvent( QPaintEvent *event );
  
  void setSpectrum( std::shared_ptr<const Measurement> spec,
                    const bool changeXRange,
                    const char *title = nullptr );

  
public slots:
  void xRangeChanged( qreal xmin, qreal xmax );
  void yRangeChanged( qreal xmin, qreal xmax );

signals:
  void fileDropped( QDropEvent * );
  void mouseLeft();
  void mousePositionChanged( int channelstart, int channelend, double energy,
                             double countheight, double channelcounts );
  
protected:
  void calcAndSetYRange();
  
  //updateSeries(): Loads spectrum to m_series using the current m_rebinFactor
  //  and m_spectrum.  Energy and Counts ranges are not updated, as neither is
  //  m_rebinFactor.
  //If m_spectrum is invalid, then m_series is cleared
  void updateSeries();
  
  //calcRebinFactor(): returns the current rebin factor you should be using
  //  for displaying the data such that each bin will be at least as wide as
  //  the series marker, assuming the current xaxis range.
  size_t calcRebinFactor() const;
  size_t calcRebinFactor( const double xmin, const double xmax ) const;
  
  //setRebinFactor(): if factor is same as m_rebinFactor, nothing is done and
  //  the function returns immediately.  Else, m_rebinFactor is updated,
  //  the m_series is updated, as well as the y range, and the y axis text.
  void setRebinFactor( const size_t factor );
  
  virtual bool viewportEvent( QEvent *event );
  virtual void keyPressEvent( QKeyEvent *event );
  virtual void resizeEvent( QResizeEvent *event );
  virtual void mouseDoubleClickEvent( QMouseEvent *event );
  virtual void mousePressEvent( QMouseEvent *event );
  virtual void mouseMoveEvent( QMouseEvent *event );
  virtual void mouseReleaseEvent( QMouseEvent *event );
  virtual void leaveEvent ( QEvent * event );

  virtual bool event( QEvent *event );

  //For some reason QMainWindow doesnt catch file drag-n-drop events on OS X,
  //  so we'll implement this functionality here.
  virtual void dragEnterEvent(QDragEnterEvent *);
  virtual void dragMoveEvent(QDragMoveEvent *);
  virtual void dragLeaveEvent(QDragLeaveEvent *);
  virtual void dropEvent(QDropEvent *);

protected:
  //handlePan(): pans the scene so that the passed in scenePos() will coorespond
  //  to the same energy as the touches initial touch down energy
  void handlePan( const QPointF &pxpos );

  //handlePinch(): scales the x-axis according to the scale factor relative
  //  to when the two-finger touch initially started.  The currentx is
  //  the center of the current two touches, in scene coordinates.
  void handlePinch( const double totalScaleFactor, const double currentx );

  //gestureEvent(): only handles tap gestures right now; zooming and
  //  panning handled in touchEvent(). Called from event(QEvent*).
  bool gestureEvent( QGestureEvent *event );

  //touchEvent(): handles one finger pan events, and two finger zoom/pinch
  //  events.  Called from event(QEvent*).
  bool touchEvent( QTouchEvent *event );

  
private:
  size_t m_rebinFactor;
  int m_numTouches;
  bool m_isTouching;  //any touch event is active
  bool m_isSettingXRange;

  qreal m_dataXMin, m_dataXMax;
  
  QT_CHARTS_NAMESPACE::QCategoryAxis *m_xaxis;
  QT_CHARTS_NAMESPACE::QAbstractAxis *m_yaxis;
  QT_CHARTS_NAMESPACE::QLineSeries *m_series;
  
  QScopedPointer<ChartViewImp> m_imp;
  QScopedPointer<BackgroundImageImp> m_back;
  
  Q_DISABLE_COPY(SpectrumView)
  
  std::shared_ptr<const Measurement> m_spectrum;
  
#if( KEEP_TOUCH_EVENTS_FROM_BUILDING_UP )
  bool m_updatingGesture;
#endif
  
  friend class ChartViewImp;
};//class SpectrumView


#endif
