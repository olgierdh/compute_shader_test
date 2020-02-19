#pragma once

#include "static_array.hpp"

struct application_data
{
    static constexpr int64_t stack_size = 1024 * 1024 * 2;
    using stack_alloc_t                 = stack_allocator< stack_size >;

    stack_alloc_t stack_alloc{};
};
