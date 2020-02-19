#version 450

layout( location = 0 ) in vec3 inPos;
layout( location = 1 ) in vec3 inNormal;
layout( location = 2 ) in vec2 inTexCoord;

layout( location = 0 ) out vec3 outColor;
layout( location = 1 ) out vec3 outPos;
layout( location = 2 ) out vec3 outNormal;
layout( location = 3 ) out vec2 texCoord;

layout( binding = 0 ) uniform uniform_buffer_object
{
    mat4 model;
    mat4 view;
    mat4 proj;
}
ubo;

void main()
{
    outColor    = vec3( 1 );
    outPos      = ( ( ubo.view * ubo.model ) * vec4( inPos.xyz, 1.0 ) ).xyz;
    texCoord    = vec2( inTexCoord.x, 1.0 - inTexCoord.y );
    outNormal   = ( ubo.view * ubo.model * vec4( inNormal, 0.0 ) ).xyz;
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4( inPos.xyz, 1.0 );
}
