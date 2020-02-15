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

#include <cmath>
#include <limits>
#include <iostream>

#include <QtCharts/QChart>
#include <QBrush>
#include <QColor>
#include <QDebug>
#include <qmath.h>
#include <QPointF>
#include <QMargins>
#include <QGesture>
#include <QMimeData>
#include <QFileInfo>
#include <QDropEvent>
#include <QtCharts/QValueAxis>
#include <QtCharts/QLineSeries>
#include <QtCharts/QCategoryAxis>
#if( !defined(ANDROID) )
#include <QRubberBand>
#endif
#include <QMouseEvent>
#include <QtCharts/QLogValueAxis>
#include <QDragLeaveEvent>
#include <QDragEnterEvent>
#include <QCoreApplication>
#include <QGestureRecognizer>
#include <QGraphicsProxyWidget>

#define BOOST_DATE_TIME_NO_LIB

#include "SpecUtils/SpecFile.h"
#include "cambio/sandia_logo.h"
#include "cambio/SpectrumView.h"
#include "cambio/SpectrumChart.h"
#include "cambio/AxisLabelUtils.h"

using namespace std;
QT_CHARTS_USE_NAMESPACE


class ChartViewImp
{
public:
  ChartViewImp( SpectrumView *q, QChart *chart = 0 );
  ~ChartViewImp();
  
  void setChart( QChart *chart );

  void resize();
  
  SpectrumView *q_ptr;
  QGraphicsScene *m_scene;
  QChart *m_chart;
  QPoint m_mouseDownPos;
  qreal m_mouseDownXMin, m_mouseDownXMax;
  QPointF m_mouseDownCoordVal;
#if( !defined(ANDROID) && !defined(IOS) )
  QRubberBand *m_rubberBand;
#endif
  Qt::MouseButton m_downMouseButton;

#if( defined(ANDROID) || defined(IOS) )
  double m_xSeriesMin;
  double m_xSeriesMax;
#endif
};//class ChartViewImp



class BackgroundImageImp
{
public:
  BackgroundImageImp( QGraphicsScene *scene, QChart *chart );
  
  ~BackgroundImageImp();
  
  double m_width, m_hight;
  
protected:
  vector<QGraphicsItem *> m_items;
};//class BackgroundImageImp


/*
class MyPanGesture : public QGesture
{

};

class PanRecognizer : public QGestureRecognizer
{
//see http://www.slideshare.net/qtbynokia/using-multitouch-and-gestures-with-qt
public:
    //virtual QGesture *create( QObject *target );
    virtual Result recognize( QGesture *gesture, QObject *watched, QEvent *event )
    {
    }
    virtual void reset( QGesture *gesture );
};//class PanRecognizer
*/

SpectrumView::SpectrumView( QWidget *parent )
  : QGraphicsView( parent ),
    m_rebinFactor( 1 ),
    m_isTouching( false ),
    m_isSettingXRange( false ),
    m_xaxis( nullptr ),
    m_yaxis( nullptr ),
    m_series( nullptr )
#if( KEEP_TOUCH_EVENTS_FROM_BUILDING_UP )
    , m_updatingGesture( false )
#endif
{
  //The Qt::PinchGesture seems to be unsatisfactory for our use; it isnt a
  //  single finger drag like you would expect, but some three finger thing or
  //  something, therefore we will just raw handle both pan and zoom/pinch
  //  events in touchEvent() function (called from SpectrumView::event()), since
  //  it wasnt that hard to handle pinch events in this function as well.
  //  The alternative to this would be to implementing a custom PanRecognizer
  //  class, and then use gestures for all functions

  grabGesture( Qt::TapGesture );
//  grabGesture( Qt::PanGesture );
//  grabGesture( Qt::PinchGesture );
//  grabGesture( Qt::TapAndHoldGesture );
  setAttribute( Qt::WA_AcceptTouchEvents );

  
  SpectrumChart *chart = new SpectrumChart();
  chart->setContentsMargins( 0, 0, 0, 0 );
  
  chart->setMargins( QMargins( 10, 3, 5, 3 ) );
  
  m_imp.reset( new ChartViewImp( this, chart ) );
  
  m_series = new QLineSeries();
  
  m_dataXMin = 0.0;
  m_dataXMax = 3000.0;
  
  QCategoryAxis *xaxis = new QCategoryAxis;
  xaxis->setLabelsPosition( QCategoryAxis::AxisLabelsPosition::AxisLabelsPositionOnValue );
  
  QLogValueAxis *yaxis = new QLogValueAxis;
  m_xaxis = xaxis;
  m_yaxis = yaxis;
  
  xaxis->setTitleText("Energy (keV)");
  //xaxis->setTickCount(10);
  //xaxis->setLabelFormat("%.1f");
  
  yaxis->setLabelFormat("%g");
  yaxis->setTitleText("Counts/Channel");
  yaxis->setBase( 10 );
  
  m_xaxis->setGridLineVisible( false );
  m_yaxis->setGridLineVisible( false );
  
  QObject::connect( m_xaxis, SIGNAL(rangeChanged(qreal,qreal)),
                   this, SLOT(xRangeChanged(qreal,qreal)));
  
  QObject::connect( m_yaxis, SIGNAL(rangeChanged(qreal,qreal)),
                   this, SLOT(yRangeChanged(qreal,qreal)));
  
  chart->addAxis( m_xaxis, Qt::AlignBottom );
  chart->addAxis( m_yaxis, Qt::AlignLeft );
  
  chart->addSeries( m_series );
  
  m_series->attachAxis( m_xaxis );
  m_series->attachAxis( m_yaxis );
  
  chart->setTitle("");
//  chart->setAnimationOptions( QChart::SeriesAnimations );
  chart->legend()->hide();
  
  setAcceptDrops( true );
}//SpectrumView constructor


SpectrumView::~SpectrumView()
{
  
}


bool SpectrumView::event( QEvent *event )
{
  if( event->type() == QEvent::Gesture )
    return gestureEvent( static_cast<QGestureEvent *>(event) );

  //To catch one finger drag on the screen could try the bellow in combination
  //  with enabling setAttribute( Qt::WA_AcceptTouchEvents );
  switch( event->type() )
  {
    case QEvent::TouchBegin:   //should only get here for first finger down
    case QEvent::TouchUpdate:  //Whenever touch is changed/added/removed
    case QEvent::TouchEnd:     //gets here only when last touch ends
    {
      QTouchEvent *te = static_cast<QTouchEvent *>( event );
      return touchEvent( te );
    }

    default:
      break;
  }//switch( event->type() )

  return QGraphicsView::event( event );
}//bool event( QEvent *event );


void SpectrumView::paintEvent( QPaintEvent *event )
{
  QGraphicsView::paintEvent( event );


//  qDebug() << "paintEvent: " << s;
  
  QGraphicsScene *s = scene();  //m_imp->m_scene
  
  if( s )
  {
    if( !m_spectrum )
    {
      //Load the background image if there is no spectrum.
      //If the background is already rendered, only re-render it if the size
      //  has changed (without this size change change, rendering the background
      //  will cause another paintEvent(...) that continues the proccess).
      const QSizeF csize = m_imp->m_chart->size();
      if( !m_back
          || (fabs(m_back->m_width - csize.width()) > 1.0)
          || (fabs(m_back->m_hight - csize.height()) > 1.0) )
      {
        m_back.reset( new BackgroundImageImp( s, m_imp->m_chart ) );
      }
    }else if( !!m_spectrum && !!m_back )
    {
      m_back.reset();
    }
  }//if( s )
}//void paintEvent( QPaintEvent *event )


void SpectrumView::updateSeries()
{
  if( !m_spectrum || !m_spectrum->gamma_counts()
     || m_spectrum->gamma_counts()->size() < 3 || !m_spectrum->channel_energies() )
  {
    m_series->clear();
    return;
  }
  
  const double xAxisMin = m_xaxis->min();
  const double xAxisMax = m_xaxis->max();

#if( REPLACE_SERIES_ON_UPDATE )
  //calling m_series->clear() and m_series->append(...) reprededely
  //  is _super_ slow for spectra with 16k channels (each append causes a redraw
  //  it kind of looks like, and I'm not to sure why m_series->clear() is
  //  unbelieveably slow), so instead we'll add all the new points to a series
  //  that isnt currently drawn, then swap it out for the previous series.
  QLineSeries *newseries = new QLineSeries();
#endif

  const vector<float> &x = *m_spectrum->channel_energies();
  const vector<float> &y = *m_spectrum->gamma_counts();
  
  //if m_yaxis->type() == QAbstractAxis::AxisTypeLogValue, then the chart
  //  wont display if any of the bins are zero or less
  const double minyval = DBL_EPSILON;
  
#if( defined(ANDROID) || defined(IOS) )
  //This optimization for Android could probably be applied to dektops
  //  well (just needs to be tested); it appears to make Android much
  //  smoother for zooming and panning.
  const double xrange = xAxisMax-xAxisMin;
  vector<float>::const_iterator lb, ub;
  lb = std::lower_bound( x.begin(), x.end(), xAxisMin - 0.15*xrange );
  ub = std::upper_bound( x.begin(), x.end(), xAxisMax + 0.15*xrange );
  const size_t startchannel = lb - x.begin();
  const size_t nchannel = ub - x.begin();
  m_imp->m_xSeriesMin = x[startchannel];
  m_imp->m_xSeriesMax = x[nchannel-1];
#else
  const size_t startchannel = 0;
  const size_t nchannel = y.size();
#endif

  const size_t nsumchannel = nchannel - m_rebinFactor - 1;

#if( !REPLACE_SERIES_ON_UPDATE )
  QList<QPointF> points;
  points.reserve(  2u*(nsumchannel - startchannel + 2u) );
#endif

  for( size_t i = startchannel; i < nsumchannel; i += m_rebinFactor )
  {
    double yval = 0.0;
    for( size_t j = 0; j < m_rebinFactor; ++j )
      yval += static_cast<double>( y[i+j] );
    
    yval = std::max( yval, minyval );
#if( REPLACE_SERIES_ON_UPDATE )
    newseries->append( x[i], yval );
    newseries->append( x[i+m_rebinFactor], yval );
#else
    points.push_back( QPointF( static_cast<qreal>(x[i]), static_cast<qreal>(yval)) );
    points.push_back( QPointF( static_cast<qreal>(x[i+m_rebinFactor]), static_cast<qreal>(yval)) );
#endif
  }//for( size_t i = 0; i < nsumchannel; i += m_rebinFactor )
  
  //XXX- possible double counting here
  double lastyval = 0.0;
  const double xmax = 2.0*static_cast<double>(x[nchannel-1]) - static_cast<double>(x[nchannel-2]);
  
  for( size_t i = nsumchannel; i < nchannel; ++i )
    lastyval += static_cast<double>( y[i] );
  lastyval = std::max( lastyval, minyval );
  
#if( REPLACE_SERIES_ON_UPDATE )
  newseries->append( x[nsumchannel], lastyval );
  newseries->append( xmax, lastyval );

  newseries->setBrush( m_series->brush() );
  newseries->setPen( m_series->pen() );

  m_imp->m_chart->addSeries( newseries );
  newseries->attachAxis( m_xaxis );
  newseries->attachAxis( m_yaxis );

  m_series->detachAxis( m_xaxis );
  m_series->detachAxis( m_yaxis );
  m_imp->m_chart->removeSeries( m_series );
  delete m_series;
  m_series = newseries;
#else
  points.push_back( QPointF( static_cast<qreal>(x[nsumchannel]), static_cast<qreal>(lastyval)) );
  points.push_back( QPointF(xmax, lastyval) );
  m_series->replace( points );
#endif

  m_xaxis->setRange( xAxisMin, xAxisMax );
}//void updateSeries()


size_t SpectrumView::calcRebinFactor() const
{
  const double lowerx = m_xaxis->min();
  const double upperx = m_xaxis->max();
  
  return calcRebinFactor( lowerx, upperx );  
}//size_t calcRebinFactor() const


size_t SpectrumView::calcRebinFactor( const double lowerx,
                                      const double upperx ) const
{
  if( !m_spectrum || !m_spectrum->gamma_counts()
      || m_spectrum->gamma_counts()->size() < 3 )
    return 1;
  
  const vector<float> &x = *m_spectrum->channel_energies();
//  const double markerw = m_series->pen().widthF();

  const double chartw = m_imp->m_chart->plotArea().width();
  
  const size_t startbin = lower_bound( x.begin(), x.end(), lowerx ) - x.begin();
  const size_t endbin = lower_bound( x.begin(), x.end(), upperx ) - x.begin();
  
  const size_t nbin = endbin - startbin;
  
#if( defined(ANDROID) || defined(IOS) )
  const double maxnbin = chartw / 2;
#else
  const double maxnbin = chartw;// / markerw;
#endif

  double rebin = std::ceil( nbin / maxnbin );
  
  rebin = std::max( rebin, 1.0 );
  rebin = std::min( rebin, 128.0 );
  
  return static_cast<size_t>( rebin );
}//size_t calcRebinFactor() const


void SpectrumView::setRebinFactor( const size_t factor )
{
  if( m_rebinFactor == factor )
    return;
    
  m_rebinFactor = factor;
  m_rebinFactor = std::max( m_rebinFactor, size_t(1) );
  
  updateSeries();
  calcAndSetYRange();
  
  char buffer[128];
  
  if( m_rebinFactor == 1 )
  {
    m_yaxis->setTitleText( "Counts/Channel" );
  }else
  {
    snprintf( buffer, sizeof(buffer),
              "Counts Per %i Channels", int(m_rebinFactor) );
    m_yaxis->setTitleText( buffer );
  }//
}//void setRebinFactor( const size_t factor )


void SpectrumView::setSpectrum( std::shared_ptr<const Measurement> spec,
                                const bool changeXRange,
                                const char *title )
{
  m_spectrum = spec;
  
  if( !m_spectrum || !m_spectrum->channel_energies()
      || m_spectrum->channel_energies()->empty() )
  {
    m_dataXMin = 0.0;
    m_dataXMax = 3000.0;
  }else
  {
    const vector<float> &x = *m_spectrum->channel_energies();
    m_dataXMin = static_cast<double>( x[0] );
    m_dataXMax = 2*static_cast<double>(x[x.size()-1]) - static_cast<double>(x[x.size()-2]);
  }//if( !!m_spectrum )
  
  if( title )
  {
    QFileInfo info( title );
    
    if( info.isFile() )
      m_imp->m_chart->setTitle( info.fileName().toUtf8().data() );
    else
      m_imp->m_chart->setTitle( title );
  }
  
  if( changeXRange )
  {
    m_xaxis->setRange( m_dataXMin, m_dataXMax );
    const size_t factor = calcRebinFactor();
    if( m_rebinFactor != factor )
    {
      setRebinFactor( factor );
    }else
    {
      updateSeries();
      calcAndSetYRange();
    }
  }else
  {
    const size_t factor = calcRebinFactor();
    if( factor != m_rebinFactor )
    {
      setRebinFactor( factor );
    }else
    {
      updateSeries();
      calcAndSetYRange();
    }//if( factor != m_rebinFactor ) / else
  }//if( changeXRange ) / else
}//void setSpectrum( std::shared_ptr<Measurement> dummy )


void SpectrumView::dragEnterEvent( QDragEnterEvent *event )
{
  const QMimeData *mimeData = event->mimeData();
  if( mimeData && mimeData->hasUrls() )
    event->acceptProposedAction();
}//void dragEnterEvent( QDragEnterEvent *event )


void SpectrumView::dragMoveEvent( QDragMoveEvent *event )
{
  event->accept();
}//void dragMoveEvent( QDragMoveEvent *event )


void SpectrumView::dragLeaveEvent( QDragLeaveEvent *event )
{
  event->accept();
}


void SpectrumView::dropEvent( QDropEvent *event )
{
  emit fileDropped(event);
}//void dropEvent( QDropEvent *event )


void SpectrumView::calcAndSetYRange()
{
  //Go through here and adjust the y range.
  double ymin = numeric_limits<double>::infinity();
  double ymax = -numeric_limits<double>::infinity();
  
  const double xmin = m_xaxis->min();
  const double xmax = m_xaxis->max();
  
  const int npoints = m_series->count();
  for( int i = 0; i < npoints; ++i )
  {
    const QPointF &p = m_series->at(i);
    if( p.x() >= xmin && p.x() <= xmax )
    {
      ymin = std::min( ymin, p.y() );
      ymax = std::max( ymax, p.y() );
    }//
  }//for( int i = 0; i < npoints; ++i )
  
  if( !isinf(ymin) || !isinf(ymax) )
  {
    if( isLogY() )
    {
      ymin = std::max( ymin/10.0, 0.1 );
      ymax = std::max( 2.0*ymax, 0.1 );
    }else
    {
      const double yrange = (ymax-ymin);
      ymin = std::max( 0.0, ymin - 0.1*yrange );
      ymax = ymax + 0.1*yrange;
    }
    
    m_yaxis->setRange( ymin, ymax );
    
    if( !isLogY() )
    {
      auto axis = dynamic_cast<QCategoryAxis *>(m_yaxis);
      if( axis )  //hsould always be true, but JIC
      {
        const double plotHeightPx = m_imp->m_chart->plotArea().height();
        
        for( const auto &label : axis->categoriesLabels() )
          axis->remove( label );
        
        for( const auto &tick : getYAxisLabelTicks(axis, plotHeightPx) )
        {
          if( tick.tickLength == TickLabel::Long )
            axis->append( tick.label, tick.value );
        }//
        axis->setStartValue( axis->min() );
      }//if( axis )
    }//if( !isLogY() )
    
  }//if( !isinf(ymin) || !isinf(ymax) )

}//void calcAndSetYRange()


void SpectrumView::xRangeChanged( qreal xmin, qreal xmax )
{
  if( m_isSettingXRange )
    return;
  
  //Could probably introduce a optimization here that if xmin and xmax
  //  are the same as last time the function was called, then nothing
  //  will be done.  However, a quick inspection shows while zooming
  //  on Android, we ony make it to this point once per paintEvent().

  m_isSettingXRange = true;
  
  const double newdrange = xmax - xmin;
  if( newdrange > (m_dataXMax-m_dataXMin) )
  {
    m_xaxis->setRange( m_dataXMin, m_dataXMax );
  }else if( xmax > m_dataXMax )
  {
    m_xaxis->setRange( m_dataXMax - newdrange, m_dataXMax );
  }else if( xmin < m_dataXMin )
  {
    m_xaxis->setRange( m_dataXMin, m_dataXMin + newdrange );
  }else
  {
//    m_xaxis->setRange( xmin, xmax );
  }//if( xrange needs adjusting ) / else
  
  const size_t newRebinFactor = calcRebinFactor( xmin, xmax );
  if( newRebinFactor != m_rebinFactor )
  {
    setRebinFactor( newRebinFactor );
  }else
  {
#if( defined(ANDROID) || defined(IOS) )
    if( m_imp->m_xSeriesMin > xmin || m_imp->m_xSeriesMax < xmax )
      updateSeries();
#endif
    calcAndSetYRange();
  }  
  
  for( const auto &label : m_xaxis->categoriesLabels() )
    m_xaxis->remove( label );
  
  const auto ticks = getXAxisLabelTicks( m_xaxis, m_imp->m_chart->plotArea().width() );
  
  for( const auto &tick : ticks )
  {
    if( tick.tickLength == TickLabel::Long )
      m_xaxis->append( tick.label, tick.value );
    //else
      //m_xaxis->append( "", tick.u );
  }//
  
  m_xaxis->setStartValue( m_xaxis->min() );
  
  m_isSettingXRange = false;
}//void xRangeChanged( qreal xmin, qreal xmax )


void SpectrumView::yRangeChanged( qreal /*ymin*/, qreal /*ymax*/ )
{
  
}//void yRangeChanged( qreal xmin, qreal xmax )


bool SpectrumView::isLogY() const
{
  return (m_yaxis->type() == QAbstractAxis::AxisTypeLogValue);
}


void SpectrumView::setLogY( const bool log )
{
  if( log == isLogY() )
    return;

  const double xmin = m_xaxis->min();
  const double xmax = m_xaxis->max();
  
  QAbstractAxis *newaxis = nullptr;
  
  if( log )
  {
    QLogValueAxis *yaxis = new QLogValueAxis;
    newaxis = yaxis;
    yaxis->setLabelFormat("%.2g");
    yaxis->setBase( 10 );
  }else
  {
    QCategoryAxis *yaxis = new QCategoryAxis;
    yaxis->setLabelsPosition( QCategoryAxis::AxisLabelsPosition::AxisLabelsPositionOnValue );
    newaxis = yaxis;
    //yaxis->setLabelFormat("%.2g");
    //yaxis->setTickCount( 10 );
  }
  
  newaxis->setGridLineVisible( false );
  
  newaxis->setTitleText( "Counts/Channel" );
  
  if( m_yaxis )
  {
    m_series->detachAxis( m_yaxis );
    m_imp->m_chart->removeAxis( m_yaxis );
    delete m_yaxis;
  }//if( m_yaxis )
  
  m_yaxis = newaxis;
  m_imp->m_chart->addAxis( newaxis, Qt::AlignLeft );
  m_series->attachAxis( newaxis );
  
  m_xaxis->setRange( xmin, xmax );
  
#if( REPLACE_SERIES_ON_UPDATE )
  //This call to updateSeries() isnt seemingly strickly necassary, however it
  //  prevents an occational crash - I'm guessing related to a QLineSeries
  //  object not loving the swapping of axises to many times.
  updateSeries();
#endif
  
  QObject::connect( m_yaxis, SIGNAL(rangeChanged(qreal,qreal)),
                   this, SLOT(yRangeChanged(qreal,qreal)));
  calcAndSetYRange();
}//void setLogY( const bool log )


bool SpectrumView::viewportEvent( QEvent *event )
{
  if( event->type() == QEvent::TouchBegin )
  {
#if defined(IOS) || !defined(__APPLE__)
    m_isTouching = true;
    m_imp->m_chart->setAnimationOptions(QChart::NoAnimation);
#endif
  }else if( event->type() == QEvent::TouchEnd )
  {
    m_isTouching = false;
  }
  
  return QGraphicsView::viewportEvent(event);
}//bool viewportEvent( QEvent *event )


void SpectrumView::keyPressEvent( QKeyEvent *event )
{
  switch( event->key() )
  {
    case Qt::Key_Plus:
      m_imp->m_chart->zoomIn();
      break;
    case Qt::Key_Minus:
      m_imp->m_chart->zoomOut();
      break;
    case Qt::Key_Left:
    {
      const double xmin = m_xaxis->min();
      const double xmax = m_xaxis->max();
      
      qreal delta = -0.1*(xmax-xmin);
      if( (xmin+delta) < m_dataXMin )
        delta = m_dataXMin - xmin;
//      m_imp->m_chart->scroll( delta, 0 );
      m_xaxis->setRange( xmin+delta, xmax+delta );
      break;
    }
      
    case Qt::Key_Right:
    {
      const double xmin = m_xaxis->min();
      const double xmax = m_xaxis->max();
      
      qreal delta = 0.1*(xmax-xmin);
      if( (xmax+delta) > m_dataXMax )
        delta = m_dataXMax - xmax;
      m_xaxis->setRange( xmin+delta, xmax+delta );
      break;
    }
      
      case Qt::Key_1:
        setLogY( !isLogY() );
      break;
      
//    case Qt::Key_Up:
//      m_imp->m_chart->scroll(0, 10);
//      break;
//    case Qt::Key_Down:
//      m_imp->m_chart->scroll(0, -10);
//      break;
    default:
      QGraphicsView::keyPressEvent(event);
      break;
  }//switch( event->key() )
}//void keyPressEvent( QKeyEvent *event )


void SpectrumView::mouseDoubleClickEvent( QMouseEvent *event )
{
  if( !m_spectrum )
      return;

  const QPoint pos = event->pos();
  const QRectF plotArea = m_imp->m_chart->plotArea();
  
  if( !plotArea.contains(pos) )
  {
    QGraphicsView::mouseDoubleClickEvent( event );
    return;
  }

  const double lx = m_xaxis->min();
  const double ux = m_xaxis->max();
  const double origdx = ux - lx;
  if( origdx  < 2.0 )
      return;

  const QPointF coorval = m_imp->m_chart->mapToValue( pos, m_series );

  double xmin = coorval.x() - 0.25*origdx;
  double xmax = coorval.x() + 0.25*origdx;

  double deltax = 0.0;
  if( xmin < m_dataXMin )
    deltax = m_dataXMin - xmin;
  else if( xmax > m_dataXMax )
    deltax = m_dataXMax - xmax;

  m_xaxis->setRange( xmin + deltax, xmax + deltax );
}//void SpectrumView::mouseDoubleClickEvent( QMouseEvent *event )


void SpectrumView::mousePressEvent( QMouseEvent *event )
{
  //The TouchEvent is emmitted before the mouse MouseEvent for macOS touchpads,
  //  so m_isTouching should be true here - however we now know this is actually
  //  a click (like with a mouse), so we need to divert the logic to mouse logic
  //
  //  TODO (20180813) - This has not been tested on touch screen devices.
  m_isTouching = false;

  if( m_imp->m_downMouseButton == Qt::NoButton )
    m_imp->m_downMouseButton = event->button();

  m_imp->m_chart->setAnimationOptions(QChart::NoAnimation);
  
  const QPoint pos = event->pos();
  const QPointF coorval = m_imp->m_chart->mapToValue( pos, m_series );
  
  QRectF plotArea = m_imp->m_chart->plotArea();
  
  if( plotArea.contains(pos) )
  {
    m_imp->m_mouseDownPos = pos;
    m_imp->m_mouseDownCoordVal = coorval;
    m_imp->m_mouseDownXMin = m_xaxis->min();
    m_imp->m_mouseDownXMax = m_xaxis->max();
    
    if( event->button() == Qt::LeftButton )
    {
#if( !defined(ANDROID) && !defined(IOS) )
      m_imp->m_rubberBand->setGeometry( QRect(pos, QSize()) );
//      m_imp->m_rubberBand->show();
#endif
      event->accept();
    }else if( event->button() == Qt::RightButton )
    {
      //
    }
  }else
  {
    qDebug() << "Outside mouse area";
    QGraphicsView::mousePressEvent( event );
  }
}//void mousePressEvent(QMouseEvent *event)


void SpectrumView::mouseMoveEvent( QMouseEvent *event )
{
  if( m_isTouching )
    return;

  const QPoint pos = event->pos();
  const QRect rect = m_imp->m_chart->plotArea().toRect();
  const QPointF coorval = m_imp->m_chart->mapToValue( pos, m_series );
  
  if( event->buttons() == Qt::NoButton )
  {
#if( !defined(ANDROID) && !defined(IOS) )
    m_imp->m_rubberBand->hide();
#endif
  }else if( event->buttons() == Qt::RightButton )
  {
    const QPointF &coordval = m_imp->m_mouseDownCoordVal;
    const QPointF newpos = m_imp->m_chart->mapToPosition( coordval, m_series );
    
    m_imp->m_chart->scroll( newpos.x() - pos.x(), 0.0 );
  }else if( event->buttons() == Qt::LeftButton )
  {
    const int width = pos.x() - m_imp->m_mouseDownPos.x();
    if( width > 0 )
    {
#if( !defined(ANDROID) && !defined(IOS) )
      m_imp->m_rubberBand->show();

      const QRect rubberrect( m_imp->m_mouseDownPos.x(),
                              rect.top(), width, rect.height() );

      m_imp->m_rubberBand->setGeometry( rubberrect.normalized() );
#endif
    }else
    {
#if( !defined(ANDROID) && !defined(IOS) )
      m_imp->m_rubberBand->hide();
#endif

      double frac = 4.0*(m_imp->m_mouseDownPos.x()-pos.x())
                    / double(m_imp->m_mouseDownPos.x());
      frac = std::min( frac, 1.0 );
      
      const double origdx = m_imp->m_mouseDownXMax - m_imp->m_mouseDownXMin;
      const double maxdx = m_dataXMax - m_dataXMin;
      const double newdx = origdx + frac*(maxdx - origdx);
      
      const double deltadx = newdx - origdx;
      m_xaxis->setRange( m_imp->m_mouseDownXMin - 0.5*deltadx,
                         m_imp->m_mouseDownXMax + 0.5*deltadx );
    }
  }else
  {
    QGraphicsView::mouseMoveEvent( event );
  }

  int channelstart = -1, channelend = -1;
  double channelcounts = 0.0;
  
  if( !!m_spectrum && !!m_spectrum->channel_energies()
      && m_spectrum->channel_energies()->size() > 3 )
  {
    const vector<float> &x = *m_spectrum->channel_energies();
    const float xval = static_cast<float>( coorval.x() );
    const size_t pos = std::upper_bound( x.begin(), x.end(), xval )
                       - x.begin() - 1;
    channelstart = m_rebinFactor*(pos/m_rebinFactor);
    
    channelend = channelstart + m_rebinFactor - 1;
    
    const vector<float> &y = *m_spectrum->gamma_counts();
	const int nchannel = static_cast<int>( y.size() );

    for( int i = channelstart; i <= channelend && i < nchannel; ++i )
      channelcounts += static_cast<double>( y[i] );
  }//if( !!m_spectrum )
  
  emit mousePositionChanged( channelstart, channelend,
                             coorval.x(), coorval.y(), channelcounts );
}//void mouseMoveEvent(QMouseEvent *event)


void SpectrumView::mouseReleaseEvent( QMouseEvent *event )
{
  m_isTouching = false;
  const QPoint pos = event->pos();
  
//  m_imp->m_chart->setAnimationOptions(QChart::SeriesAnimations);

  if( m_imp->m_downMouseButton == event->button() )
  {
    if( event->button() == Qt::LeftButton )
    {
#if( !defined(ANDROID) && !defined(IOS) )
      m_imp->m_rubberBand->hide();
#endif

      const int width = pos.x() - m_imp->m_mouseDownPos.x();
      
      if( width > 0 )
      {
        const QPointF &startval = m_imp->m_mouseDownCoordVal;
        const QPointF endval = m_imp->m_chart->mapToValue( pos, m_series );
        
        const double xmin = startval.x();
        const double xmax = endval.x();
        m_xaxis->setRange( xmin, xmax );
//        const size_t newRebinFactor = calcRebinFactor( xmin, xmax );
//        setRebinFactor( newRebinFactor );
      }else
      {
        const double xmin = m_xaxis->min();
        const double xmax = m_xaxis->max();
        const size_t newRebinFactor = calcRebinFactor( xmin, xmax );
        setRebinFactor( newRebinFactor );
      }//if( width > 0 ) / else
    }//if( event->button() == Qt::LeftButton )
   
    m_imp->m_downMouseButton = Qt::NoButton;
  }else if( event->button() == Qt::RightButton )
  {
//If vert. or horiz. rubberband mode, restrict zoom out to specified axis.
//Since there is no suitable API for that, use zoomIn with rect bigger than the
//  plot area.
//    QRectF rect = m_imp->m_chart->plotArea();
//    qreal adjustment = rect.width() / 2;
//    rect.adjust(-adjustment, 0, adjustment, 0);
//    m_imp->m_chart->zoomIn(rect);
  }
//  else
//  {
//    m_imp->m_chart->zoomOut();
//  }
  
  event->accept();
}//void mouseReleaseEvent( QMouseEvent *event )

void SpectrumView::leaveEvent ( QEvent * )
{
  emit mouseLeft();
}


void SpectrumView::handlePan( const QPointF &pxpos )
{
   const double origdx = m_imp->m_mouseDownXMax - m_imp->m_mouseDownXMin;
   const QPointF leftpos = m_imp->m_chart->mapToPosition( QPointF(m_xaxis->min(), 1.0) );
   const QPointF rightpos = m_imp->m_chart->mapToPosition( QPointF(m_xaxis->max(), 1.0) );

   const double frac = (pxpos.x() - leftpos.x()) / (rightpos.x() - leftpos.x());
   const double xmin = m_imp->m_mouseDownCoordVal.x() - frac*origdx;
   const double xmax = m_imp->m_mouseDownCoordVal.x() + (1.0-frac)*origdx;

   double deltax = 0.0;
   if( xmin < m_dataXMin )
     deltax = m_dataXMin - xmin;
   else if( xmax > m_dataXMax )
     deltax = m_dataXMax - xmax;

   m_xaxis->setRange( xmin + deltax, xmax + deltax );
}//void handlePan( const QPointF &pxpos )


void SpectrumView::handlePinch( const double sf, const double currentx )
{
    const QPointF leftpos = m_imp->m_chart->mapToPosition( QPointF(m_xaxis->min(), 1.0) );
    const QPointF rightpos = m_imp->m_chart->mapToPosition( QPointF(m_xaxis->max(), 1.0) );

    const double frac = 1.0 / (sf <= 0.00001 ? 1.0 : sf);
    const double origdx = m_imp->m_mouseDownXMax - m_imp->m_mouseDownXMin;
    const double newdx = std::max( std::min( frac * origdx, m_dataXMax - m_dataXMin ), 1.0 );

    const double pxfrac = (currentx - leftpos.x()) / (rightpos.x() - leftpos.x());
    const double xmin = m_imp->m_mouseDownCoordVal.x() - pxfrac*newdx;
    const double xmax = m_imp->m_mouseDownCoordVal.x() + (1.0-pxfrac)*newdx;

    double deltax = 0.0;
    if( xmin < m_dataXMin )
      deltax = m_dataXMin - xmin;
    else if( xmax > m_dataXMax )
      deltax = m_dataXMax - xmax;

    m_xaxis->setRange( xmin + deltax, xmax + deltax );
}//void handlePinch( const double totalScaleFactor, const double currentx );


bool SpectrumView::touchEvent( QTouchEvent *te )
{
#if defined(__APPLE__) && !defined(IOS)
  te->accept();
  return true;
#endif

  const QList<QTouchEvent::TouchPoint> &points = te->touchPoints();

  if( points.size() != 1 && points.size() != 2 )
    return false;
  
  //TODO: make this function handle pinch events as well, so we dont need to use
  //      gestures for zoomin in/out

  const QPointF pospx = points[0].scenePos();

  switch( te->type() )
  {
    case QEvent::TouchBegin:
    {
      //record location of touch, maybye set an is panning flag/variable,
      //  and accept event
      m_isTouching = true;
      m_numTouches = points.size();
      m_imp->m_mouseDownXMin = m_xaxis->min();
      m_imp->m_mouseDownXMax = m_xaxis->max();

      if( points.size() == 1 )
      {
        m_imp->m_mouseDownPos.setX( static_cast<int>(floor(pospx.x()+0.5)) );
        m_imp->m_mouseDownPos.setY( static_cast<int>(floor(pospx.y()+0.5)) );
      }else
      {
        const QPointF pospx1 = points[1].scenePos();
        m_imp->m_mouseDownPos.setX( static_cast<int>(floor( 0.5*(pospx.x()+pospx1.x())+0.5)) );
        m_imp->m_mouseDownPos.setY( static_cast<int>(floor(0.5*(pospx.y()+pospx1.y())+0.5)) );
      }

      m_imp->m_mouseDownCoordVal = m_imp->m_chart->mapToValue( pospx, m_series );

      break;
    }//case QEvent::TouchBegin:

    case QEvent::TouchUpdate:
    {
#if( KEEP_TOUCH_EVENTS_FROM_BUILDING_UP )
      if( m_updatingGesture )
        return true;
      m_updatingGesture = true;
#endif

      //We could also use points[0].startScenePos(), as well as the current x range
      //  to calculate the effective initial energy of the touch start, so we can
      //  figure out how much to pan.  This would be for the case the user does a
      //  pinch to zoom in/out, and then they leave a finger (not necassarily the
      //  first down) to then pan.  Note that this is irrelevant while still using
      //  gestures to handle pinches.
      if( points.size() == 1 )
      {
        if( m_numTouches != points.size() && points.size()==1 )
        {
          //transistioning from a pinch scale, to a single finger pan
          m_numTouches = points.size();
          m_imp->m_mouseDownXMin = m_xaxis->min();
          m_imp->m_mouseDownXMax = m_xaxis->max();
          m_imp->m_mouseDownPos.setX( static_cast<int>( floor(0.5+pospx.x() ) ) );
          m_imp->m_mouseDownPos.setY( static_cast<int>( floor(0.5+pospx.y() ) ) );

          m_imp->m_mouseDownCoordVal = m_imp->m_chart->mapToValue( pospx, m_series );
        }//if( m_numTouches != points.size() && points.size()==1 )

        handlePan( pospx );
      }else
      {
        const QPointF pospx1 = points[1].scenePos();
        const QPointF startpx0 = points[0].startScenePos();
        const QPointF startpx1 = points[1].startScenePos();

        if( m_numTouches != points.size() && points.size()==2 )
        {
          //transistioning from a single finger pan, to a two finger pinch
          m_numTouches = points.size();
          m_imp->m_mouseDownPos.setX( static_cast<int>(floor( 0.5*(startpx0.x()+startpx1.x())+0.5)) );
          m_imp->m_mouseDownPos.setY( static_cast<int>(floor(0.5*(startpx0.y()+startpx1.y())+0.5)) );
          m_imp->m_mouseDownCoordVal = m_imp->m_chart->mapToValue( m_imp->m_mouseDownPos, m_series );
        }//if( m_numTouches != points.size() && points.size()==2 )

        const double totalScaleFactor = fabs(pospx1.x() - pospx.x()) / fabs(startpx0.x() - startpx1.x());
        const double currentx = 0.5*(pospx.x() + pospx1.x());
        handlePinch( totalScaleFactor, currentx );
      }

#if( KEEP_TOUCH_EVENTS_FROM_BUILDING_UP )
      QCoreApplication::processEvents();
      QCoreApplication::flush();
      m_updatingGesture = false;
#endif

      break;
    }//case QEvent::TouchUpdate:

    case QEvent::TouchEnd:
    {
#if( KEEP_TOUCH_EVENTS_FROM_BUILDING_UP )
      QCoreApplication::processEvents();
      QCoreApplication::flush();
      m_updatingGesture = false;
#endif
      if( points.size() == 1 && m_numTouches==1 )
      {
        handlePan( pospx );
      }else if( points.size() == 2 )
      {
        const QPointF pospx1 = points[1].scenePos();
        const QPointF startpx0 = points[0].startScenePos();
        const QPointF startpx1 = points[1].startScenePos();

        const double totalScaleFactor = fabs(pospx1.x() - pospx.x()) / fabs(startpx0.x() - startpx1.x());
        const double currentx = 0.5*(pospx.x() + pospx1.x());
        handlePinch( totalScaleFactor, currentx );
      }

      m_numTouches = 0;

      m_isTouching = false;
      break;
    }//case QEvent::TouchEnd:

    default:
      qDebug() << "Unrecognized touch event";
      return false;
  }//switch( event->type() )

  te->accept();

  //qDebug() << "Got a touch event";
  
  return true;
}//bool touchEvent( QTouchEvent *te )


bool SpectrumView::gestureEvent( QGestureEvent *event )
{
  if( event->activeGestures().size() > 3 )
    return false;

  if( QGesture *gesture = event->gesture(Qt::TapGesture) )
  {
    event->setAccepted( true );
    QTapGesture *tap = static_cast<QTapGesture *>(gesture);
    switch( gesture->state() )
    {
      case Qt::NoGesture:
      break;

      case Qt::GestureStarted:
        m_imp->m_mouseDownXMin = m_xaxis->min();
        m_imp->m_mouseDownXMax = m_xaxis->max();
        m_imp->m_mouseDownCoordVal = m_imp->m_chart->mapToValue( tap->position(), m_series );
      break;

      case Qt::GestureUpdated:
      {
//        const QPointF coord = m_imp->m_chart->mapToValue( tap->position(), m_series );
//        const QPointF newpos = m_imp->m_chart->mapToPosition( m_imp->m_mouseDownCoordVal, m_series );
//        m_imp->m_chart->scroll( newpos.x() - coord.x(), 0.0 );
        break;
      }//case Qt::GestureUpdated:

      case Qt::GestureFinished:
      {
        const bool istouch = m_isTouching;
        m_isTouching = false;
        QMouseEvent me( QEvent::None, tap->position(),
                         Qt::NoButton, Qt::NoButton, Qt::NoModifier );
        mouseMoveEvent( &me );
        m_isTouching = istouch;
        break;
      }//case Qt::GestureFinished:

      case Qt::GestureCanceled:
//        m_xaxis->setRange( m_imp->m_mouseDownXMin, m_imp->m_mouseDownXMax );
      break;
    }//switch( gesture->state() )

    return true;
  }//if( tap gesture )
  
  return false;
}//void gestureEvent( QMouseEvent *event )

void SpectrumView::resizeEvent( QResizeEvent *event )
{
  QGraphicsView::resizeEvent(event);
  m_imp->resize();
  xRangeChanged( m_xaxis->min(), m_xaxis->max() );
}

ChartViewImp::ChartViewImp( SpectrumView *q, QChart *chart )
: q_ptr(q),
  m_scene( new QGraphicsScene(q) ),
  m_chart(chart),
#if( !defined(ANDROID) && !defined(IOS) )
  m_rubberBand( nullptr ),
#endif
  m_downMouseButton( Qt::NoButton )
#if( defined(ANDROID) || defined(IOS) )
  , m_xSeriesMin( 0.0 ),
  m_xSeriesMax( 0.0 )
#endif
{
  q_ptr->setFrameShape(QFrame::NoFrame);
  q_ptr->setBackgroundRole(QPalette::Window);
  q_ptr->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  q_ptr->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  q_ptr->setScene(m_scene);
  q_ptr->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

  if (!m_chart)
    m_chart = new QChart();
  m_scene->addItem(m_chart);

#if( !defined(ANDROID) && !defined(IOS) )
  m_rubberBand =  new QRubberBand( QRubberBand::Rectangle, q );
#endif
}


ChartViewImp::~ChartViewImp()
{
}


void ChartViewImp::setChart( QChart *chart )
{
  Q_ASSERT(chart);
  
  if( m_chart == chart )
    return;

  if( m_chart )
    m_scene->removeItem( m_chart );
  
  m_chart = chart;
  m_scene->addItem( m_chart );

  resize();
}//void setChart( QChart *chart )


void ChartViewImp::resize()
{
  // Fit the chart into view if it has been rotated
  qreal sinA = qAbs(q_ptr->transform().m21());
  qreal cosA = qAbs(q_ptr->transform().m11());
  QSize chartSize = q_ptr->size();
  
  if( sinA == 1.0 )
  {
    chartSize.setHeight(q_ptr->size().width());
    chartSize.setWidth(q_ptr->size().height());
  } else if( sinA != 0.0 )
  {
    //Non-90 deg rotation, find largest square chart that can fit into the view.
    const qreal minDimension = qMin(q_ptr->size().width(), q_ptr->size().height());
    const qreal h = (minDimension - (minDimension / ((sinA / cosA) + 1.0))) / sinA;
    chartSize.setHeight( static_cast<int>(h) );
    chartSize.setWidth( static_cast<int>(h) );
  }//if( at a 90 degree rotation ) / else
  
  m_chart->resize(chartSize);
  
  q_ptr->setMinimumSize(m_chart->minimumSize().toSize());
  q_ptr->setSceneRect(m_chart->geometry());
  
  const size_t factor = q_ptr->calcRebinFactor();
  q_ptr->setRebinFactor( factor );
}//void resize()


BackgroundImageImp::BackgroundImageImp( QGraphicsScene *scene, QChart *chart )
{
  const QPointF cpos = chart->pos();
  const QSizeF csize = chart->size();
  m_width = csize.width();
  m_hight = csize.height();
  
  const QMargins cmargins = chart->margins();
  const double plotw = csize.width() - cmargins.left() - cmargins.right();
  const double ploth = csize.height() - cmargins.top() - cmargins.bottom();
  
  QPixmap logopixmap;
  logopixmap.loadFromData( sandia_logo_png, sandia_logo_png_len );
  logopixmap = logopixmap.scaledToWidth( static_cast<int>(0.5*plotw), Qt::SmoothTransformation );
  QGraphicsPixmapItem *logo = scene->addPixmap( logopixmap );
  
  double xpos = cpos.x() + cmargins.left() + 0.25*plotw;
  double ypos = cpos.y() + cmargins.top() + 0.45*ploth - 0.5*logopixmap.height();
  logo->setOffset( xpos, ypos );
  
  const char *msg = "Drag a single spectrum onto app to view, modify, and export it."
                    "\nDragging multiple spectra, or a folder will initiate batch mode.";
  
  QGraphicsTextItem *text = scene->addText( msg );
  
  QFont font;
  font.setPixelSize( 16 );
  text->setFont( font );
  
  double textw = text->boundingRect().width();
  
  if( textw > 0.95*plotw )
  {
    text->setTextWidth( 0.75*plotw );
    textw = 0.75*plotw;
  }

  xpos = cpos.x() + cmargins.left() + 0.5*plotw - 0.5*textw;
  ypos = ypos + logopixmap.height() + 10;
  text->setPos( QPointF(xpos,ypos) );
  
  m_items.push_back( logo );
  m_items.push_back( text );
}//BackgroundImageImp constructor


BackgroundImageImp::~BackgroundImageImp()
{
  for( size_t i = 0; i < m_items.size(); ++i )
    delete m_items[i];
}//~BackgroundImageImp


