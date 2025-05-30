/***************************************************************************
  qgsonlinedemterrainsettings.h
  --------------------------------------
  Date                 : August 2024
  Copyright            : (C) 2024 by Nyall Dawson
  Email                : nyall dot dawson at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSONLINEDEMTERRAINSETTINGS_H
#define QGSONLINEDEMTERRAINSETTINGS_H

#include "qgis_3d.h"
#include "qgis_sip.h"
#include "qgsabstractterrainsettings.h"
#include "qgsrectangle.h"

/**
 * \ingroup qgis_3d
 * \brief Terrain settings for a terrain generator that uses an online DEM to build terrain.
 *
 * \warning This is not considered stable API, and may change in future QGIS releases. It is
 * exposed to the Python bindings as a tech preview only.
 *
 * \since QGIS 3.42
 */
class _3D_EXPORT QgsOnlineDemTerrainSettings : public QgsAbstractTerrainSettings
{
  public:
    /**
     * Creates a new instance of a QgsOnlineDemTerrainSettings object.
     */
    static QgsAbstractTerrainSettings *create() SIP_FACTORY;

    QgsOnlineDemTerrainSettings *clone() const final SIP_FACTORY;
    QString type() const final;
    void readXml( const QDomElement &element, const QgsReadWriteContext &context ) final;
    void writeXml( QDomElement &element, const QgsReadWriteContext &context ) const final;
    bool equals( const QgsAbstractTerrainSettings *other ) const final;
    std::unique_ptr<QgsTerrainGenerator> createTerrainGenerator( const Qgs3DRenderContext &context ) const override SIP_SKIP;

    /**
     * Sets the \a resolution of the terrain (how many elevation samples are taken on one side of a terrain tile).
     *
     * \see resolution()
     */
    void setResolution( int resolution ) { mResolution = resolution; }

    /**
     * Returns the resolution of the terrain (how many elevation samples are taken on one side of a terrain tile).
     *
     * \see resolution()
     */
    int resolution() const { return mResolution; }

    /**
     * Sets the skirt \a height (in world units).
     *
     * Skirts at the edges of terrain tiles help hide cracks between adjacent tiles.
     *
     * \see skirtHeight()
     */
    void setSkirtHeight( double height ) { mSkirtHeight = height; }

    /**
     * Returns the skirt height (in world units).
     *
     * Skirts at the edges of terrain tiles help hide cracks between adjacent tiles.
     *
     * \see setSkirtHeight()
     */
    double skirtHeight() const { return mSkirtHeight; }

  private:
    //! how many vertices to place on one side of the tile
    int mResolution = 16;
    //! height of the "skirts" at the edges of tiles to hide cracks between adjacent cracks
    double mSkirtHeight = 10.;
};


#endif // QGSONLINEDEMTERRAINSETTINGS_H
