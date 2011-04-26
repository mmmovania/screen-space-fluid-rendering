#version 330

uniform mat4 u_Persp;

in vec3 Position;
in vec2 Texcoord;

out vec2 fs_Texcoord;
out vec3 fs_Position;

void main()
{
	fs_Texcoord = Texcoord;
	fs_Position =  Position;
	gl_Position = vec4(Position,1.0f);
}