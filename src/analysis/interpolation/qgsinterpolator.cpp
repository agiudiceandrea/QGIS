/***************************************************************************
                              qgsinterpolator.cpp
                              -------------------
  begin                : Marco 10, 2008
  copyright            : (C) 2008 by Marco Hugentobler
  email                : marco dot hugentobler at karto dot baug dot ethz dot ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsinterpolator.h"
#include "qgsfeatureiterator.h"
#include "qgsvectordataprovider.h"
#include "qgsvectorlayer.h"
#include "qgsgeometry.h"
#include "qgsfeedback.h"
#include "qgsvariantutils.h"

QgsInterpolator::QgsInterpolator( const QList<LayerData> &layerData )
  : mLayerData( layerData )
{
}

QgsInterpolator::Result QgsInterpolator::cacheBaseData( QgsFeedback *feedback )
{
  if ( mLayerData.empty() )
  {
    return QgsInterpolator::Result::Success;
  }

  //reserve initial memory for 100000 vertices
  mCachedBaseData.clear();
  mCachedBaseData.reserve( 100000 );

  const QgsCoordinateReferenceSystem crs = !mLayerData.empty() ? mLayerData.at( 0 ).source->sourceCrs() : QgsCoordinateReferenceSystem();

  double layerStep = !mLayerData.empty() ? 100.0 / mLayerData.count() : 1;
  int layerCount = 0;
  for ( const LayerData &layer : std::as_const( mLayerData ) )
  {
    if ( feedback && feedback->isCanceled() )
      return QgsInterpolator::Result::Canceled;

    QgsFeatureSource *source = layer.source;
    if ( !source )
    {
      return QgsInterpolator::Result::InvalidSource;
    }

    QgsAttributeList attList;
    switch ( layer.valueSource )
    {
      case QgsInterpolator::ValueSource::Attribute:
        attList.push_back( layer.interpolationAttribute );
        break;

      case QgsInterpolator::ValueSource::Z:
      case QgsInterpolator::ValueSource::M:
        break;
    }

    double attributeValue = 0.0;
    bool attributeConversionOk = false;
    double progress = layerCount * layerStep;

    QgsFeatureIterator fit = source->getFeatures( QgsFeatureRequest().setSubsetOfAttributes( attList ).setDestinationCrs( crs, layer.transformContext ) );
    double featureStep = source->featureCount() > 0 ? layerStep / source->featureCount() : layerStep;

    QgsFeature feature;
    while ( fit.nextFeature( feature ) )
    {
      if ( feedback && feedback->isCanceled() )
        return QgsInterpolator::Result::Canceled;

      progress += featureStep;
      if ( feedback )
        feedback->setProgress( progress );

      switch ( layer.valueSource )
      {
        case QgsInterpolator::ValueSource::Attribute:
        {
          QVariant attributeVariant = feature.attribute( layer.interpolationAttribute );
          if ( QgsVariantUtils::isNull( attributeVariant ) ) //attribute not found, something must be wrong (e.g. NULL value)
          {
            continue;
          }
          attributeValue = attributeVariant.toDouble( &attributeConversionOk );
          if ( !attributeConversionOk || std::isnan( attributeValue ) ) //don't consider vertices with attributes like 'nan' for the interpolation
          {
            continue;
          }
          break;
        }

        case QgsInterpolator::ValueSource::Z:
        case QgsInterpolator::ValueSource::M:
          break;
      }

      if ( !addVerticesToCache( feature.geometry(), layer.valueSource, attributeValue ) )
        return QgsInterpolator::Result::FeatureGeometryError;
    }
    layerCount++;
  }

  return QgsInterpolator::Result::Success;
}

bool QgsInterpolator::addVerticesToCache( const QgsGeometry &geom, ValueSource source, double attributeValue )
{
  if ( geom.isNull() || geom.isEmpty() )
    return true; // nothing to do

  //validate source
  switch ( source )
  {
    case QgsInterpolator::ValueSource::Attribute:
      break;

    case QgsInterpolator::ValueSource::M:
      if ( !geom.constGet()->isMeasure() )
        return false;
      else
        break;

    case QgsInterpolator::ValueSource::Z:
      if ( !geom.constGet()->is3D() )
        return false;
      else
        break;
  }

  for ( auto point = geom.vertices_begin(); point != geom.vertices_end(); ++point )
  {
    switch ( source )
    {
      case QgsInterpolator::ValueSource::M:
        mCachedBaseData.push_back( QgsInterpolatorVertexData( ( *point ).x(), ( *point ).y(), ( *point ).m() ) );
        break;

      case QgsInterpolator::ValueSource::Z:
        mCachedBaseData.push_back( QgsInterpolatorVertexData( ( *point ).x(), ( *point ).y(), ( *point ).z() ) );
        break;

      case QgsInterpolator::ValueSource::Attribute:
        mCachedBaseData.push_back( QgsInterpolatorVertexData( ( *point ).x(), ( *point ).y(), attributeValue ) );
        break;
    }
  }
  mDataIsCached = true;
  return true;
}
