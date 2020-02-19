#include <numeric>

#include "model.hpp"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include "debug.hpp"

namespace detail
{
    model_data calculate_normals( model_data&& m )
    {
        for ( uint32_t i = 0; i < m.index_data.size(); i += 3 )
        {
            auto& v0 = m.vertex_data[m.index_data[i]];
            auto& v1 = m.vertex_data[m.index_data[i + 1]];
            auto& v2 = m.vertex_data[m.index_data[i + 2]];

            const auto n = glm::normalize(
                glm::cross( v1.position - v0.position, v2.position - v0.position ) );

            v0.normal += n;
            v1.normal += n;
            v2.normal += n;
        }

        for ( auto& v : m.vertex_data )
        {
            v.normal = glm::normalize( v.normal );
        }

        return std::move( m );
    }
} // namespace detail

model_data load_model( const char* file_name )
{
    model_data ret{};
    tinyobj::attrib_t attrib;

    std::vector< tinyobj::shape_t > shapes;
    std::vector< tinyobj::material_t > materials;
    std::string warn{}, err{};

    const auto res =
        tinyobj::LoadObj( &attrib, &shapes, &materials, &warn, &err, file_name );

    NEO_ASSERT_ALWAYS( res == true, "Couldn't load model: ", file_name, " ", warn.c_str(),
                       " ", err.c_str() );

    std::unordered_map< vtx_t::vertex, uint32_t > unique_vertices = {};

    ret.vertex_data.reserve( attrib.vertices.size() );
    const auto max_indices = std::accumulate(
        shapes.cbegin(), shapes.cend(), 0,
        []( auto sum, const auto& s ) { return sum + s.mesh.indices.size(); } );
    ret.index_data.reserve( max_indices );

    for ( const auto& shape : shapes )
    {
        for ( const auto& index : shape.mesh.indices )
        {
            vtx_t::vertex v{};

            v.position = {attrib.vertices[3 * index.vertex_index + 0],
                          attrib.vertices[3 * index.vertex_index + 1],
                          attrib.vertices[3 * index.vertex_index + 2]};

            v.position.y = -v.position.y;

            v.texcoord = {attrib.texcoords[2 * index.texcoord_index + 0],
                          attrib.texcoords[2 * index.texcoord_index + 1]};

            if ( unique_vertices.count( v ) == 0 )
            {
                unique_vertices[v] = static_cast< uint32_t >( ret.vertex_data.size() );
                ret.vertex_data.push_back( v );
            }

            ret.index_data.push_back( unique_vertices[v] );
        }
    }

    return detail::calculate_normals( std::move( ret ) );
}