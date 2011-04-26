/*
 * Copyright 1993-2010 NVIDIA Corporation.  All rights reserved.
 *
 * Please refer to the NVIDIA end user license agreement (EULA) associated
 * with this source code for terms and conditions that govern your use of
 * this software. Any use, reproduction, disclosure, or distribution of
 * this software and related documentation outside the terms of the EULA
 * is strictly prohibited.
 *
 */

#ifndef __RENDER_PARTICLES__
#define __RENDER_PARTICLES__
#include <glm/glm.hpp>


namespace particle_attributes {
	enum {
		POSITION,
		COLOR,
	};
}

namespace quad_attributes {
	enum {
		POSITION,
		TEXCOORD
	};
}

typedef struct {
	unsigned int vertex_array;
	unsigned int vbo_indices;
	unsigned int num_indices;
	//Don't need these to get it working, but needed for deallocation
	unsigned int vbo_data;
} device_mesh2_t;


typedef struct {
	glm::vec3 pt;
	glm::vec2 texcoord;
} vertex2_t;

enum DisplayMode {
	DISPLAY_DEPTH = 0,
	DISPLAY_NORMAL = 1,
	DISPLAY_POSITION = 2,
	DISPLAY_COLOR = 3,
	DISPLAY_DIFFUSE = 4,
	DISPLAY_DIFFUSE_SPEC = 5,
	DISPLAY_FRESNEL = 6,
	DISPLAY_REFLECTION = 7,
	DISPLAY_FRES_REFL = 8,
	DISPLAY_THICKNESS = 9,
	DISPLAY_REFRAC = 10,
	DISPLAY_TOTAL = 11
};

class ParticleRenderer
{
public:
    ParticleRenderer();
	ParticleRenderer(int w, int h);
    ~ParticleRenderer();

	//Set Vertex Positions from a float array
    void setPositions(float *pos, int numParticles);

	//VBO's For Storing Things (A Lot Faster than Immediate Mode)
	void setVertexBuffer(unsigned int vbo, int numParticles);
    void setColorBuffer(unsigned int vbo) { m_colorVBO = vbo; }

    void display();

    void setPointSize(float size)  { m_pointSize = size; }
    void setParticleRadius(float r) { m_particleRadius = r; }
	void setModelViewMatrix(glm::mat4 m) { 
		m_modelView = m;
	}
	void setProjectionMatrix(glm::mat4 p) { 
		m_projection = p;
	}

	void setModelViewMatrix(float * m);
	void setProjectionMatrix(float * p);
    void setFOV(float fov) { m_fov = fov; }
    void setWindowSize(int w, int h) { m_window_w = w; m_window_h = h; }
	void setNearFarPlane(float n, float f) { m_NEARP = n; m_FARP = f; }
	void setCamPos(float x, float y, float z) {m_CamX=x;m_CamY=y;m_CamZ=z;}
	void setDisplayMode(DisplayMode m) { m_display_type = m ; }
	DisplayMode  getDisplayMode() {return m_display_type; }
	bool loadCubeMapTexture(char *filename);
	void loadSkyBoxTexture(char * filename);
	void renderSkyBox();


protected: // methods
	//FBO Methods
	void _initFBO(int w, int h);
	void _bindFBO(GLuint FBO);
	void _freeFBO();
	void _setTextures();
    void _drawPoints();
	void _initQuad();

	//Shader Loading
	void _initGL();
	void _initNormalPassProgram();
	void _initShaderProgram();
	void _initDepthPassProgram();
	void _initBlurPassProgram();
	void _initThickPassProgram();

protected: // data
    float *m_pos;
    int m_numParticles;

	DisplayMode m_display_type;
    float m_pointSize;
    float m_particleRadius;
    float m_fov;
    int m_window_w, m_window_h;

	glm::mat4 m_modelView;
	glm::mat4 m_projection;
	float m_NEARP;
	float m_FARP;
	float m_CamX;
	float m_CamY;
	float m_CamZ;

	//SkyBox and CubeMap Textures
	int m_faceDim;
	unsigned char *m_cubeTexData;
	GLuint m_cubeMapTexture;

	unsigned char **m_skyTexData;
	GLuint *m_skyBoxTexture;

	unsigned char *m_backgroundTexData;
	
	GLuint m_normal_pass_program;
	GLuint m_depth_pass_program;
	GLuint m_blur_pass_program;
	GLuint m_thick_pass_program;
    GLuint m_program;

    GLuint m_vbo;
    GLuint m_colorVBO;

	GLuint m_depthTexture;
	GLuint m_colorTexture;
	GLuint m_positionTexture;
	GLuint m_normalTexture;
	GLuint m_backgroundTexture;
	GLuint m_blurDepthTexture;
	GLuint m_thickTexture;

	GLuint m_FBO;
	GLuint m_normalsFBO;
	GLuint m_backgroundFBO;
	GLuint m_blurDepthFBO;
	GLuint m_thickFBO;

	device_mesh2_t m_device_quad;
};

#endif //__ RENDER_PARTICLES__
