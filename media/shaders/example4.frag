#version 450

layout( location = 0 ) in vec3 inColor;
layout( location = 1 ) in vec3 inPos;
layout( location = 2 ) in vec3 inNormal;
layout( location = 3 ) in vec2 texCoord;

layout( location = 0 ) out vec4 outFragColor;
layout( binding = 1 ) uniform sampler2D texSampler;

vec4 linearize( vec4 c )
{
    return pow( c, vec4( 2.4 ) );
}

vec4 gammaize( vec4 c )
{
    return pow( c, vec4( 1 / 2.4 ) );
}

vec3 simpleReinhardToneMapping(vec3 color)
{
	float exposure = 1.5;
	color *= exposure/(1. + color / exposure);
	return color;
}

vec3 whitePreservingLumaBasedReinhardToneMapping(vec3 color)
{
	float white = 2.;
	float luma = dot(color, vec3(0.2126, 0.7152, 0.0722));
	float toneMappedLuma = luma * (1. + luma / (white*white)) / (1. + luma);
	color *= toneMappedLuma / luma;
	color = pow(color, vec3(1. / 2.2));
	return color;
}

vec4 tone_map( vec3 c )
{
    const float gamma = 2.2;
    vec3 hdrColor = c.rgb;

    // Exposure tone mapping
    vec3 mapped = vec3(1.0) - exp(-hdrColor * 1.5);
    // Gamma correction
    // mapped = pow(mapped, vec3(1.0 / 2.4));

    return vec4( mapped, 1.0 );
}

void main()
{
    vec4 color   = linearize( texture( texSampler, texCoord ) );
    float NdL    = max( dot( inNormal, normalize( vec3( 0.0, 0.0, 1.0 ) ) ), 0.0 );
    float spec   = clamp( pow( NdL, 50.0 ) * 1.0, 0.0, 1.0 ) * 0.2;
    float diff   = NdL;

    outFragColor = vec4( whitePreservingLumaBasedReinhardToneMapping(
        color.rgb * linearize( vec4( 0.4 ) ).rgb + spec + ( diff * ( 1.0 - spec ) ) * color.rgb ), 1.0 );
    // outFragColor = vec4( whitePreservingLumaBasedReinhardToneMapping( vec3( fresnel ) ), 1.0 );
    // outFragColor = vec4( inNormal, 1.0 );
}