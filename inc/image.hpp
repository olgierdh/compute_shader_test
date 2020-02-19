#pragma once

#include <cstdint>
#include <vector>

struct image
{
    std::vector< unsigned char > data;
    int32_t width;
    int32_t height;
    int32_t channels;
};

image load_image( const char* file_name );