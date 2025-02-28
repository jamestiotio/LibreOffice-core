/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * This file incorporates work covered by the following license notice:
 *
 *   Licensed to the Apache Software Foundation (ASF) under one or more
 *   contributor license agreements. See the NOTICE file distributed
 *   with this work for additional information regarding copyright
 *   ownership. The ASF licenses this file to you under the Apache
 *   License, Version 2.0 (the "License"); you may not use this file
 *   except in compliance with the License. You may obtain a copy of
 *   the License at http://www.apache.org/licenses/LICENSE-2.0 .
 */

#ifndef INCLUDED_SVL_POOLITEM_HXX
#define INCLUDED_SVL_POOLITEM_HXX

#include <sal/config.h>

#include <memory>
#include <vector>

#include <com/sun/star/uno/Any.hxx>
#include <svl/hint.hxx>
#include <svl/svldllapi.h>
#include <svl/typedwhich.hxx>
#include <tools/mapunit.hxx>
#include <tools/long.hxx>
#include <boost/property_tree/ptree_fwd.hpp>

class IntlWrapper;

#define SFX_ITEMS_OLD_MAXREF                0xffef
#define SFX_ITEMS_MAXREF                    0xfffffffe

#define CONVERT_TWIPS                       0x80    // Uno conversion for measurement (for MemberId)

// warning, if there is no boolean inside the any this will always return the value false
inline bool Any2Bool( const css::uno::Any&rValue )
{
    bool bValue = false;
    if( !(rValue >>= bValue) )
    {
        sal_Int32 nNum = 0;
        if( rValue >>= nNum )
            bValue = nNum != 0;
    }

    return bValue;
}

/*
 * The values of this enum describe the degree of textual
 * representation of an item after calling the virtual
 * method <SfxPoolItem::GetPresentation()const>.
 */
enum class SfxItemPresentation
{
    Nameless,
    Complete
};

/**
 * These values have to match the values in the
 * css::frame::status::ItemState IDL
 * to be found at offapi/com/sun/star/frame/status/ItemState.idl
*/
enum class SfxItemState {

    /** Specifies an unknown state. */
    UNKNOWN  = 0,

    /** Specifies that the property is currently disabled. */
    DISABLED = 0x0001,

    /** Specifies that the property is currently in a don't care state.
     * <br/>
     * This is normally used if a selection provides more than one state
     * for a property at the same time.
     */
    DONTCARE = 0x0010,

    /** Specifies that the property is currently in a default state. */
    DEFAULT  = 0x0020,

    /** The property has been explicitly set to a given value hence we know
     * we are not taking the default value.
     * <br/>
     * For example, you may want to get the font color and it might either
     * be the default one or one that has been explicitly set.
    */
    SET      = 0x0040
};

#ifdef DBG_UTIL
SVL_DLLPUBLIC size_t getAllocatedSfxPoolItemCount();
SVL_DLLPUBLIC size_t getUsedSfxPoolItemCount();
#endif

class SfxItemPool;
class SfxItemSet;
typedef struct _xmlTextWriter* xmlTextWriterPtr;

class SVL_DLLPUBLIC SfxPoolItem
{
friend class SfxItemPool;
friend class SfxItemDisruptor_Impl;
friend class SfxItemPoolCache;
friend class SfxItemSet;

    mutable sal_uInt32 m_nRefCount;
    sal_uInt16  m_nWhich;

    // bitfield for flags (instead of SfxItemKind)
    bool        m_bIsVoidItem : 1;
    bool        m_bDeleteOnIdle : 1;
    bool        m_bStaticDefault : 1;
    bool        m_bPoolDefault : 1;

protected:
    // Defines if the Item can be shared/RefCounted else it will be cloned.
    // Default is true - as it should be for all Items. It is needed by some
    // SW items, so protected to let them set it in constructor. If this could
    // be fixed at that Items we may remove this again.
    bool        m_bShareable : 1;

private:
#ifdef DBG_UTIL
    // this flag will make debugging item stuff much simpler
    bool        m_bDeleted : 1;
#endif

private:
    inline void SetRefCount(sal_uInt32 n)
    {
        m_nRefCount = n;
        m_bStaticDefault = m_bPoolDefault = false;
    }

protected:
    void setVoidItem() { m_bIsVoidItem = true; }
    void setDeleteOnIdle() { m_bDeleteOnIdle = true; }
    void setStaticDefault() { m_bStaticDefault = true; }
    void setPoolDefault() { m_bPoolDefault = true; }

public:
    inline void AddRef(sal_uInt32 n = 1) const
    {
        assert(m_nRefCount <= SFX_ITEMS_MAXREF && "AddRef with non-Pool-Item");
        assert(n <= SFX_ITEMS_MAXREF - m_nRefCount && "AddRef: refcount overflow");
        m_nRefCount += n;
    }

    bool isVoidItem() const { return m_bIsVoidItem; }
    bool isDeleteOnIdle() const { return m_bDeleteOnIdle; }
    bool isStaticDefault() const { return m_bStaticDefault; }
    bool isPoolDefault() const { return m_bPoolDefault; }
    bool isShareable() const { return m_bShareable; }

private:
    inline sal_uInt32 ReleaseRef(sal_uInt32 n = 1) const
    {
        assert(m_nRefCount <= SFX_ITEMS_MAXREF && "ReleaseRef with non-Pool-Item");
        assert(n <= m_nRefCount);
        m_nRefCount -= n;
        return m_nRefCount;
    }

protected:
                             explicit SfxPoolItem( sal_uInt16 nWhich = 0 );
                             SfxPoolItem( const SfxPoolItem& rCopy)
                                 : SfxPoolItem(rCopy.m_nWhich) {}

public:
    virtual                  ~SfxPoolItem();

    void                     SetWhich( sal_uInt16 nId )
                             {
                                 // can only change the Which before we are in a set
                                 assert(m_nRefCount==0);
                                 m_nWhich = nId;
                             }
    sal_uInt16               Which() const { return m_nWhich; }
    // StaticWhichCast asserts if the TypedWhichId is not matching its type, otherwise it returns a reference.
    // You can use StaticWhichCast when you are sure about the type at compile time -- like a static_cast.
    template<class T> T& StaticWhichCast(TypedWhichId<T> nId)
    {
        (void)nId;
        assert(nId == m_nWhich);
        assert(dynamic_cast<T*>(this));
        return *static_cast<T*>(this);
    }
    template<class T> const T& StaticWhichCast(TypedWhichId<T> nId) const
    {
        (void)nId;
        assert(nId == m_nWhich);
        assert(dynamic_cast<const T*>(this));
        return *static_cast<const T*>(this);
    }
    // DynamicWhichCast returns nullptr if the TypedWhichId is not matching its type, otherwise it returns a typed pointer.
    // it asserts if the TypedWhichId matches its Which, but not the RTTI type.
    // You can use DynamicWhichCast when you are not sure about the type at compile time -- like a dynamic_cast.
    template<class T> T* DynamicWhichCast(TypedWhichId<T> nId)
    {
        if(m_nWhich != nId)
            return nullptr;
        assert(dynamic_cast<T*>(this));
        return static_cast<T*>(this);
    }
    template<class T> const T* DynamicWhichCast(TypedWhichId<T> nId) const
    {
        if(m_nWhich != nId)
            return nullptr;
        assert(dynamic_cast<const T*>(this));
        return static_cast<const T*>(this);
    }
    virtual bool             operator==( const SfxPoolItem& ) const = 0;
    bool                     operator!=( const SfxPoolItem& rItem ) const
                             { return !(*this == rItem); }

    // Sorting is only used for faster searching in a pool which contains large quantities
    // of a single kind of pool item.
    virtual bool             operator<( const SfxPoolItem& ) const { assert(false); return false; }
    virtual bool             IsSortable() const { return false; }

    // Some item types cannot be IsSortable() (such as because they are modified while stored
    // in a pool, which would change the ordering position, see e.g. 585e0ac43b9bd8a2f714903034).
    // To improve performance in such cases it is possible to reimplement Lookup() to do a linear
    // lookup optimized for the specific class (avoiding virtual functions may allow the compiler
    // to generate better code and class-specific optimizations such as hashing or caching may
    // be used.)
    // If reimplemented, the Lookup() function should search [begin,end) for an item matching
    // this object and return an iterator pointing to the item or the end iterator.
    virtual bool             HasLookup() const { return false; }
    typedef std::vector<SfxPoolItem*>::const_iterator lookup_iterator;
    virtual lookup_iterator  Lookup(lookup_iterator /*begin*/, lookup_iterator end ) const
                             { assert( false ); return end; }

    /**  @return true if it has a valid string representation */
    virtual bool             GetPresentation( SfxItemPresentation ePresentation,
                                    MapUnit eCoreMetric,
                                    MapUnit ePresentationMetric,
                                    OUString &rText,
                                    const IntlWrapper& rIntlWrapper ) const;

    virtual void             ScaleMetrics( tools::Long lMult, tools::Long lDiv );
    virtual bool             HasMetrics() const;

    virtual bool             QueryValue( css::uno::Any& rVal, sal_uInt8 nMemberId = 0 ) const;
    virtual bool             PutValue( const css::uno::Any& rVal, sal_uInt8 nMemberId );

    virtual SfxPoolItem*     Clone( SfxItemPool *pPool = nullptr ) const = 0;
    // clone and call SetWhich
    std::unique_ptr<SfxPoolItem> CloneSetWhich( sal_uInt16 nNewWhich ) const;
    template<class T> std::unique_ptr<T> CloneSetWhich( TypedWhichId<T> nId ) const
    {
        return std::unique_ptr<T>(static_cast<T*>(CloneSetWhich(sal_uInt16(nId)).release()));
    }

    sal_uInt32               GetRefCount() const { return m_nRefCount; }
    virtual void dumpAsXml(xmlTextWriterPtr pWriter) const;
    virtual boost::property_tree::ptree dumpAsJSON() const;

private:
    SfxPoolItem&             operator=( const SfxPoolItem& ) = delete;
};



inline bool IsPoolDefaultItem(const SfxPoolItem *pItem )
{
    return pItem && pItem->isPoolDefault();
}

inline bool IsStaticDefaultItem(const SfxPoolItem *pItem )
{
    return pItem && pItem->isStaticDefault();
}

inline bool IsDefaultItem( const SfxPoolItem *pItem )
{
    return pItem && (pItem->isPoolDefault() || pItem->isStaticDefault());
}

inline bool IsPooledItem( const SfxPoolItem *pItem )
{
    return pItem && pItem->GetRefCount() > 0 && pItem->GetRefCount() <= SFX_ITEMS_MAXREF;
}

SVL_DLLPUBLIC extern SfxPoolItem const * const INVALID_POOL_ITEM;

inline bool IsInvalidItem(const SfxPoolItem *pItem)
{
    return pItem == INVALID_POOL_ITEM;
}

class SVL_DLLPUBLIC SfxPoolItemHint final : public SfxHint
{
    SfxPoolItem* pObj;
public:
    explicit SfxPoolItemHint( SfxPoolItem* Object ) : pObj(Object) {}
    SfxPoolItem* GetObject() const { return pObj; }
};

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
