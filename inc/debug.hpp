#pragma once

#include "logger.hpp"

#define NEO_ASSERT_ALWAYS_IMPL( cnd, line, file_name, ... )                              \
    if ( !( cnd ) )                                                                      \
    {                                                                                    \
        log( "ASSERT: [", file_name, ":", line, "] ", __VA_ARGS__ );                      \
        exit( -1 );                                                                      \
    }
#define NEO_ASSERT_ALWAYS( cnd, ... )                                                    \
    NEO_ASSERT_ALWAYS_IMPL( cnd, __LINE__, __FILE__, __VA_ARGS__ )

