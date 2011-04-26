#version 330

uniform mat4x4 u_ModelView;
uniform mat4x4 u_Persp;
uniform mat4x4 u_InvTrans;

uniform float pointRadius;  // point size in world space
uniform float pointScale;   // scale to calculate size in pixels

in vec3 Position;
in vec3 Color;

out vec3 fs_PosEye;
out vec4 fs_Position;

void main(void) {
	vec3 posEye = (u_ModelView * vec4(Position.xyz, 1.0f)).xyz;
	float dist = length(posEye);
	gl_PointSize = pointRadius * (pointScale/dist);
	
	fs_PosEye = posEye;
	fs_Position = u_ModelView * vec4(Position.xyz, 1.0);
	gl_Position = u_Persp * u_ModelView * vec4(Position.xyz, 1.0);
}