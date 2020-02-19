#include "image.hpp"

#include <cstring>

#include "debug.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

image load_image( const char* file_name )
{
    image ret{};

    stbi_uc* pixels =
        stbi_load( file_name, &ret.width, &ret.height, &ret.channels, STBI_rgb_alpha );

    NEO_ASSERT_ALWAYS( pixels != nullptr, "Couldn't load image: ", file_name );

    ret.channels              = 4; //!< guaranteed via STBI_rgb_alpha
    const auto data_byte_size = ret.width * ret.height * ret.channels;

    ret.data.resize( data_byte_size );
    std::memcpy( ret.data.data(), pixels, data_byte_size );
    stbi_image_free( pixels );

    return ret;
}