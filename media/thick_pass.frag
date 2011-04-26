#version 330

#define PI 3.14159265358979323846264

uniform mat4x4 u_ModelView;
uniform mat4x4 u_Persp;
uniform mat4x4 u_InvTrans;

uniform float pointRadius;  // point size in world space
uniform float pointScale;   // scale to calculate size in pixels

in vec4 fs_Position;
in vec3 fs_PosEye;

out vec4 out_Thick;

void main(void)
{
    // calculate normal from texture coordinates
    vec3 N;
    N.xy = gl_PointCoord.xy*vec2(2.0, -2.0) + vec2(-1.0, 1.0);
    float mag = dot(N.xy, N.xy);
    if (mag > 1.0) discard;   // kill pixels outside circle
    
	//Gaussian Distribution
	float dist = length(gl_PointCoord.xy-vec2(0.5f,0.5f));
	float sigma = 3.0f;
	float mu = 0.0f;
	float g_dist = 0.02f * exp(-(dist-mu)*(dist-mu)/(2*sigma));
	
	out_Thick = vec4(g_dist);
}
