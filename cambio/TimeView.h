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


#ifndef TimeView_H
#define TimeView_H

#include <set>
#include <memory>
#include <vector>
#include <utility>

#include <QtCharts/QChart>
#include <QGraphicsView>

#include <boost/shared_ptr.hpp>

class QDropEvent;
class TimeViewImp;
class QGraphicsScene;
class QGraphicsRectItem;

QT_CHARTS_BEGIN_NAMESPACE
  class QValueAxis;
  class QLineSeries;
  class QCategoryAxis;
  class QAbstractAxis;
QT_CHARTS_END_NAMESPACE

namespace SpecUtils{ class SpecFile; }
namespace SpecUtils{ class Measurement; }

class TimeView : public QGraphicsView
{
  Q_OBJECT
  
public:
  explicit TimeView( QWidget *parent = nullptr );
  ~TimeView();
  
  void setGrossCounts( std::shared_ptr<const SpecUtils::Measurement> grossCounts );
  
public slots:
  void xRangeChanged( qreal xmin, qreal xmax );
  void yRangeChanged( qreal xmin, qreal xmax );
  
  void spectrumSampleNumbersChanged( std::shared_ptr<SpecUtils::SpecFile> meas,
                                     std::set<int> samplenums,
                                     std::vector<bool> detectors );

signals:
  void fileDropped( QDropEvent * );
  void mouseLeft();
  
  //mousePositionChanged(): emmitted when either no button, or the left mouse
  //  button is used.
  //  'channel' cooresponds to what consequitive number time sample the mouse
  //    is over
  //  'time' is how many seconds from start of measurment the mouse cooresponds
  //    to
  //  'countheight' is the equivalent gamma count height the mouse is at
  //  'channelgammacounts' is the number of gamma counts in the sample the mouse
  //    is over
  //  'channelneutroncounts' is the number of neutron counts in the sample the
  //    mouse is over
  void mousePositionChanged( int channel, double time,
                             double countheight, double channelgammacounts,
                             double channelneutroncounts );
  
  //timeRangeSelected: emitted when the mouse is actually released.  If it is
  //  the right mouse button, the Qt::ControlModifier will be added to the
  //  mods.
  //  Note that 'firstsample' and 'lastsample' will range between 0 and
  //  m_grossCounts->channel_energies()->size()-1,
  //  and DO NOT coorespond to m_grossCounts->sample_numbers()!
  void timeRangeSelected( int firstsample, int lastsample,
                          Qt::KeyboardModifiers mods );
  
protected:
  void updateSeries();
  void calcAndSetYRange();
  void drawHighlightedRegions();
  
  bool viewportEvent( QEvent *event );
  void keyPressEvent( QKeyEvent *event );
  void resizeEvent( QResizeEvent *event );
  void mousePressEvent( QMouseEvent *event );
  void mouseMoveEvent( QMouseEvent *event );
  void mouseReleaseEvent( QMouseEvent *event );
  void leaveEvent ( QEvent * event );
  
  //For some reason QMainWindow doesnt catch file drag-n-drop events on OS X,
  //  so we'll implement this functionality here.
  virtual void dragEnterEvent(QDragEnterEvent *);
  virtual void dragMoveEvent(QDragMoveEvent *);
  virtual void dragLeaveEvent(QDragLeaveEvent *);
  virtual void dropEvent(QDropEvent *);

  
private:
  bool m_isTouching, m_isSettingXRange;
  qreal m_dataGammaXMin, m_dataGammaXMax, m_dataNeutronXMin, m_dataNeutronXMax;
  
  QT_CHARTS_NAMESPACE::QCategoryAxis *m_xaxis;
  QT_CHARTS_NAMESPACE::QCategoryAxis *m_gammaAxis;
  QT_CHARTS_NAMESPACE::QCategoryAxis *m_neutronAxis;
  QT_CHARTS_NAMESPACE::QLineSeries *m_gammaSeries;
  QT_CHARTS_NAMESPACE::QLineSeries *m_neutronSeries;
  
  QScopedPointer<TimeViewImp> m_imp;
  
  
  std::vector<QGraphicsRectItem *> m_highlights;
  std::vector< std::pair<float,float> > m_highlightedRanges;
  
  size_t m_rebinFactor;
  std::shared_ptr<const SpecUtils::Measurement> m_grossCounts;
  
  Q_DISABLE_COPY(TimeView)
  
  friend class TimeViewImp;
};//class TimeView


#endif
