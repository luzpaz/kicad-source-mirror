/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Cirilo Bernardo <cirilo.bernardo@gmail.com>
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

#ifndef KICADFOOTPRINT_H
#define KICADFOOTPRINT_H

#include "base.h"
#include "kicadpcb.h"

#include <string>
#include <vector>

namespace SEXPR
{
    class SEXPR;
}

class KICADPAD;
class KICADCURVE;
class KICADMODEL;
class PCBMODEL;
class S3D_RESOLVER;

class KICADFOOTPRINT
{
private:
    bool parseModel( SEXPR::SEXPR* data );
    bool parseCurve( SEXPR::SEXPR* data, CURVE_TYPE aCurveType );
    bool parseLayer( SEXPR::SEXPR* data );
    bool parsePosition( SEXPR::SEXPR* data );
    bool parseAttribute( SEXPR::SEXPR* data );
    bool parseText( SEXPR::SEXPR* data );
    bool parsePad( SEXPR::SEXPR* data );
    bool parseRect( SEXPR::SEXPR* data );

    KICADPCB*   m_parent;     // The parent KICADPCB, to know layer names

    LAYERS      m_side;
    std::string m_refdes;
    DOUBLET     m_position;
    double      m_rotation; // rotation (radians)
    bool        m_virtual;  // true for a virtual (usually mechanical) component

    std::vector< KICADPAD* >    m_pads;
    std::vector< KICADCURVE* >  m_curves;
    std::vector< KICADMODEL* >  m_models;

public:
    KICADFOOTPRINT( KICADPCB* aParent );
    virtual ~KICADFOOTPRINT();

    bool Read( SEXPR::SEXPR* aEntry );

    bool ComposePCB( class PCBMODEL* aPCB, S3D_RESOLVER* resolver,
        DOUBLET aOrigin, bool aComposeVirtual = true );
};

#endif  // KICADFOOTPRINT_H
