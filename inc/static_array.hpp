#pragma once
#include <cstdint>
#include <cassert>
#include <utility>

#include "logger.hpp"
#include "stack_allocator.hpp"

template < typename T, int64_t N, typename TAllocator > class static_array;

template < typename T, int64_t N, typename TAllocator >
constexpr void
swap( static_array< T, N, TAllocator >& lhs, static_array< T, N, TAllocator >& rhs )
{
    using std::swap;
    swap( lhs.m_data, rhs.m_data );
    swap( lhs.m_allocator, rhs.m_allocator );
    swap( lhs.m_current_element, rhs.m_current_element );
}

template < typename T, int64_t N, typename TAllocator > class static_array
{
  public:
    constexpr static_array( TAllocator* a )
        : m_data{nullptr}
        , m_allocator{a}
        , m_current_element{0}
    {
        assert( a != nullptr );
        m_data = reinterpret_cast< T* >( a->allocate( N * sizeof( T ), alignof( T ) ) );
    }

    ~static_array()
    {
        m_allocator->free( m_data );
        m_data            = nullptr;
        m_current_element = 0;
    }

    constexpr static_array( static_array&& other )
        : m_data{std::exchange( other.m_data, nullptr )}
        , m_allocator{std::exchange( other.m_allocator, nullptr )}
        , m_current_element{std::exchange( other.m_current_element, 0 )}
    {
    }

    constexpr static_array& operator=( static_array&& other )
    {
        using std::swap;
        static_array __tmp = std::move( other );
        swap( *this, __tmp );
        return *this;
    }

    constexpr static_array( const static_array& ) = delete;
    constexpr static_array& operator=( const static_array& ) = delete;

  public:
    constexpr friend void swap<>( static_array& lhs, static_array& rhs );

  public:
    constexpr T& operator[]( int64_t index )
    {
        assert( index < m_current_element );
        assert( index >= 0 );
        return m_data[index];
    }

    constexpr const T& operator[]( int64_t index ) const
    {
        assert( index < m_current_element );
        assert( index >= 0 );
        return m_data[index];
    }

    constexpr void push_back( T&& v )
    {
        assert( m_current_element < N );
        new ( &m_data[m_current_element] ) T{std::move( v )};
        m_current_element += 1;
    }

    constexpr void resize( int64_t new_size )
    {
        assert( m_current_element + new_size < N );
        m_current_element += new_size;
    }

    constexpr int64_t size() const { return m_current_element; }

    constexpr int64_t capacity() const { return N; }

    constexpr T* begin() { return &m_data[0]; }

    constexpr T* end() { return &m_data[m_current_element]; }

    constexpr T* data() { return m_data; }

    constexpr const T* data() const { return m_data; }

  private:
    T* m_data;
    TAllocator* m_allocator;
    int64_t m_current_element;
};
