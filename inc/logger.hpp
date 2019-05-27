#pragma once

#include <iostream>
#include <iomanip>
#include <chrono>

static const auto start = std::chrono::high_resolution_clock::now();

template < typename... Args > void log( Args&&... args )
{
    const uint64_t us = std::chrono::duration_cast< std::chrono::microseconds >(
                            std::chrono::high_resolution_clock::now() - start )
                            .count();
    std::cout << "[" << std::setfill( '0' ) << std::setw( 9 ) << std::fixed
              << std::setprecision( 4 ) << us / 1000000.0f << "]:\t";
    ( ..., ( std::cout << args ) );
    std::cout << std::endl;
}
