#pragma once

#include <cstdint>
#include <cassert>

constexpr int64_t aligned_size( int64_t size, int64_t alignment )
{
    return ( size + alignment - 1 ) & ~( alignment - 1 );
}

template < int64_t N > class stack_allocator
{
  public:
    constexpr stack_allocator()
        : m_current_size{0}
    {
    }
    constexpr stack_allocator( const stack_allocator& ) = delete;
    constexpr stack_allocator& operator=( const stack_allocator& ) = delete;
    constexpr stack_allocator( stack_allocator&& )                 = delete;
    constexpr stack_allocator& operator=( stack_allocator&& ) = delete;

  public:
    constexpr void* allocate( int64_t size, int64_t alignment = alignof( void* ) )
    {
        const int64_t current_address =
            reinterpret_cast< int64_t >( &m_ptr[m_current_size] );
        const int64_t aligned_address = aligned_size( current_address, alignment );
        const int64_t new_size =
            m_current_size + ( aligned_address - current_address ) + size;
        assert( new_size < N );
        m_current_size = new_size;
        return reinterpret_cast< void* >( aligned_address );
    }

    constexpr void free( void* ptr ) {}

  private:
    char m_ptr[N];
    int64_t m_current_size;
};
