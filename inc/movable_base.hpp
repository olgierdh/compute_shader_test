#pragma once

struct movable_base
{
    constexpr movable_base( movable_base&& ) = default;
    constexpr movable_base& operator=( movable_base&& ) = default;
    constexpr movable_base( const movable_base& )       = delete;
    constexpr movable_base& operator=( const movable_base& ) = delete;
};
