#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_LEFT_HANDED
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include "glm/glm.hpp"
#include "glm/gtx/hash.hpp"

#include <vector>

namespace vtx_t
{
    struct vertex
    {
        glm::vec3 position = glm::vec3( 0 );
        glm::vec3 normal   = glm::vec3( 0 );
        glm::vec2 texcoord = glm::vec2( 0 );

        friend bool operator==( const vertex& lhs, const vertex& rhs )
        {
            return lhs.position == rhs.position && lhs.texcoord == rhs.texcoord;
        }
    };

    using index = uint32_t;
} // namespace vtx_t

namespace std
{
    template <> struct hash< vtx_t::vertex >
    {
        size_t operator()( vtx_t::vertex const& v ) const
        {
            return ( hash< glm::vec3 >()( v.position )
                     ^ ( hash< glm::vec2 >()( v.texcoord ) << 1 ) );
        }
    };
} // namespace std

struct model_data
{
    std::vector< vtx_t::vertex > vertex_data;
    std::vector< vtx_t::index > index_data;
};

model_data load_model( const char* file_name );