#pragma once

#include <utility>
#include "movable_base.hpp"

template < typename T > struct resource;

template < typename T > constexpr void swap( resource< T >& lhs, resource< T >& rhs )
{
    using std::swap;
    swap( lhs.m_resource, rhs.m_resource );
}

template < typename T > struct resource : public movable_base
{
    using movable_base::movable_base;

    template < typename... Args > static resource make( Args... args );
    static void deleter( T& resource );

    constexpr resource()
        : movable_base{}
        , m_resource( T{} )
    {
    }

  private:
    constexpr resource( T resource )
        : movable_base{}
        , m_resource( std::move( resource ) )
    {
    }

  public:
    constexpr resource( resource&& other )
        : movable_base{}
        , m_resource{std::exchange( other.m_resource, T{} )}
    {
    }

    ~resource() { deleter( m_resource ); }

    constexpr resource& operator=( resource&& other )
    {
        resource __tmp{std::move( other )};
        swap( *this, __tmp );
        return *this;
    }

    friend constexpr void swap<>( resource& lhs, resource& rhs );
    constexpr const T& get_value() const { return m_resource; }

  private:
    T m_resource;
};
