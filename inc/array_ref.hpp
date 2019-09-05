#pragma once

#include <utility>

template < typename T > struct array_ref
{
  public:
    template < int N >
    array_ref( T ( &a )[N] )
        : m_ptr( a )
        , m_size( N )
    {
    }

    array_ref( T* data, int size )
        : m_ptr( data )
        , m_size( size )
    {
    }

    array_ref( array_ref&& other )
        : m_ptr{std::exchange( other.m_ptr, nullptr )}
        , m_size{std::exchange( other.m_size )}
    {
    }

    array_ref& operator=( array_ref&& other )
    {
        array_ref __tmp = std::move( other );
        swap( *this, __tmp );
        return *this;
    }

  private:
    T* m_ptr;
    int m_size;
};