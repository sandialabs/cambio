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


#include <limits>
#include <iostream>

#include <QtCharts/QChart>
#include <QDebug>
#include <qmath.h>
#include <QPointF>
#include <QMargins>
#include <QMimeData>
#include <QDropEvent>
#include <QtCharts/QValueAxis>
#include <QtCharts/QLineSeries>
#include <QRubberBand>
#include <QMouseEvent>
#include <QtCharts/QLogValueAxis>
#include <QDragLeaveEvent>
#include <QDragEnterEvent>
#include <QGraphicsColorizeEffect>

#include "SpecUtils/SpectrumDataStructs.h"

#include "cambio/TimeView.h"
#include "cambio/TimeChart.h"
#include "cambio/AxisLabelUtils.h"


using namespace std;
#if( !defined(QT_CHARTS_VERSION) || QT_CHARTS_VERSION < 0x020000 )
QTCOMMERCIALCHART_USE_NAMESPACE
#else
QT_CHARTS_USE_NAMESPACE
#endif


class TimeViewImp
{
public:
  TimeViewImp( TimeView *q, QChart *chart = nullptr );
  ~TimeViewImp();
  void setChart( QChart *chart );
  void resize();
  
  TimeView *q_ptr;
  QGraphicsScene *m_scene;
  QChart *m_chart;
  QPoint m_mouseDownPos;
  qreal m_mouseDownXMin, m_mouseDownXMax;
  QPointF m_mouseDownCoordVal;
  QRubberBand *m_rubberBand;
  Qt::MouseButton m_downMouseButton;
};//class TimeViewImp


TimeView::TimeView( QWidget *parent )
  : QGraphicsView( parent ),
    m_isTouching( false ),
    m_isSettingXRange( false ),
    m_xaxis( nullptr ),
    m_gammaAxis( nullptr ),
    m_gammaSeries( nullptr ),
    m_neutronSeries( nullptr ),
    m_rebinFactor( 1 )
{
  //On macOS touchpad events arent grabbed unless we have the following.
  //grabGesture( Qt::TapGesture );
  setAttribute( Qt::WA_AcceptTouchEvents );
  
  TimeChart *chart = new TimeChart();
  chart->setContentsMargins( 0, 0, 0, 0 );
  
  chart->setMargins( QMargins( 10, 3, 5, 3 ) );
  
  m_imp.reset( new TimeViewImp( this, chart ) );
  
  m_gammaSeries = new QLineSeries();
  for (int i = 0; i < 500; i++)
  {
    QPointF p( static_cast<qreal>(i), fabs(qSin(M_PI / 50 * i) * 500) + 0.1 );
    p.ry() += qrand() % 20;
    QPointF q( p.x()+1, p.y() );
    *m_gammaSeries << p << q;
  }
  
  m_neutronSeries = new QLineSeries();
  for (int i = 0; i < 250; i++)
  {
    QPointF p( static_cast<qreal>(2*i), fabs(qCos(M_PI / 50 * i) * 500) + 0.1 );
    p.ry() += qrand() % 20;
    QPointF q( p.x()+2, p.y() );
    *m_neutronSeries << p << q;
  }
  
  m_xaxis = new QCategoryAxis;
  m_gammaAxis = new QCategoryAxis;
  m_neutronAxis = new QCategoryAxis;
  
  m_xaxis->setTitleText("Time (s)");
  //m_xaxis->setTickCount(10);
  //m_xaxis->setLabelFormat("%.1f");
  //m_gammaAxis->setLabelFormat("%.1f");
  m_gammaAxis->setTitleText("Gross Counts");
  m_neutronAxis->setTitleText("Neutron Counts");
  
  m_xaxis->setGridLineVisible( false );
  m_gammaAxis->setGridLineVisible( false );
  m_neutronAxis->setGridLineVisible( false );
  
  m_xaxis->setLabelsPosition( QCategoryAxis::AxisLabelsPosition::AxisLabelsPositionOnValue );
  m_gammaAxis->setLabelsPosition( QCategoryAxis::AxisLabelsPosition::AxisLabelsPositionOnValue );
  m_neutronAxis->setLabelsPosition( QCategoryAxis::AxisLabelsPosition::AxisLabelsPositionOnValue );
  
  QObject::connect( m_xaxis, SIGNAL(rangeChanged(qreal,qreal)),
                   this, SLOT(xRangeChanged(qreal,qreal)));
  
  QObject::connect( m_gammaAxis, SIGNAL(rangeChanged(qreal,qreal)),
                   this, SLOT(yRangeChanged(qreal,qreal)));
  
  chart->addAxis( m_xaxis, Qt::AlignBottom );
  chart->addAxis( m_gammaAxis, Qt::AlignLeft );
  chart->addAxis( m_neutronAxis, Qt::AlignRight );
  
//  m_neutronAxis->hide();
  
  chart->addSeries( m_gammaSeries );
  chart->addSeries( m_neutronSeries );
  
  m_gammaSeries->attachAxis( m_xaxis );
  m_gammaSeries->attachAxis( m_gammaAxis );
  
  m_neutronSeries->attachAxis( m_xaxis );
  m_neutronSeries->attachAxis( m_neutronAxis );
  
//  chart->setTitle("Gross Counts");
//  chart->setAnimationOptions( QChart::SeriesAnimations );
  chart->legend()->hide();
  
  setAcceptDrops( true );
}//TimeView constructor



void TimeView::spectrumSampleNumbersChanged( std::shared_ptr<MeasurementInfo> meas,
                          std::set<int> samplenums,
                          std::vector<bool> detectors )
{
  m_highlightedRanges.clear();
 
  if( !!meas && samplenums.size()
      && !!m_grossCounts
      && !!(m_grossCounts->channel_energies())
      && meas->sample_numbers().size() != samplenums.size() )
  {
    const vector<float> times = *(m_grossCounts->channel_energies());
  
    const int first_sample = *meas->sample_numbers().begin();
  
    int startbin = (*samplenums.begin()) - first_sample + 1;
    int lastbin = startbin;
    for( std::set<int>::const_iterator iter = samplenums.begin();
        iter != samplenums.end(); ++iter )
    {
      const int bin = (*iter) - first_sample + 1;
      if( bin > (lastbin+1) )
      {
        const float lowerx = m_grossCounts->GetBinLowEdge( startbin );
        const float upperx = m_grossCounts->GetBinLowEdge( lastbin )
                              + m_grossCounts->GetBinWidth( lastbin );
        m_highlightedRanges.push_back( pair<float,float>(lowerx,upperx) );
        startbin = lastbin = bin;
      }else
      {
        lastbin = bin;
      }
    }//for( iterate over sample )
    
    const float lowerx = m_grossCounts->GetBinLowEdge( startbin );
    const float upperx = m_grossCounts->GetBinLowEdge( lastbin )
        + m_grossCounts->GetBinWidth( lastbin );
    m_highlightedRanges.push_back( pair<float,float>(lowerx,upperx) );
  }//if( !!meas )
  
  drawHighlightedRegions();
}//void spectrumSampleNumbersChanged(...)


void TimeView::setGrossCounts( std::shared_ptr<const Measurement> counts )
{
  m_highlightedRanges.clear();
  m_grossCounts = counts;
  updateSeries();
  drawHighlightedRegions();
}//void setGrossCounts()


void TimeView::updateSeries()
{
  std::shared_ptr<const Measurement> counts = m_grossCounts;
  
  const bool validGamma = (!!counts && !!counts->gamma_counts()
                            && counts->gamma_counts()->size() > 3);
  const bool validNeutron = (!!counts && counts->neutron_counts().size() > 3
                              && counts->neutron_counts_sum() > 1.0 );
  
  m_gammaSeries->clear();
  m_neutronSeries->clear();
  m_gammaAxis->setVisible( validGamma );
  m_neutronSeries->setVisible( validNeutron );
  m_neutronAxis->setVisible( validNeutron );
  
  m_rebinFactor = 1;
  
  if( validGamma )
  {
    const vector<float> &y = *counts->gamma_counts();
    const vector<float> &x = *counts->channel_energies();
    
    const double markerw = m_gammaSeries->pen().widthF();
    const double chartw = m_imp->m_chart->plotArea().width();
    
    const size_t nbin = x.size();
    
    const double maxnbin = chartw / markerw;
    double rebin = std::ceil( nbin / maxnbin );
    
    rebin = std::max( rebin, 1.0 );
    rebin = std::min( rebin, 128.0 );
    m_rebinFactor = static_cast<size_t>( rebin );
    
    const size_t nsumchannel = nbin - m_rebinFactor - 1;
    
    for( size_t i = 0; i < nsumchannel; i += m_rebinFactor )
    {
      double val = 0.0;
      for( size_t j = 0; j < m_rebinFactor; ++j )
        val += y[i+j];
      m_gammaSeries->append( x[i], val );
      m_gammaSeries->append( x[i+m_rebinFactor], val );
    }//for( size_t i = 0; i < nbin; i += m_rebinFactor )
    
    //XXX- possible double counting here
    double lastyval = 0.0;
    const double xmax = 2*x[nbin-1]-x[nbin-2];
    
    for( size_t i = nsumchannel; i < nbin; ++i )
      lastyval += y[i];
    
    m_gammaSeries->append( x[nsumchannel], lastyval);
    m_gammaSeries->append( xmax, lastyval );
    
    m_xaxis->setRange( x[0], xmax );
    
    for( const auto &label : m_xaxis->categoriesLabels() )
      m_xaxis->remove( label );
    
    const auto labels = getXAxisLabelTicks( m_xaxis, m_imp->m_chart->plotArea().width() );
    
    for( const auto &tick : labels )
    {
      if( tick.tickLength == TickLabel::Long )
        m_xaxis->append( tick.label, tick.value );
      //else
      //m_xaxis->append( "", tick.u );
    }//
    
    m_xaxis->setStartValue( m_xaxis->min() );
  }//if( validGamma )
  
  
  if( validNeutron )
  {
    //Assuming m_rebinFactor has already been set (we expect gamma to be valid
    //  whenever neutrons are), if not it defaults to 1 above
    const vector<float> &y = counts->neutron_counts();
    const vector<float> &x = *counts->channel_energies();
    
    const size_t nbin = x.size();
    const size_t nsumchannel = nbin - m_rebinFactor - 1;
    
    for( size_t i = 0; i < nsumchannel; i += m_rebinFactor )
    {
      double val = 0.0;
      for( size_t j = 0; j < m_rebinFactor; ++j )
        val += y[i+j];
      m_neutronSeries->append( x[i], val );
      m_neutronSeries->append( x[i+m_rebinFactor], val );
    }//for( size_t i = 0; i < nbin; i += m_rebinFactor )
    
    //XXX- possible double counting here
    double lastyval = 0.0;
    const double xmax = 2*x[nbin-1]-x[nbin-2];
    
    for( size_t i = nsumchannel; i < nbin; ++i )
      lastyval += y[i];
    
    m_neutronSeries->append( x[nsumchannel], lastyval);
    m_neutronSeries->append( xmax, lastyval );
  }//if( validNeutron )
  
  calcAndSetYRange();
}//void updateSeries(...)


void TimeView::drawHighlightedRegions()
{
  foreach( QGraphicsRectItem *h, m_highlights )
    delete h;
  m_highlights.clear();
  
  if( !m_grossCounts || !m_grossCounts->channel_energies() )
    return;
  
  const double ymin = m_gammaAxis->min();
  const double ymax = m_gammaAxis->max();
  
  QBrush brush( QColor(253,255,184,175) );
  QPen pen;
  pen.setWidth( 0 );
  pen.setStyle( Qt::NoPen);
  
  for( size_t i = 0; i < m_highlightedRanges.size(); ++i )
  {
    const std::pair<float,float> &h = m_highlightedRanges[i];
    
    const QPointF upperleft = m_imp->m_chart->mapToPosition( QPointF(h.first,ymax) );
    const QPointF lowerright = m_imp->m_chart->mapToPosition( QPointF(h.second,ymin) );
    
    const double x = upperleft.x();
    const double y = upperleft.y();
    const double width = lowerright.x() - upperleft.x();
    const double height = lowerright.y() - upperleft.y();
    
    QGraphicsRectItem *item = scene()->addRect( x, y, width, height );
    item->setBrush( brush );
    item->setPen( pen );
//    item->setFocusProxy( m_imp->m_chart );
    m_highlights.push_back( item );
  }//for( size_t i = 0; i < m_highlightedRanges.size(); ++i )
}//void drawHighlightedRegions();


TimeView::~TimeView()
{
  foreach( QGraphicsRectItem *h, m_highlights )
    delete h;
  m_highlights.clear();
}


void TimeView::dragEnterEvent( QDragEnterEvent *event )
{
  const QMimeData *mimeData = event->mimeData();
  if( mimeData && mimeData->hasUrls() )
    event->acceptProposedAction();
}//void dragEnterEvent( QDragEnterEvent *event )


void TimeView::dragMoveEvent( QDragMoveEvent *event )
{
  event->accept();
}//void dragMoveEvent( QDragMoveEvent *event )


void TimeView::dragLeaveEvent( QDragLeaveEvent *event )
{
  event->accept();
}


void TimeView::dropEvent( QDropEvent *event )
{
  emit fileDropped(event);
  event->accept();
}//void dropEvent( QDropEvent *event )


void TimeView::calcAndSetYRange()
{
  const double plotHeightPx = m_imp->m_chart->plotArea().height();
  //Go through here and adjust the y range.
  double ymin = numeric_limits<double>::infinity();
  double ymax = -numeric_limits<double>::infinity();
  
  const double xmin = m_xaxis->min();
  const double xmax = m_xaxis->max();
  
  foreach( const QPointF &p, m_gammaSeries->points() )
  {
    if( p.x() >= xmin && p.x() <= xmax )
    {
      ymin = std::min( ymin, p.y() );
      ymax = std::max( ymax, p.y() );
    }//
  }//foreach( QPointF p, m_gammaSeries->points() )
  
  for( const auto &label : m_gammaAxis->categoriesLabels() )
    m_gammaAxis->remove( label );
  
  if( !isinf(ymin) || !isinf(ymax) )
  {
    m_gammaAxis->setRange( 0.0, 1.1*ymax );
    
    for( const auto &tick : getYAxisLabelTicks(m_gammaAxis, plotHeightPx) )
    {
      if( tick.tickLength == TickLabel::Long )
        m_gammaAxis->append( tick.label, tick.value );
    }//
    m_gammaAxis->setStartValue( m_gammaAxis->min() );
  }
  
  ymax = -numeric_limits<double>::infinity();
  foreach( const QPointF &p, m_neutronSeries->points() )
  {
    if( p.x() >= xmin && p.x() <= xmax )
      ymax = std::max( ymax, p.y() );
  }//foreach( QPointF p, m_gammaSeries->points() )
  
  for( const auto &label : m_neutronAxis->categoriesLabels() )
    m_neutronAxis->remove( label );
  
  if( !isinf(ymax) )
  {
    m_neutronAxis->setRange( 0.0, 1.1*ymax );
    for( const auto &tick : getYAxisLabelTicks(m_neutronAxis, plotHeightPx) )
    {
      if( tick.tickLength == TickLabel::Long )
        m_neutronAxis->append( tick.label, tick.value );
    }//
    m_neutronAxis->setStartValue( m_neutronAxis->min() );
  }
}//void calcAndSetYRange()


void TimeView::xRangeChanged( qreal xmin, qreal xmax )
{
  if( m_isSettingXRange )
    return;
  
  m_isSettingXRange = true;
  
  m_isSettingXRange = false;
}//void xRangeChanged( qreal xmin, qreal xmax )


void TimeView::yRangeChanged( qreal ymin, qreal ymax )
{
  
}//void yRangeChanged( qreal xmin, qreal xmax )



bool TimeView::viewportEvent( QEvent *event )
{
#if defined(IOS) || !defined(__APPLE__)
  if( event->type() == QEvent::TouchBegin )
  {
    // By default touch events are converted to mouse events. So
    // after this event we will get a mouse event also but we want
    // to handle touch events as gestures only. So we need this safeguard
    // to block mouse events that are actually generated from touch.
    m_isTouching = true;
    
    // Turn off animations when handling gestures they
    // will only slow us down.
    m_imp->m_chart->setAnimationOptions(QChart::NoAnimation);
  }else if( event->type() == QEvent::TouchEnd )
  {
    auto e = dynamic_cast<QTouchEvent *>(event);
    if( !e || !e->touchPoints().size() )
      m_isTouching = false;
  }
#endif
    
  return QGraphicsView::viewportEvent(event);
}//bool viewportEvent( QEvent *event )


void TimeView::keyPressEvent( QKeyEvent *event )
{
  QGraphicsView::keyPressEvent(event);
  
//  switch( event->key() )
//  {
//    case Qt::Key_Plus:
//      break;
//    case Qt::Key_Minus:
//      break;
//    case Qt::Key_Left:
//    {
//      break;
//    }
      
//    case Qt::Key_Right:
//    {
//      break;
//    }
      
//    case Qt::Key_Up:
//      m_imp->m_chart->scroll(0, 10);
//      break;
//    case Qt::Key_Down:
//      m_imp->m_chart->scroll(0, -10);
//      break;
//    default:
//      QGraphicsView::keyPressEvent(event);
//      break;
//  }//switch( event->key() )
   
}//void keyPressEvent( QKeyEvent *event )


void TimeView::mousePressEvent( QMouseEvent *event )
{
  if( m_isTouching )
    return;
  
  m_imp->m_downMouseButton = event->button();
  m_imp->m_chart->setAnimationOptions(QChart::NoAnimation);
  
  QPoint pos = event->pos();
  const QRectF plotArea = m_imp->m_chart->plotArea();
  
  if( plotArea.contains(pos)
      || (pos.y() > plotArea.y() && pos.y() < (plotArea.y()+plotArea.height()) ) )
  {
    if( pos.x() < plotArea.x() )
      pos.setX( plotArea.x() );
    if( pos.x() > (plotArea.x()+plotArea.width()) )
      pos.setX( plotArea.x()+plotArea.width() );
    
    m_imp->m_mouseDownPos = pos;
    m_imp->m_mouseDownCoordVal
                             = m_imp->m_chart->mapToValue( pos, m_gammaSeries );
    m_imp->m_mouseDownXMin = m_xaxis->min();
    m_imp->m_mouseDownXMax = m_xaxis->max();
    
    if( event->button() == Qt::LeftButton )
    {
      event->accept();
    }else if( event->button() == Qt::RightButton )
    {
      event->accept();
    }
  }else
  {
    QGraphicsView::mousePressEvent( event );
  }
}//void mousePressEvent(QMouseEvent *event)


void TimeView::mouseMoveEvent( QMouseEvent *event )
{
  if( m_isTouching )
    return;
  
  const QPoint pos = event->pos();
  const QRect rect = m_imp->m_chart->plotArea().toRect();
  const QPointF coorval = m_imp->m_chart->mapToValue( pos, m_gammaSeries );
  
  if( event->buttons() == Qt::NoButton )
  {
    m_imp->m_rubberBand->hide();
  }else if( event->buttons() == Qt::LeftButton
            || event->buttons() == Qt::RightButton )
  {
    int width = pos.x() - m_imp->m_mouseDownPos.x();
    m_imp->m_rubberBand->show();
    const QRect rubberrect( m_imp->m_mouseDownPos.x(),
                              rect.top(), width, rect.height() );

    QGraphicsColorizeEffect *e = new QGraphicsColorizeEffect(m_imp->m_rubberBand);
    
    if( event->buttons() == Qt::LeftButton )
      e->setColor( QColor(181,213,255,255) );
    else
      e->setColor( QColor(250,155,164,255) );
    
    m_imp->m_rubberBand->setGraphicsEffect(e);
    m_imp->m_rubberBand->setGeometry( rubberrect.normalized() );
  }else
  {
    QGraphicsView::mouseMoveEvent( event );
    return;
  }
  
  int channel = -1;
  double gammacounts = -1.0, neutroncounts = -1.0;
  
  if( !!m_grossCounts )
  {
    const int bin = m_grossCounts->FindFixBin( coorval.x() );
    if( bin >= 1 && bin <= m_grossCounts->GetNbinsX() )
    {
      channel = bin - 1;
      gammacounts = m_grossCounts->GetBinContent( bin );
      if( (bin-1) < static_cast<int>(m_grossCounts->neutron_counts().size()) )
        neutroncounts = m_grossCounts->neutron_counts()[bin-1];
    }
  }//if( !!m_grossCounts )
  
  emit mousePositionChanged( channel, coorval.x(), coorval.y(), gammacounts, neutroncounts );
  
  event->accept();
}//void mouseMoveEvent(QMouseEvent *event)


void TimeView::mouseReleaseEvent( QMouseEvent *event )
{
  m_isTouching = false;
  QPoint pos = event->pos();
  
  if( m_imp->m_downMouseButton == event->button()
      && (event->button() == Qt::LeftButton
          || event->button() == Qt::RightButton) )
  {
    m_imp->m_rubberBand->hide();
  
    const QRectF plotArea = m_imp->m_chart->plotArea();
    if( pos.x() < plotArea.x() )
      pos.setX( plotArea.x() );
    if( pos.x() > (plotArea.x()+plotArea.width()) )
      pos.setX( plotArea.x()+plotArea.width() );
    
    const int beginXPx = std::min( pos.x(), m_imp->m_mouseDownPos.x() );
    const int endXPx = std::max( pos.x(), m_imp->m_mouseDownPos.x() );
    
    const QPointF pxPos0( beginXPx, m_imp->m_mouseDownPos.y() );
    const QPointF pxPos1( endXPx, m_imp->m_mouseDownPos.y() );
    
    const QPointF time0 = m_imp->m_chart->mapToValue( pxPos0, m_gammaSeries );
    const QPointF time1 = m_imp->m_chart->mapToValue( pxPos1, m_gammaSeries );
    const float t0 = static_cast<float>( time0.x() );
    const float t1 = static_cast<float>( time1.x() );
    
    if( !m_grossCounts )
      return;
  
    std::set<int> samples;
    std::shared_ptr< const vector<float> > timesptr
                                           = m_grossCounts->channel_energies();
    
    if( !timesptr )
      return;
    
    const vector<float> &times = *timesptr;
    vector<float>::const_iterator startpos, endpos;
    startpos = std::upper_bound( times.begin(), times.end(), t0 );
    if( startpos != times.begin() )
      --startpos;
    endpos = std::upper_bound( times.begin(), times.end(), t1 );
    if( endpos == times.end() )
      --endpos;
    
    const int firstsample = static_cast<int>( startpos - times.begin() );
    const int lastsample = static_cast<int>( endpos - times.begin() );
    
    Qt::KeyboardModifiers mods = event->modifiers();
    if( event->button() == Qt::RightButton )
      mods |= Qt::ControlModifier;
    
    emit timeRangeSelected( firstsample, lastsample, mods );
  
    m_imp->m_downMouseButton = Qt::NoButton;
  }else
  {
    QGraphicsView::mouseReleaseEvent( event );
    return;
  }
  
  event->accept();
}//void mouseReleaseEvent( QMouseEvent *event )


void TimeView::leaveEvent ( QEvent *event )
{
  m_imp->m_rubberBand->hide();
  m_imp->m_downMouseButton = Qt::NoButton;
  
  emit mouseLeft();
  event->accept();
}//void leaveEvent ( QEvent *event )


void TimeView::resizeEvent( QResizeEvent *event )
{
  QGraphicsView::resizeEvent(event);
  m_imp->resize();
  
  drawHighlightedRegions();
}//void resizeEvent( QResizeEvent *event )


TimeViewImp::TimeViewImp( TimeView *q, QChart *chart )
: q_ptr( q ),
  m_scene( new QGraphicsScene( q ) ),
  m_chart( chart ),
  m_rubberBand( 0 ),
  m_downMouseButton( Qt::NoButton )
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
  m_rubberBand =  new QRubberBand( QRubberBand::Rectangle, q );
}


TimeViewImp::~TimeViewImp()
{
}


void TimeViewImp::setChart(QChart *chart)
{
  Q_ASSERT(chart);
  
  if( m_chart == chart )
    return;
  
  if( m_chart )
    m_scene->removeItem( m_chart );
  
  m_chart = chart;
  m_scene->addItem( m_chart );
  
  resize();
}


void TimeViewImp::resize()
{
  // Fit the chart into view if it has been rotated
  qreal sinA = qAbs(q_ptr->transform().m21());
  qreal cosA = qAbs(q_ptr->transform().m11());
  QSize chartSize = q_ptr->size();
  
  if (sinA == 1.0) {
    chartSize.setHeight(q_ptr->size().width());
    chartSize.setWidth(q_ptr->size().height());
  } else if (sinA != 0.0) {
    // Non-90 degree rotation, find largest square chart that can fit into the view.
    qreal minDimension = qMin(q_ptr->size().width(), q_ptr->size().height());
    qreal h = (minDimension - (minDimension / ((sinA / cosA) + 1.0))) / sinA;
    chartSize.setHeight(h);
    chartSize.setWidth(h);
  }
  
  m_chart->resize(chartSize);
  q_ptr->setMinimumSize(m_chart->minimumSize().toSize());
  q_ptr->setSceneRect(m_chart->geometry());
  
  q_ptr->updateSeries();
}

