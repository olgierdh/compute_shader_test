#version 450

layout( location = 0 ) in vec3 inPos;
layout( location = 1 ) in vec3 inColor;

layout( location = 0 ) out vec3 outColor;

layout( binding = 0 ) uniform uniform_buffer_object
{
    mat4 model;
    mat4 view;
    mat4 proj;
}
ubo;

void main()
{
    outColor    = inColor;
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4( inPos.xyz, 1.0 );
}
