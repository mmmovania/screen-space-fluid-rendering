#version 330

uniform sampler2D u_Depthtex;

uniform float u_Far;
uniform float u_Near;

in vec2 fs_Texcoord;

out vec4 out_Depth;

//Depth used in the Z buffer is not linearly related to distance from camera
//This restores linear depth
float linearizeDepth(float exp_depth, float near, float far) {
    return	(2 * near) / (far + near -  exp_depth * (far - near)); 
}

void main()
{       
    //Get Depth Information about the Pixel
    float exp_depth = texture(u_Depthtex,fs_Texcoord).r;
    float lin_depth = linearizeDepth(exp_depth,u_Near,u_Far);
    float blurRadius = (1.0f/lin_depth) * 0.0002f;
    int windowWidth = 5;
    float sum = 0;
    float wsum = 0;
    
    if(exp_depth > 0.99f){
		out_Depth = vec4(exp_depth);
		return;
    }
    
    for(int x = -windowWidth; x < windowWidth; x++){
		for(int y = -windowWidth; y < windowWidth; y++){
			vec2 sample = vec2(fs_Texcoord.s + x*blurRadius, fs_Texcoord.t + y*blurRadius);
			float sampleDepth = texture(u_Depthtex, sample).r;
			
			if(sampleDepth < 0.99f){
				//Spatial
				float r = length(vec2(x,y)) * 0.0001f;
				float w = exp(-r*r);
			
				//Range
				//float r2 = (sampleDepth-exp_depth) * 200.0f;
				//float g = exp(-r2*r2);
			
				sum += sampleDepth * w ;
				wsum += w ;
			}
		}
    }
    
    if(wsum > 0.0f){
		sum = sum/wsum;
    }
    
    out_Depth = vec4(sum);
    //out_Depth = vec4(exp_depth);
}