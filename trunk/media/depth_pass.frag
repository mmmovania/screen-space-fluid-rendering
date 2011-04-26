#version 330

uniform mat4x4 u_ModelView;
uniform mat4x4 u_Persp;
uniform mat4x4 u_InvTrans;

uniform float pointRadius;  // point size in world space
uniform float pointScale;   // scale to calculate size in pixels

in vec4 fs_Position;
in vec3 fs_PosEye;
in vec4 fs_Color;

out vec4 out_Color;
out vec4 out_Position;

void main(void)
{
    // calculate normal from texture coordinates
    vec3 N;
    N.xy = gl_PointCoord.xy*vec2(2.0, -2.0) + vec2(-1.0, 1.0);
    float mag = dot(N.xy, N.xy);
    if (mag > 1.0) discard;   // kill pixels outside circle
    N.z = sqrt(1.0-mag);
    
    //calculate depth
    vec4 pixelPos = vec4(fs_PosEye + normalize(N)*pointRadius,1.0f);
    vec4 clipSpacePos = u_Persp * pixelPos;
    gl_FragDepth = clipSpacePos.z / clipSpacePos.w;
    
	out_Position = pixelPos;
	out_Color = fs_Color;
}
