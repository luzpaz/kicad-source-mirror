/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2020 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef BASIC_GAL_H
#define BASIC_GAL_H

#include <eda_rect.h>

#include <gal/stroke_font.h>
#include <gal/graphics_abstraction_layer.h>
#include <newstroke_font.h>

class PLOTTER;


/*
 * A minimal GAL implementation to draw, plot and convert stroke texts to a set of segments
 * for DRC tests, and to calculate text sizes.
 *
 * Currently it allows one to use GAL and STROKE_FONT methods in legacy draw mode
 * (using wxDC functions) in plot functions only for texts.
 * It is used also to calculate the text bounding boxes
 *
 * The main purpose is to avoid duplicate code to do the same thing in GAL canvas,
 * print & plotter canvasses and DRC.
 *
 * It will be certainly removed when a full GAL canvas using wxDC is implemented
 * (or at least restricted to plotter and DRC "canvas")
 */

struct TRANSFORM_PRM    // A helper class to transform coordinates in BASIC_GAL canvas
{
    VECTOR2D m_rotCenter;
    VECTOR2D m_moveOffset;
    double   m_rotAngle;
};


class BASIC_GAL: public KIGFX::GAL
{
public:
    BASIC_GAL( KIGFX::GAL_DISPLAY_OPTIONS& aDisplayOptions ) :
        GAL( aDisplayOptions ),
        m_DC( nullptr ),
        m_Color( RED ),
        m_transform(),
        m_clipBox(),
        m_isClipped( false ),
        m_callback( nullptr ),
        m_callbackData( nullptr ),
        m_plotter( nullptr )
    {
    }

    void SetPlotter( PLOTTER* aPlotter )
    {
        m_plotter = aPlotter;
    }

    void SetCallback( void (* aCallback)( int x0, int y0, int xf, int yf, void* aData ),
                      void* aData  )
    {
        m_callback = aCallback;
        m_callbackData = aData;
    }

    /// Set a clip box for drawings
    /// If NULL, no clip will be made
    void SetClipBox( EDA_RECT* aClipBox )
    {
        m_isClipped = aClipBox != nullptr;

        if( aClipBox )
            m_clipBox = *aClipBox;
    }

    /// Save the context.
    virtual void Save() override
    {
        m_transformHistory.push( m_transform );
    }

    virtual void Restore() override
    {
        m_transform = m_transformHistory.top();
        m_transformHistory.pop();
    }

    /**
     * Draw a polyline
     *
     * @param aPointList is a list of 2D-Vectors containing the polyline points.
     */
    virtual void DrawPolyline( const std::deque<VECTOR2D>& aPointList ) override;

    virtual void DrawPolyline( const VECTOR2D aPointList[], int aListSize ) override;

    /**
     * Start and end points are defined as 2D-Vectors.
     *
     * @param aStartPoint   is the start point of the line.
     * @param aEndPoint     is the end point of the line.
     */
    virtual void DrawLine( const VECTOR2D& aStartPoint, const VECTOR2D& aEndPoint ) override;

    /**
     * Translate the context.
     *
     * @param aTranslation is the translation vector.
     */
    virtual void Translate( const VECTOR2D& aTranslation ) override
    {
        m_transform.m_moveOffset += aTranslation;
    }

    /**
     * Rotate the context.
     *
     * @param aAngle is the rotation angle in radians.
     */
    virtual void Rotate( double aAngle ) override
    {
        m_transform.m_rotAngle = aAngle;
        m_transform.m_rotCenter = m_transform.m_moveOffset;
    }

private:
    void doDrawPolyline( const std::vector<wxPoint>& aLocalPointList );

    // Apply the roation/translation transform to aPoint
    const VECTOR2D transform( const VECTOR2D& aPoint ) const;

public:
    wxDC* m_DC;
    COLOR4D m_Color;

private:
    TRANSFORM_PRM m_transform;
    std::stack <TRANSFORM_PRM>  m_transformHistory;

    // A clip box, to clip drawings in a wxDC (mandatory to avoid draw issues)
    EDA_RECT  m_clipBox;        // The clip box
    bool      m_isClipped;      // Allows/disallows clipping

    // When calling the draw functions outside a wxDC, to get the basic drawings
    // lines / polylines ..., a callback function (used in DRC) to store
    // coordinates of each segment:
    void (* m_callback)( int x0, int y0, int xf, int yf, void* aData );
    void* m_callbackData;       // a optional parameter for m_callback

    // When calling the draw functions for plot, the plotter acts as a wxDC to plot basic items.
    PLOTTER* m_plotter;
};


extern BASIC_GAL basic_gal;

#endif      // define BASIC_GAL_H
