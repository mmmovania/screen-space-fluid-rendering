#version 330

//ENUMS
#define	DISPLAY_DEPTH 0
#define	DISPLAY_NORMAL 1
#define	DISPLAY_POSITION 2
#define	DISPLAY_COLOR 3
#define	DISPLAY_DIFFUSE 4
#define	DISPLAY_DIFFUSE_SPEC 5
#define	DISPLAY_FRESNEL 6
#define	DISPLAY_REFLECTION 7
#define	DISPLAY_FRES_REFL 8
#define	DISPLAY_THICKNESS 9
#define	DISPLAY_REFRAC 10
#define	DISPLAY_TOTAL 11

uniform mat4 u_ModelView;
uniform mat4 u_Persp;
uniform mat4 u_InvTrans;
uniform mat4 u_InvProj;

uniform sampler2D u_Depthtex;
uniform sampler2D u_Normaltex;
uniform sampler2D u_Colortex;
uniform sampler2D u_Positiontex;
uniform sampler2D u_Backgroundtex;
uniform sampler2D u_Thicktex;

uniform samplerCube u_Cubemaptex;

uniform float u_Far;
uniform float u_Near;
uniform int u_DisplayType;

in vec2 fs_Texcoord;
in vec3 fs_Position;

out vec4 out_Color;

//Depth used in the Z buffer is not linearly related to distance from camera
//This restores linear depth

float linearizeDepth(float exp_depth, float near, float far) {
    return	(2 * near) / (far + near -  exp_depth * (far - near)); 
}

vec3 uvToEye(vec2 texCoord, float depth){
	float fovy = radians(60.0);
	float aspect = 640.0f / 480.0f;
	float invFocalLenX   = tan(fovy * 0.5) * aspect;
	float invFocalLenY   = tan(fovy * 0.5);
	//vec2 uv = (texCoord * vec2(2.0,-2.0) - vec2(1.0,-1.0));
	//return vec3(uv * vec2(invFocalLenX, invFocalLenY) * depth, depth);
	
	float x = texCoord.x * 2.0 - 1.0;
	float y = texCoord.y * -2.0 + 1.0;
	vec4 clipPos = vec4(x , y, depth, 1.0f);
	vec4 viewPos = u_InvProj * clipPos;
	return viewPos.xyz / viewPos.w;
}


void main()
{
	//Uniform Light Direction (Billboard)
    vec4 lightDir = u_ModelView * vec4(0.7f, 1.0f, 0.0f, 0.0f);
        
    //Get Texture Information about the Pixel
    vec3 N = texture(u_Normaltex,fs_Texcoord).xyz;
    float exp_depth = texture(u_Depthtex,fs_Texcoord).r;
    float lin_depth = linearizeDepth(exp_depth,u_Near,u_Far);
    vec3 Color = texture(u_Colortex,fs_Texcoord).xyz;
    vec3 BackColor = texture(u_Backgroundtex,fs_Texcoord).xyz;
	vec3 position = texture(u_Positiontex,fs_Texcoord).xyz;
	float thickness = clamp(texture(u_Thicktex,fs_Texcoord).r,0.0f,1.0f);
    //vec3 position = uvToEye(fs_Texcoord,lin_depth).xyz;
    
    vec3 incident = normalize(lightDir.xyz);
    vec3 viewer = normalize(-position.xyz);
    
    //Blinn-Phong Shading Coefficients
    vec3 H = normalize(incident + viewer);
    float specular = pow(max(0.0f, dot(H,N)),50.0f);
    float diffuse = max(0.0f, dot(incident, N));
    
    //Background Only Pixels
    if(exp_depth > 0.99f){
		out_Color = vec4(BackColor.rgb,1.0f);
		return;
	}
    
    //Fresnel Reflection
    float r_0 = 0.3f;
    float fres_refl = r_0 + (1-r_0)*pow(1-dot(N,viewer),5.0f);
    
    //Cube Map Reflection Values
    vec3 reflect = reflect(-viewer,N);
    vec4 refl_color = texture(u_Cubemaptex, reflect);
    
    //Color Attenuation from Thickness
    //(Beer's Law)
    float k_r = 5.0f;
    float k_g = 1.0f;
    float k_b = 0.1f;
    vec3 color_atten = vec3( exp(-k_r*thickness), exp(-k_g*thickness), exp(-k_b*thickness));
    
    //Background Refraction
    vec4 refrac_color = texture(u_Backgroundtex, fs_Texcoord + N.xy*thickness);
    
    //Final Real Color Mix
    float transparency = 1-thickness;
    vec3 final_color = mix(color_atten.rgb * diffuse, refrac_color.rgb,transparency);
    
	switch(u_DisplayType) {
		case(DISPLAY_DEPTH):
			out_Color = vec4(lin_depth,lin_depth,lin_depth,1.0f);
			break;
		case(DISPLAY_NORMAL):
			out_Color = vec4(abs(N.xyz),1.0f);
			break;
		case(DISPLAY_POSITION):
			out_Color = vec4( abs(position)/ u_Far,1.0f);
			break;
		case(DISPLAY_COLOR):
			out_Color = vec4(Color.rgb,1.0f);
			break;
		case(DISPLAY_DIFFUSE):
			out_Color = vec4(Color.rgb * diffuse, 1.0f);
			break;
		case(DISPLAY_DIFFUSE_SPEC):
			out_Color = vec4(Color.rgb * diffuse + specular * vec3(1.0f), 1.0f);
			break;
		case(DISPLAY_FRESNEL):
			out_Color = vec4(vec3(fres_refl), 1.0f);
			break;
		case(DISPLAY_REFLECTION):
			out_Color = vec4(refl_color.rgb, 1.0f);
			break;
		case(DISPLAY_FRES_REFL):
			out_Color = vec4(refl_color.rgb * fres_refl, 1.0f);
			break;
		case(DISPLAY_THICKNESS):
			out_Color = vec4(vec3(thickness),1.0f);
			break;
		case(DISPLAY_REFRAC):
			out_Color = refrac_color;
			break;
		case(DISPLAY_TOTAL):
			out_Color = vec4(final_color.rgb + specular * vec3(1.0f) + refl_color.rgb * fres_refl, 1.0f);
			break;
	}
	
	return;
}