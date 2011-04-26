#version 330
uniform mat4 u_Persp;
uniform mat4 u_InvTrans;
uniform mat4 u_InvProj;

uniform sampler2D u_Depthtex;
uniform sampler2D u_Positiontex;

uniform float u_Far;
uniform float u_Near;

in vec2 fs_Texcoord;

out vec4 out_Normal;

//Depth used in the Z buffer is not linearly related to distance from camera
//This restores linear depth

float linearizeDepth(float exp_depth, float near, float far) {
    return	(2 * near) / (far + near -  exp_depth * (far - near)); 
}

vec3 uvToEye(vec2 texCoord, float depth){
	float x = texCoord.x * 2.0 - 1.0;
	float y = texCoord.y * 2.0 - 1.0;
	vec4 clipPos = vec4(x , y, depth, 1.0f);
	vec4 viewPos = u_InvProj * clipPos;
	return viewPos.xyz / viewPos.w;
}

vec3 getEyePos(in vec2 texCoord){
	float exp_depth = texture(u_Depthtex,fs_Texcoord).r;
    float lin_depth = linearizeDepth(exp_depth,u_Near,u_Far);
    return uvToEye(texCoord,lin_depth);
}

void main()
{       
    //Get Depth Information about the Pixel
    float exp_depth = texture(u_Depthtex,fs_Texcoord).r;

    //float lin_depth = linearizeDepth(exp_depth,u_Near,u_Far);
	vec3 position = uvToEye(fs_Texcoord, exp_depth);
    
    //vec3 ddx = getEyePos(fs_Texcoord + vec2(texelSize,0)) - position;
    //vec3 ddx2 = position - texture(u_Positiontex,fs_Texcoord + vec2(-texelSize,0)).xyz;
    
	//vec3 ddy = getEyePos(fs_Texcoord + vec2(0,texelSize)) - position;
	//vec3 ddy2 = position - texture(u_Positiontex, fs_Texcoord + vec2(0, -texelSize)).xyz;
		
	//out_Normal = vec4(normalize(cross(ddx, ddy)), 1.0f);
	//out_Normal = vec4(position, 1.0f);
	
	//Compute Gradients of Depth and Cross Product Them to Get Normal
	out_Normal = vec4(normalize(cross(dFdx(position.xyz), dFdy(position.xyz))), 1.0f);
}