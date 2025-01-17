/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2017 CERN
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

#ifndef SELECTION_H
#define SELECTION_H

#include <algorithm>
#include <core/optional.h>
#include <deque>
#include <eda_rect.h>
#include <eda_item.h>
#include <view/view_group.h>

class SELECTION : public KIGFX::VIEW_GROUP
{
public:
    SELECTION() :
            KIGFX::VIEW_GROUP::VIEW_GROUP()
    {
        m_isHover = false;
    }

    SELECTION( const SELECTION& aOther ) :
            KIGFX::VIEW_GROUP::VIEW_GROUP()
    {
        m_items = aOther.m_items;
        m_isHover = aOther.m_isHover;
    }

    SELECTION& operator= ( const SELECTION& aOther )
    {
        m_items = aOther.m_items;
        m_isHover = aOther.m_isHover;
        return *this;
    }

    using ITER = std::deque<EDA_ITEM*>::iterator;
    using CITER = std::deque<EDA_ITEM*>::const_iterator;

    ITER begin() { return m_items.begin(); }
    ITER end() { return m_items.end(); }
    CITER begin() const { return m_items.cbegin(); }
    CITER end() const { return m_items.cend(); }

    void SetIsHover( bool aIsHover )
    {
        m_isHover = aIsHover;
    }

    bool IsHover() const
    {
        return m_isHover;
    }

    virtual void Add( EDA_ITEM* aItem )
    {
        // We're not sorting here; this is just a time-optimized way to do an
        // inclusion check.  std::lower_bound will return the first i >= aItem
        // and the second i > aItem check rules out i == aItem.
        ITER i = std::lower_bound( m_items.begin(), m_items.end(), aItem );

        if( i == m_items.end() || *i > aItem )
            m_items.insert( i, aItem );
    }

    virtual void Remove( EDA_ITEM *aItem )
    {
        ITER i = std::lower_bound( m_items.begin(), m_items.end(), aItem );

        if( !( i == m_items.end() || *i > aItem  ) )
            m_items.erase( i );
    }

    virtual void Clear() override
    {
        m_items.clear();
    }

    virtual unsigned int GetSize() const override
    {
        return m_items.size();
    }

    virtual KIGFX::VIEW_ITEM* GetItem( unsigned int aIdx ) const override
    {
        if( aIdx < m_items.size() )
            return m_items[ aIdx ];

        return nullptr;
    }

    bool Contains( EDA_ITEM* aItem ) const
    {
        CITER i = std::lower_bound( m_items.begin(), m_items.end(), aItem );

        return !( i == m_items.end() || *i > aItem  );
    }

    /// Checks if there is anything selected
    bool Empty() const
    {
        return m_items.empty();
    }

    /// Returns the number of selected parts
    int Size() const
    {
        return m_items.size();
    }

    const std::deque<EDA_ITEM*> GetItems() const
    {
        return m_items;
    }

    /// Returns the center point of the selection area bounding box.
    virtual VECTOR2I GetCenter() const
    {
        KICAD_T labelTypes[] = { SCH_LABEL_T, SCH_GLOBAL_LABEL_T, SCH_HIER_LABEL_T, EOT };
        bool includeLabels = true;

        // If the selection contains at least one non-label then don't include labels when
        // calculating the centerpoint.

        for( EDA_ITEM* item : m_items )
        {
            if( !item->IsType( labelTypes ) )
            {
                includeLabels = false;
                break;
            }
        }

        EDA_RECT bbox;

        for( EDA_ITEM* item : m_items )
        {
            if( !item->IsType( labelTypes ) || includeLabels )
                bbox.Merge( item->GetBoundingBox() );
        }

        return static_cast<VECTOR2I>( bbox.Centre() );
    }

    virtual const BOX2I ViewBBox() const override
    {
        BOX2I r;
        r.SetMaximum();
        return r;
    }

    /// Returns the top left point of the selection area bounding box.
    VECTOR2I GetPosition() const
    {
        return static_cast<VECTOR2I>( GetBoundingBox().GetPosition() );
    }

    virtual EDA_RECT GetBoundingBox() const
    {
        EDA_RECT bbox;

        for( EDA_ITEM* item : m_items )
            bbox.Merge( item->GetBoundingBox() );

        return bbox;
    }

    virtual EDA_ITEM* GetTopLeftItem( bool onlyModules = false ) const
    {
        return nullptr;
    }

    EDA_ITEM* operator[]( const size_t aIdx ) const
    {
        if( aIdx < m_items.size() )
            return m_items[ aIdx ];

        return nullptr;
    }

    EDA_ITEM* Front() const
    {
        return m_items.size() ? m_items.front() : nullptr;
    }

    std::deque<EDA_ITEM*>& Items()
    {
        return m_items;
    }

    template<class T>
    T* FirstOfKind() const
    {
        auto refType = T( nullptr ).Type();

        for( auto item : m_items )
        {
            if( item->Type() == refType )
                return static_cast<T*> ( item );
        }

        return nullptr;
    }

    /**
     * Checks if there is at least one item of requested kind.
     *
     * @param aType is the type to check for.
     * @return True if there is at least one item of such kind.
     */
    bool HasType( KICAD_T aType ) const
    {
        for( auto item : m_items )
        {
            if( item->Type() == aType )
                return true;
        }

        return false;
    }

    virtual const VIEW_GROUP::ITEMS updateDrawList() const override
    {
        std::vector<VIEW_ITEM*> items;

        for( auto item : m_items )
            items.push_back( item );

        return items;
    }

    bool HasReferencePoint() const
    {
        return m_referencePoint != NULLOPT;
    }

    VECTOR2I GetReferencePoint() const
    {
        return *m_referencePoint;
    }

    void SetReferencePoint( const VECTOR2I& aP )
    {
        m_referencePoint = aP;
    }

    void ClearReferencePoint()
    {
        m_referencePoint = NULLOPT;
    }

    /**
     * Checks if all items in the selection are the same KICAD_T type
     *
     * @return True if all items are the same type, this includes zero or single items
     */
    bool AreAllItemsIdentical() const
    {
        return ( std::all_of( m_items.begin() + 1, m_items.end(),
                        [&]( const EDA_ITEM* r )
                        {
                            return r->Type() == m_items.front()->Type();
                        } ) );
    }

protected:
    OPT<VECTOR2I>         m_referencePoint;
    std::deque<EDA_ITEM*> m_items;
    bool                  m_isHover;

    // mute hidden overloaded virtual function warnings
    using VIEW_GROUP::Add;
    using VIEW_GROUP::Remove;
};


#endif
