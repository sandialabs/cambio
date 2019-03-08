#include <cmath>
#include <vector>

#include <QDebug>
#include <QString>
#include <QFontMetricsF>
#include <QtCharts/QCategoryAxis>

#include "cambio/AxisLabelUtils.h"

using namespace std;
QT_CHARTS_USE_NAMESPACE

TickLabel::TickLabel( double v, TickLength length, const QString &l )
  : value(v), tickLength(length), label(l)
{
}


std::vector<TickLabel> getXAxisLabelTicks( const QCategoryAxis *axis,
                                           const double widthpx )
{
  std::vector<TickLabel> ticks;
  
  
  //This function equivalentish to WAxis::getLabelTicks(...) but makes it so
  //  the x axis labels (hopefully) always line up nicely where we want them
  //  e.g. kinda like multiple of 5, 10, 25, 50, 100, etc.
  static double EPSILON = 1E-3;
  
  //qreal avrgcharwidth = mettric.averageCharWidth();
  //if( avrgcharwidth < 0 )
  //  avrgcharwidth = 8.0;
  //const double labelSpacePx = 10*avrgcharwidth;  //InterSpec uses a flat 50.0 for this
  QFontMetricsF mettric( axis->labelsFont() );
  QRectF maxsize = mettric.boundingRect("3049.13");  //an example of probably the longest label we will see
  const double labelSpacePx = 25 + maxsize.width();
  
  const double rendermin = axis->min();
  const double rendermax = axis->max();
  const int nlabel = widthpx / labelSpacePx;  //changed from Inte
  const double range = rendermax - rendermin;
  double renderInterval;// = range / 10.0;
  const double n = std::pow(10, std::floor(std::log10(range/nlabel)));
  const double msd = range / nlabel / n;
  
  
  if( std::isnan(n) || std::isinf(n) || nlabel<=0 || n<=0.0 )  //JIC
    return ticks;
  
  int subdashes = 0;
  
  if (msd < 1.5)
  {
    subdashes = 2;
    renderInterval = 0.5*n;
  }else if (msd < 3.3)
  {
    subdashes = 5;
    renderInterval = 0.5*n;
  }else if (msd < 7)
  {
    subdashes = 5;
    renderInterval = n;
  }else
  {
    subdashes = 10;
    renderInterval = n;
  }
  
  const double biginterval = subdashes * renderInterval;
  double startEnergy = biginterval * std::floor((rendermin + std::numeric_limits<double>::epsilon()) / biginterval);
  
  if( startEnergy < (rendermin-EPSILON*renderInterval) )
    startEnergy += biginterval;
  
  for( int i = -int(floor(startEnergy-rendermin)/renderInterval); ; ++i)
  {
    if( i > 5000 )  //JIC
      break;
    
    double v = startEnergy + renderInterval * i;
    
    if( (v - rendermax) > EPSILON * renderInterval )
      break;
    
    QString t;
    
    if( i>=0 && (i % subdashes == 0) ){
      t.setNum( v );
    }
    
    ticks.emplace_back( v, i % subdashes == 0 ? TickLabel::Long : TickLabel::Short, t );
  }//for( intervals to draw ticks for )
  
  return ticks;
}//getXAxisLabelTicks(...)


std::vector<TickLabel> getYAxisLabelTicks( const QT_CHARTS_NAMESPACE::QCategoryAxis *axis, const double heightpx )
{
  std::vector<TickLabel> ticks;
  
  //This function equivalentish to WAxis::getLabelTicks(...) but makes it so
  //  the y axis labels (hopefully) always line up nicely where we want them
  //  e.g. kinda like multiple of 5, 10, 25, 50, 100, etc.
  const double EPSILON = 1.0E-3;
  
  const double renderymin = axis->min();
  const double renderymax = axis->max();
  const double range = renderymax - renderymin;
  
  QFontMetricsF mettric( axis->labelsFont() );
  QRectF maxsize = mettric.boundingRect( "1000" );
  const double labelHeight = maxsize.height();

  const bool linear = true;
  
  if( linear )
  {
    //px_per_div: pixels between major and/or minor labels.
    //const double fontPointSize = axis->labelsFont().pointSizeF();
    //const double px_per_div = (fontPointSize <= 0.0 ? 40 : (3.0*fontPointSize));  //InterSpec uses a flat 50.0 for this
    const double px_per_div = 20 + labelHeight;

    //nlabel: approx number of major + minor labels we would like to have.
    const int nlabel = heightpx / px_per_div;
    
    //renderInterval: Inverse of how many large labels to place between powers
    //  of 10 (1 is none, 0.5 is is one).
    double renderInterval;
    
    //n: approx how many labels will be used
    const double n = std::pow(10, std::floor(std::log10(range/nlabel)));
    
    //msd: approx how many sub dashes we would expect there to have to be
    //     to satisfy the spacing of labels we want with the given range.
    const double msd = range / nlabel / n;
    
    
    if( std::isinf(n) || std::isinf(n) || nlabel<=0 || n<=0.0 )  //JIC
      return ticks;
    
    int subdashes = 0;
    
    if( msd < 1.5 )
    {
      subdashes = 2;
      renderInterval = 0.5*n;
    }else if (msd < 3.3)
    {
      subdashes = 5;
      renderInterval = 0.5*n;
    }else if (msd < 7)
    {
      subdashes = 5;
      renderInterval = n;
    }else
    {
      subdashes = 10;
      renderInterval = n;
    }
    
    const double biginterval = subdashes * renderInterval;
    double starty = biginterval * std::floor((renderymin + std::numeric_limits<double>::epsilon()) / biginterval);
    
    if( starty < (renderymin-EPSILON*renderInterval) )
      starty += biginterval;
    
    for( int i = -int(floor(starty-renderymin)/renderInterval); ; ++i)
    {
      if( i > 500 )  //JIC
        break;
      
      const double v = starty + renderInterval * i;
      
      if( (v - renderymax) > EPSILON * renderInterval )
        break;
      
      QString t;
      if( i>=0 && ((i % subdashes) == 0) )
        t.setNum( v );
      TickLabel::TickLength len = ((i % subdashes) == 0) ? TickLabel::Long : TickLabel::Short;
      
      ticks.push_back( TickLabel(v, len, t) );
    }//for( intervals to draw ticks for )
  }else //if( linear )
  {
    //Get the power of 10 just bellow or equal to rendermin.
    int minpower = (renderymin > 0.0) ? (int)floor( log10(renderymin) ) : -1;
    
    //Get the power of 10 just above or equal to renderymax.  If renderymax
    //  is less than or equal to 0, set power to be 0.
    int maxpower = (renderymax > 0.0) ? (int)ceil( log10(renderymax) ): 0;
    
    //Adjust minpower and maxpower
    if( maxpower == minpower )
    {
      //Happens when renderymin==renderymax which is a power of 10
      ++maxpower;
      --minpower;
    }else if( maxpower > 2 && minpower < -1)
    {
      //We had a tiny value (possibly a fraction of a count), as well as a
      //  large value (>1000).
      minpower = -1;
    }else if( maxpower >= 0 && minpower < -1 && (maxpower-minpower) > 6 )
    {
      //we had a tiny power (1.0E-5), as well as one between 1 and 999,
      //  so we will only show the most significant decades
      minpower = maxpower - 5;
    }//if( minpower == maxpower ) / else / else
    
    
    //numdecades: number of decades the data covers, including the decade
    //  above and bellow the data.
    const int numdecades = maxpower - minpower + 1;
    
    //minpxdecade: minimum number of pixels we need per decade.
    //const int minpxdecade = 10;
    const int minpxdecade = 15 + labelHeight;
    
    //labeldelta: the number of decades between successive labeled large ticks
    //  each decade will have a large tick regardless
    int labeldelta = 1;
    
    //nticksperdecade: number of small+large ticks per decade
    int nticksperdecade = 10;
    
    
    if( (heightpx / minpxdecade) < numdecades )
    {
      labeldelta = numdecades / (heightpx / minpxdecade);
      nticksperdecade = 1;
      //        if( labeldelta < 3 )
      //          nticksperdecade = 10;
      //        else if( labeldelta < 4 )
      //          nticksperdecade = 5;
      //        else
      //          nticksperdecade = 3;
    }//if( (heightpx / minpxdecade) < numdecades )
    
    
    int nticks = 0;
    int nmajorticks = 0;
    
    for( int decade = minpower; decade <= maxpower; ++decade )
    {
      const double startcounts = std::pow( 10.0, decade );
      const double deltacounts = 10.0 * startcounts / nticksperdecade;
      const double eps = deltacounts * EPSILON;
      
      if( (startcounts - renderymin) > -eps && (startcounts - renderymax) < eps )
      {
        QString t;
        if( (decade%labeldelta)==0 )
          t.setNum( startcounts );
        ++nticks;
        ++nmajorticks;
        ticks.push_back( TickLabel(startcounts, TickLabel::Long, t) );
      }//if( startcounts >= renderymin && startcounts <= renderymax )
      
      
      for( int i = 1; i < (nticksperdecade-1); ++i )
      {
        const double y = startcounts + i*deltacounts;
        QString t;
        t.setNum( y );
        
        if( (y - renderymin) > -eps && (y - renderymax) < eps )
        {
          ++nticks;
          ticks.push_back( TickLabel(y, TickLabel::Short, t) );
        }
      }//for( int i = 1; i < nticksperdecade; ++i )
    }//for( int decade = minpower; decade <= maxpower; ++decade )
    
    //If we have a decent number of (sub) labels, the user can orient
    //  themselves okay, so well get rid of the minor labels.
    if( (nticks > 8 || (heightpx/nticks) < 25 || nmajorticks > 1) && nmajorticks > 0 )
    {
      for( size_t i = 0; i < ticks.size(); ++i )
        if( ticks[i].tickLength == TickLabel::Short )
          ticks[i].label = "";
    }
    
    if( ticks.empty() )
    {
      qDebug() << "Forcing a single axis point in";
      const double y = 0.5*(renderymin+renderymax);
      QString t;
      t.setNum( y );
      ticks.push_back( TickLabel(y, TickLabel::Long, t) );
    }
  }//if( linear ) / else
  
  return ticks;
}//getYAxisLabelTicks(...)

