#include "test_resource.hpp"

template <> void test_resource::deleter( int& resource ) { resource = -1; };

template <> template <> test_resource test_resource::make( int&& a )
{
    return test_resource( std::move( a ) );
}
