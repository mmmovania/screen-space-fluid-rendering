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

#include <GL/glew.h>

#include <math.h>
#include <assert.h>
#include <stdio.h>
#include <iostream>
#include "IL/ilu.h"
#include "IL/ilut.h"

#include "Utility.h"
#include "render_particles.h"

#ifndef M_PI
#define M_PI    3.1415926535897932384626433832795
#endif

using namespace glm;

GLenum  cube[6] = 
{
	GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
	GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
	GL_TEXTURE_CUBE_MAP_POSITIVE_X,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_Z ,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
	GL_TEXTURE_CUBE_MAP_POSITIVE_Y
};

//Helper Functions
mat4 toMatrix4(float *in)
{
	mat4 outMatrix(in[0],in[1],in[2],in[3],
					in[4],in[5],in[6],in[7],
					in[8],in[9],in[10],in[11],
					in[12],in[13],in[14],in[15]);
	return outMatrix;
}

void checkFramebufferStatus(GLenum framebufferStatus) {
	switch (framebufferStatus) {
        case GL_FRAMEBUFFER_COMPLETE_EXT: break;
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT:
            printf("Attachment Point Unconnected");
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT:
            printf("Missing Attachment");
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
            printf("Dimensions do not match");
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:
            printf("Formats");
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT:
            printf("Draw Buffer");
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT:
            printf("Read Buffer");
            break;
        case GL_FRAMEBUFFER_UNSUPPORTED_EXT:
            printf("Unsupported Framebuffer Configuration");
            break;
        default:
            printf("Unkown Framebuffer Object Failure");
            break;
    }
}

ParticleRenderer::ParticleRenderer()
: m_pos(0),
  m_numParticles(0),
  m_pointSize(1.0f),
  m_particleRadius(0.125f * 0.5f),
  m_display_type(DISPLAY_TOTAL),
  m_program(0),
  m_depth_pass_program(0),
  m_normal_pass_program(0),
  m_vbo(0),
  m_colorVBO(0),
  m_projection(0),
  m_modelView(0),
  m_NEARP(0),
  m_FARP(0),
  m_depthTexture(0),
  m_normalTexture(0),
  m_colorTexture(0),
  m_positionTexture(0),
  m_FBO(0),
  m_normalsFBO(0)
{
    _initGL();
}

ParticleRenderer::ParticleRenderer(int w, int h)
: m_pos(0),
  m_numParticles(0),
  m_pointSize(1.0f),
  m_particleRadius(0.125f * 0.5f),
  m_display_type(DISPLAY_TOTAL),
  m_window_h(h),
  m_window_w(w),
  m_program(0),
  m_depth_pass_program(0),
  m_normal_pass_program(0),
  m_vbo(0),
  m_colorVBO(0),
  m_projection(0),
  m_NEARP(0),
  m_FARP(0),
  m_modelView(0),
  m_depthTexture(0),
  m_normalTexture(0),
  m_colorTexture(0),
  m_positionTexture(0),
  m_FBO(0),
  m_normalsFBO(0)
{
	_initGL();
}

ParticleRenderer::~ParticleRenderer()
{
    m_pos = 0;
}

void ParticleRenderer::setPositions(float *pos, int numParticles)
{
    m_pos = pos;
    m_numParticles = numParticles;
}

void ParticleRenderer::setVertexBuffer(unsigned int vbo, int numParticles)
{
    m_vbo = vbo;
    m_numParticles = numParticles;
}

void 
ParticleRenderer::setModelViewMatrix(float * m)
{
	m_modelView = toMatrix4(m);
}

void
ParticleRenderer::setProjectionMatrix(float * p)
{
	m_projection = toMatrix4(p);
}

void ParticleRenderer::_drawPoints()
{
        glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_vbo);
        glVertexPointer(4, GL_FLOAT, 0, 0);
        glEnableClientState(GL_VERTEX_ARRAY);                

        glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_colorVBO);
        glColorPointer(4, GL_FLOAT, 0, 0);
		glEnableClientState(GL_COLOR_ARRAY);

        glDrawArrays(GL_POINTS, 0, m_numParticles);

        glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
        glDisableClientState(GL_VERTEX_ARRAY); 
        glDisableClientState(GL_COLOR_ARRAY); 
}

bool ParticleRenderer::loadCubeMapTexture(char *filename)
{
	/*if(m_cubeTexData)
	{
		free(m_cubeTexData);
		m_cubeTexData = NULL;
	}*/
	
	//	Get Image Info
	ILuint nCurrTexImg = 0;
	ilGenImages(1, &nCurrTexImg);
	ilBindImage(nCurrTexImg);	

	if(ilLoadImage(filename))
	{
		int nWidth  = ilGetInteger(IL_IMAGE_WIDTH);
		int nHeight = ilGetInteger(IL_IMAGE_HEIGHT);
		
		m_faceDim = abs(nWidth - nHeight);
		int nDataOffset = m_faceDim * m_faceDim * 3;

		m_cubeTexData = (unsigned char *)malloc(sizeof(unsigned char) * 6 * m_faceDim * m_faceDim  * 3);

		ilCopyPixels(           0, m_faceDim    , 0, m_faceDim, m_faceDim, 1, IL_RGB, IL_UNSIGNED_BYTE, m_cubeTexData);		
		ilCopyPixels(m_faceDim    , m_faceDim    , 0, m_faceDim, m_faceDim, 1, IL_RGB, IL_UNSIGNED_BYTE, m_cubeTexData + nDataOffset);		
		ilCopyPixels(m_faceDim * 2, m_faceDim    , 0, m_faceDim, m_faceDim, 1, IL_RGB, IL_UNSIGNED_BYTE, m_cubeTexData + nDataOffset * 2);	
		ilCopyPixels(m_faceDim * 3, m_faceDim    , 0, m_faceDim, m_faceDim, 1, IL_RGB, IL_UNSIGNED_BYTE, m_cubeTexData + nDataOffset * 3);				
		ilCopyPixels(m_faceDim    , m_faceDim * 2    , 0, m_faceDim, m_faceDim, 1, IL_RGB, IL_UNSIGNED_BYTE, m_cubeTexData + nDataOffset * 4);				
		ilCopyPixels(m_faceDim    , 0, 0, m_faceDim, m_faceDim, 1, IL_RGB, IL_UNSIGNED_BYTE, m_cubeTexData + nDataOffset * 5);				
		
		ilDeleteImages(1, &nCurrTexImg);

		//
		glEnable(GL_TEXTURE_2D);
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER,0);
		glGenTextures(1, &m_cubeMapTexture);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);	

		glBindTexture(GL_TEXTURE_CUBE_MAP, m_cubeMapTexture);	
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);	// Set far filtering mode
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);   

		for (int i = 0; i < 6; i ++) 
		{ 
			glTexImage2D
			(	
				cube[i], 
				0,                  //level 
				3,					//internal format 
				m_faceDim,           //width 
				m_faceDim,           //height 
				0,                  //border 
				GL_RGB,             //format 
				GL_UNSIGNED_BYTE,   //type 
				m_cubeTexData + i * m_faceDim * m_faceDim * 3
			); // pixel data 
		}

		glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
		glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
		glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);

		return true;
	}

	ILenum ilErr = ilGetError();
	printf("Error in LoadImage: %d [%s]\n", ilErr, filename);

	ilDeleteImages(1, &nCurrTexImg);
	return false;
}

void ParticleRenderer::loadSkyBoxTexture(char *filename)
{

	ILuint nCurrTexImg = 0;
	ilGenImages(1, &nCurrTexImg);
	ilBindImage(nCurrTexImg);	

	if(ilLoadImage(filename))
	{
		int nWidth  = ilGetInteger(IL_IMAGE_WIDTH);
		int nHeight = ilGetInteger(IL_IMAGE_HEIGHT);
		
		m_faceDim = abs(nWidth - nHeight);
		int nDataOffset = m_faceDim * m_faceDim * 3;
		
		{
			m_skyTexData = (unsigned char **)malloc(sizeof(unsigned char *) * 6);

			m_skyTexData[0] = (unsigned char *)malloc(sizeof(unsigned char) * m_faceDim * m_faceDim  * 3);
			ilCopyPixels(           0, m_faceDim    , 0, m_faceDim, m_faceDim, 1, IL_RGB, IL_UNSIGNED_BYTE, m_skyTexData[0]);		

			m_skyTexData[1] = (unsigned char *)malloc(sizeof(unsigned char) * m_faceDim * m_faceDim  * 3);
			ilCopyPixels(m_faceDim    , m_faceDim    , 0, m_faceDim, m_faceDim, 1, IL_RGB, IL_UNSIGNED_BYTE, m_skyTexData[1]);

			m_skyTexData[2] = (unsigned char *)malloc(sizeof(unsigned char) * m_faceDim * m_faceDim  * 3);
			ilCopyPixels(m_faceDim * 2, m_faceDim    , 0, m_faceDim, m_faceDim, 1, IL_RGB, IL_UNSIGNED_BYTE, m_skyTexData[2]);	

			m_skyTexData[3] = (unsigned char *)malloc(sizeof(unsigned char) * m_faceDim * m_faceDim  * 3);
			ilCopyPixels(m_faceDim * 3, m_faceDim    , 0, m_faceDim, m_faceDim, 1, IL_RGB, IL_UNSIGNED_BYTE, m_skyTexData[3]);		

			m_skyTexData[4] = (unsigned char *)malloc(sizeof(unsigned char) * m_faceDim * m_faceDim  * 3);
			ilCopyPixels(m_faceDim    , m_faceDim * 2, 0, m_faceDim, m_faceDim, 1, IL_RGB, IL_UNSIGNED_BYTE, m_skyTexData[4]);				

			m_skyTexData[5] = (unsigned char *)malloc(sizeof(unsigned char) * m_faceDim * m_faceDim  * 3);
			ilCopyPixels(m_faceDim    , 0, 0, m_faceDim, m_faceDim, 1, IL_RGB, IL_UNSIGNED_BYTE, m_skyTexData[5]);				
		}

		{
			glEnable(GL_TEXTURE_2D);

			glActiveTexture(GL_TEXTURE0);

			m_skyBoxTexture =  (GLuint *)malloc(sizeof(GLuint) * 6);

			for(int i = 0; i < 6; i ++)
			{
				glGenTextures(1, &m_skyBoxTexture[i]);
				glPixelStorei(GL_UNPACK_ALIGNMENT, 1);		
				glBindBuffer(GL_PIXEL_UNPACK_BUFFER,0);

				glBindTexture(GL_TEXTURE_2D, m_skyBoxTexture[i]);	

				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);	// Set far filtering mode
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);   
				glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

				glTexImage2D
				(	
					GL_TEXTURE_2D, 
					0,                  //level 
					3,					//internal format 
					m_faceDim,           //width 
					m_faceDim,           //height 
					0,                  //border 
					GL_RGB,             //format 
					GL_UNSIGNED_BYTE,   //type 
					m_skyTexData[i]
				); // pixel data 
			
			}

		}

		ilDeleteImages(1, &nCurrTexImg);
	}
}

void ParticleRenderer::renderSkyBox()
{
	float fSkyFaceDim = 800;

	glEnable(GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE0);
	
	//	Ground
	glBindTexture(GL_TEXTURE_2D, m_skyBoxTexture[4]);
	float groundCtr[3] = {m_CamX, m_CamY - fSkyFaceDim / 2.f, m_CamZ};
	glBegin(GL_QUADS);
		glTexCoord2d(0, 0);	glVertex3f(groundCtr[0] - fSkyFaceDim / 2.f, groundCtr[1], groundCtr[2] - fSkyFaceDim / 2.f);
		glTexCoord2d(0, 1);	glVertex3f(groundCtr[0] - fSkyFaceDim / 2.f, groundCtr[1], groundCtr[2] + fSkyFaceDim / 2.f);
		glTexCoord2d(1, 1);	glVertex3f(groundCtr[0] + fSkyFaceDim / 2.f, groundCtr[1], groundCtr[2] + fSkyFaceDim / 2.f);
		glTexCoord2d(1, 0);	glVertex3f(groundCtr[0] + fSkyFaceDim / 2.f, groundCtr[1], groundCtr[2] - fSkyFaceDim / 2.f);
	glEnd();

	//	Ceiling
	glBindTexture(GL_TEXTURE_2D, m_skyBoxTexture[5]);
	float ceilingCtr[3] = {m_CamX, m_CamY + fSkyFaceDim / 2.f, m_CamZ};
	glBegin(GL_QUADS);
		glTexCoord2d(0, 1);	glVertex3f(ceilingCtr[0] - fSkyFaceDim / 2.f, ceilingCtr[1], ceilingCtr[2] - fSkyFaceDim / 2.f);
		glTexCoord2d(0, 0);	glVertex3f(ceilingCtr[0] - fSkyFaceDim / 2.f, ceilingCtr[1], ceilingCtr[2] + fSkyFaceDim / 2.f);
		glTexCoord2d(1, 0);	glVertex3f(ceilingCtr[0] + fSkyFaceDim / 2.f, ceilingCtr[1], ceilingCtr[2] + fSkyFaceDim / 2.f);
		glTexCoord2d(1, 1);	glVertex3f(ceilingCtr[0] + fSkyFaceDim / 2.f, ceilingCtr[1], ceilingCtr[2] - fSkyFaceDim / 2.f);
	glEnd();

	//	X-
	glBindTexture(GL_TEXTURE_2D, m_skyBoxTexture[0]);
	float x_m_ctr[3] = {m_CamX - fSkyFaceDim / 2.f, m_CamY, m_CamZ};
	glBegin(GL_QUADS);
		glTexCoord2d(1, 1);	glVertex3f(x_m_ctr[0], x_m_ctr[1] - fSkyFaceDim / 2.f, x_m_ctr[2] - fSkyFaceDim / 2.f);
		glTexCoord2d(1, 0);	glVertex3f(x_m_ctr[0], x_m_ctr[1] + fSkyFaceDim / 2.f, x_m_ctr[2] - fSkyFaceDim / 2.f);
		glTexCoord2d(0, 0);	glVertex3f(x_m_ctr[0], x_m_ctr[1] + fSkyFaceDim / 2.f, x_m_ctr[2] + fSkyFaceDim / 2.f);
		glTexCoord2d(0, 1);	glVertex3f(x_m_ctr[0], x_m_ctr[1] - fSkyFaceDim / 2.f, x_m_ctr[2] + fSkyFaceDim / 2.f);
	glEnd();

	//	X+
	glBindTexture(GL_TEXTURE_2D, m_skyBoxTexture[2]);
	float x2_m_ctr[3] = {m_CamX + fSkyFaceDim / 2.f, m_CamY, m_CamZ};
	glBegin(GL_QUADS);
		glTexCoord2d(0, 1);	glVertex3f(x2_m_ctr[0], x2_m_ctr[1] - fSkyFaceDim / 2.f, x2_m_ctr[2] - fSkyFaceDim / 2.f);
		glTexCoord2d(0, 0);	glVertex3f(x2_m_ctr[0], x2_m_ctr[1] + fSkyFaceDim / 2.f, x2_m_ctr[2] - fSkyFaceDim / 2.f);
		glTexCoord2d(1, 0);	glVertex3f(x2_m_ctr[0], x2_m_ctr[1] + fSkyFaceDim / 2.f, x2_m_ctr[2] + fSkyFaceDim / 2.f);
		glTexCoord2d(1, 1);	glVertex3f(x2_m_ctr[0], x2_m_ctr[1] - fSkyFaceDim / 2.f, x2_m_ctr[2] + fSkyFaceDim / 2.f);
	glEnd();

	//	-Z
	glBindTexture(GL_TEXTURE_2D, m_skyBoxTexture[1]);
	float z_m_ctr[3] = { m_CamX, m_CamY, m_CamZ - fSkyFaceDim / 2.f};
	glBegin(GL_QUADS);
		glTexCoord2d(0, 1);	glVertex3f(z_m_ctr[0] - fSkyFaceDim / 2.f, z_m_ctr[1] - fSkyFaceDim / 2.f, z_m_ctr[2]);
		glTexCoord2d(1, 1);	glVertex3f(z_m_ctr[0] + fSkyFaceDim / 2.f, z_m_ctr[1] - fSkyFaceDim / 2.f, z_m_ctr[2]);
		glTexCoord2d(1, 0);	glVertex3f(z_m_ctr[0] + fSkyFaceDim / 2.f, z_m_ctr[1] + fSkyFaceDim / 2.f, z_m_ctr[2]);
		glTexCoord2d(0, 0);	glVertex3f(z_m_ctr[0] - fSkyFaceDim / 2.f, z_m_ctr[1] + fSkyFaceDim / 2.f, z_m_ctr[2]);
	glEnd();

	//	+Z
	glBindTexture(GL_TEXTURE_2D, m_skyBoxTexture[3]);
	float z2_m_ctr[3] = { m_CamX, m_CamY, m_CamZ + fSkyFaceDim / 2.f};
	glBegin(GL_QUADS);
		glTexCoord2d(1, 1);	glVertex3f(z2_m_ctr[0] - fSkyFaceDim / 2.f, z2_m_ctr[1] - fSkyFaceDim / 2.f, z2_m_ctr[2]);
		glTexCoord2d(0, 1);	glVertex3f(z2_m_ctr[0] + fSkyFaceDim / 2.f, z2_m_ctr[1] - fSkyFaceDim / 2.f, z2_m_ctr[2]);
		glTexCoord2d(0, 0);	glVertex3f(z2_m_ctr[0] + fSkyFaceDim / 2.f, z2_m_ctr[1] + fSkyFaceDim / 2.f, z2_m_ctr[2]);
		glTexCoord2d(1, 0);	glVertex3f(z2_m_ctr[0] - fSkyFaceDim / 2.f, z2_m_ctr[1] + fSkyFaceDim / 2.f, z2_m_ctr[2]);
	glEnd();
}

void ParticleRenderer::display()
{

	mat4 inverse_transposed = inverse(m_modelView);

	//Render Skybox To Color Texture
	_bindFBO(m_backgroundFBO);
	glClearColor(1, 1, 1, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	glUseProgram(0);
	renderSkyBox();
	
	//Render Attributes to Texture
	_setTextures();
	_bindFBO(m_FBO);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	//Draw Particles
	glEnable(GL_POINT_SPRITE_ARB);
    glTexEnvi(GL_POINT_SPRITE_ARB, GL_COORD_REPLACE_ARB, GL_TRUE);
    glEnable(GL_VERTEX_PROGRAM_POINT_SIZE_NV);
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);

    glUseProgram(m_depth_pass_program);
    glUniform1f( glGetUniformLocation(m_depth_pass_program, "pointScale"), m_window_h / tanf(m_fov*0.5f*(float)M_PI/180.0f) );
    glUniform1f( glGetUniformLocation(m_depth_pass_program, "pointRadius"), m_particleRadius );
	glUniform1f( glGetUniformLocation(m_depth_pass_program, "u_Far"), m_FARP );
	glUniform1f( glGetUniformLocation(m_depth_pass_program, "u_Near"), m_NEARP );
	glUniformMatrix4fv(glGetUniformLocation(m_depth_pass_program,"u_ModelView"),1,GL_FALSE,&m_modelView[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(m_depth_pass_program,"u_Persp"),1,GL_FALSE,&m_projection[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(m_depth_pass_program,"u_InvTrans"),1,GL_FALSE,&inverse_transposed[0][0]);
    
	glColor3f(1, 1, 1);
    _drawPoints();
    glDisable(GL_POINT_SPRITE_ARB);

	//Create Fluid Thickness Map
	_setTextures();
	_bindFBO(m_thickFBO);
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	glEnable(GL_POINT_SPRITE_ARB);
    glTexEnvi(GL_POINT_SPRITE_ARB, GL_COORD_REPLACE_ARB, GL_TRUE);
    glEnable(GL_VERTEX_PROGRAM_POINT_SIZE_NV);
    //glDepthMask(GL_FALSE);
	glEnable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glBlendFunc(GL_ONE,GL_ONE);


	glUseProgram(m_thick_pass_program);
    glUniform1f( glGetUniformLocation(m_thick_pass_program, "pointScale"), m_window_h / tanf(m_fov*0.5f*(float)M_PI/180.0f) );
    glUniform1f( glGetUniformLocation(m_thick_pass_program, "pointRadius"), m_particleRadius );
	glUniform1f( glGetUniformLocation(m_thick_pass_program, "u_Far"), m_FARP );
	glUniform1f( glGetUniformLocation(m_thick_pass_program, "u_Near"), m_NEARP );
	glUniformMatrix4fv(glGetUniformLocation(m_thick_pass_program,"u_ModelView"),1,GL_FALSE,&m_modelView[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(m_thick_pass_program,"u_Persp"),1,GL_FALSE,&m_projection[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(m_thick_pass_program,"u_InvTrans"),1,GL_FALSE,&inverse_transposed[0][0]);
    
	glColor3f(1, 1, 1);
    _drawPoints();

	glDisable(GL_POINT_SPRITE_ARB);
	glDisable(GL_BLEND);
	//glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);

	//Blur Depth Texture
	_setTextures();
	_bindFBO(m_blurDepthFBO);

	glUseProgram(m_blur_pass_program);
	glBindVertexArray(m_device_quad.vertex_array);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_device_quad.vbo_indices);
	
	glEnable(GL_TEXTURE_2D);

	glActiveTexture(GL_TEXTURE11);
	glBindTexture(GL_TEXTURE_2D, m_depthTexture);
	glUniform1i(glGetUniformLocation(m_blur_pass_program, "u_Depthtex"),11);
	glUniform1f( glGetUniformLocation(m_blur_pass_program, "u_Far"), m_FARP );
	glUniform1f( glGetUniformLocation(m_blur_pass_program, "u_Near"), m_NEARP );
	
	glDrawElements(GL_TRIANGLES, m_device_quad.num_indices, GL_UNSIGNED_SHORT,0);
	
	glBindVertexArray(0);

	//Write Normals to Texture from Depth Texture
	_setTextures();
	_bindFBO(m_normalsFBO);
	
	glUseProgram(m_normal_pass_program);
	glBindVertexArray(m_device_quad.vertex_array);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_device_quad.vbo_indices);

	glEnable(GL_TEXTURE_2D);

	glActiveTexture(GL_TEXTURE11);
	glBindTexture(GL_TEXTURE_2D, m_blurDepthTexture);
	glUniform1i(glGetUniformLocation(m_normal_pass_program, "u_Depthtex"),11);
	glActiveTexture(GL_TEXTURE12);
	glBindTexture(GL_TEXTURE_2D, m_positionTexture);
	glUniform1i(glGetUniformLocation(m_normal_pass_program, "u_Positiontex"),12);

	glUniform1f( glGetUniformLocation(m_normal_pass_program, "u_Far"), m_FARP );
	glUniform1f( glGetUniformLocation(m_normal_pass_program, "u_Near"), m_NEARP );
	glUniformMatrix4fv(glGetUniformLocation(m_normal_pass_program,"u_InvTrans"),1,GL_FALSE,&inverse_transposed[0][0]);
	mat4 inverse_projectiond = inverse(m_projection);
	glUniformMatrix4fv(glGetUniformLocation(m_normal_pass_program,"u_InvProj"),1,GL_FALSE,&inverse_projectiond[0][0]);

	glDrawElements(GL_TRIANGLES, m_device_quad.num_indices, GL_UNSIGNED_SHORT,0);
	
	glBindVertexArray(0);
	
	//Draw Full Screen Quad
	_setTextures();
	glUseProgram(m_program);
	glBindVertexArray(m_device_quad.vertex_array);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_device_quad.vbo_indices);
	
    glEnable(GL_TEXTURE_2D);

	glActiveTexture(GL_TEXTURE6);
	glBindTexture(GL_TEXTURE_2D, m_blurDepthTexture);
    glUniform1i(glGetUniformLocation(m_program, "u_Depthtex"),6);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_normalTexture);
    glUniform1i(glGetUniformLocation(m_program, "u_Normaltex"),1);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, m_colorTexture);
    glUniform1i(glGetUniformLocation(m_program, "u_Colortex"),2);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, m_positionTexture);
    glUniform1i(glGetUniformLocation(m_program, "u_Positiontex"),3);
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, m_backgroundTexture);
    glUniform1i(glGetUniformLocation(m_program, "u_Backgroundtex"),4);
	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_CUBE_MAP, m_cubeMapTexture);
    glUniform1i(glGetUniformLocation(m_program, "u_Cubemaptex"),5);
	glActiveTexture(GL_TEXTURE7);
	glBindTexture(GL_TEXTURE_2D, m_thickTexture);
    glUniform1i(glGetUniformLocation(m_program, "u_Thicktex"),7);
	
	glUniformMatrix4fv(glGetUniformLocation(m_program,"u_ModelView"),1,GL_FALSE,&m_modelView[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(m_program,"u_Persp"),1,GL_FALSE,&m_projection[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(m_program,"u_InvTrans"),1,GL_FALSE, &inverse_transposed[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(m_program,"u_InvProj"),1,GL_FALSE,&inverse_projectiond[0][0]);

	glUniform1f( glGetUniformLocation(m_program, "u_Far"), m_FARP);
	glUniform1f( glGetUniformLocation(m_program, "u_Near"), m_NEARP);
	glUniform1i( glGetUniformLocation(m_program, "u_DisplayType"), m_display_type);
	
	glDrawElements(GL_TRIANGLES, m_device_quad.num_indices, GL_UNSIGNED_SHORT,0);
    

	glBindVertexArray(0);
}

void ParticleRenderer::_initNormalPassProgram() {
	m_normal_pass_program = glCreateProgram();
	glBindAttribLocation(m_normal_pass_program, quad_attributes::POSITION, "Position");
	glBindAttribLocation(m_normal_pass_program, quad_attributes::TEXCOORD, "Texcoord");
	Utility::attachAndLinkProgram(m_normal_pass_program, Utility::loadShaders("../media/normal_pass.vert","../media/normal_pass.frag"));
}

void ParticleRenderer::_initDepthPassProgram() {
	m_depth_pass_program = glCreateProgram();
	glBindAttribLocation(m_depth_pass_program, particle_attributes::POSITION, "Position");
	glBindAttribLocation(m_depth_pass_program, particle_attributes::COLOR, "Color");
	Utility::attachAndLinkProgram(m_depth_pass_program, Utility::loadShaders("../media/depth_pass.vert","../media/depth_pass.frag"));
}

void ParticleRenderer::_initShaderProgram() {
	m_program = glCreateProgram();
	glBindAttribLocation(m_program, quad_attributes::POSITION, "Position");
	glBindAttribLocation(m_program,quad_attributes::TEXCOORD, "Texcoord");
	Utility::attachAndLinkProgram(m_program, Utility::loadShaders("../media/shader.vert","../media/shader.frag"));
}

void ParticleRenderer::_initBlurPassProgram() {
	m_blur_pass_program = glCreateProgram();
	glBindAttribLocation(m_blur_pass_program, quad_attributes::POSITION, "Position");
	glBindAttribLocation(m_blur_pass_program, quad_attributes::TEXCOORD, "Texcoord");
	Utility::attachAndLinkProgram(m_blur_pass_program, Utility::loadShaders("../media/blur_pass.vert","../media/blur_pass.frag"));
}

void ParticleRenderer::_initThickPassProgram() {
	m_thick_pass_program = glCreateProgram();
	glBindAttribLocation(m_thick_pass_program, particle_attributes::POSITION, "Position");
	glBindAttribLocation(m_thick_pass_program, particle_attributes::COLOR, "Color");
	Utility::attachAndLinkProgram(m_thick_pass_program, Utility::loadShaders("../media/thick_pass.vert","../media/thick_pass.frag"));
}



void ParticleRenderer::_initGL()
{
	_initDepthPassProgram();
	_initNormalPassProgram();
	_initBlurPassProgram();
	_initThickPassProgram();
	_initShaderProgram();
	_initFBO(m_window_w,m_window_h);
	_initQuad();

}

void ParticleRenderer::_initFBO(int w, int h) {
    GLenum FBOstatus;
	
	glActiveTexture(GL_TEXTURE10);
	
	glGenTextures(1, &m_depthTexture);
	glGenTextures(1, &m_colorTexture);
    glGenTextures(1, &m_normalTexture);
	glGenTextures(1, &m_positionTexture);
	glGenTextures(1, &m_backgroundTexture);
	glGenTextures(1, &m_blurDepthTexture);
	glGenTextures(1, &m_thickTexture);

	//Depth Texture Initiaialization
	glBindTexture(GL_TEXTURE_2D, m_depthTexture);
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_INTENSITY);

	glTexImage2D( GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, w, h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);

	//Blurred Depth Texture
	glBindTexture(GL_TEXTURE_2D, m_blurDepthTexture);
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB32F, w, h, 0, GL_RGBA, GL_FLOAT, 0);
	
	//Blurred Thick  Texture
	glBindTexture(GL_TEXTURE_2D, m_thickTexture);
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB32F, w, h, 0, GL_RGBA, GL_FLOAT, 0);

	//Normal Texture Initialization
	glBindTexture(GL_TEXTURE_2D, m_normalTexture);
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB32F , w, h, 0, GL_RGBA, GL_FLOAT,0);

	//Position Texture Initialization
	glBindTexture(GL_TEXTURE_2D, m_positionTexture);
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB32F , w, h, 0, GL_RGBA, GL_FLOAT,0);

	//Color Texture Initialization
	glBindTexture(GL_TEXTURE_2D, m_colorTexture);
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB32F , w, h, 0, GL_RGBA, GL_FLOAT, 0);

	//Background Texture Initialization
	glBindTexture(GL_TEXTURE_2D, m_backgroundTexture);
	
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB , w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	
	glBindTexture(GL_TEXTURE_2D, 0);
	
	m_backgroundTexData = (unsigned char *)malloc(sizeof(unsigned char) * 4 * w * h);
	
	glGenFramebuffers(1, &m_FBO);
	glGenFramebuffers(1, &m_normalsFBO);
	glGenFramebuffers(1, &m_backgroundFBO);
	glGenFramebuffers(1, &m_blurDepthFBO);
	glGenFramebuffers(1, &m_thickFBO);
	
	//Create First Framebuffer Object
	glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
	
	// Instruct openGL that we won't bind a color texture with the currently binded FBO
	glReadBuffer(GL_NONE);
    GLint position_loc = glGetFragDataLocation(m_depth_pass_program,"out_Position");
	GLint color_loc = glGetFragDataLocation(m_depth_pass_program,"out_Color");
    GLenum draws [2];
    draws[position_loc] = GL_COLOR_ATTACHMENT1;
    draws[color_loc] = GL_COLOR_ATTACHMENT0;

	glDrawBuffers(2, draws);
	
	// attach the texture to FBO depth attachment point
    int test = GL_COLOR_ATTACHMENT0;
	glBindTexture(GL_TEXTURE_2D, m_depthTexture);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_depthTexture, 0);
	glBindTexture(GL_TEXTURE_2D, m_positionTexture);    
	glFramebufferTexture(GL_FRAMEBUFFER, draws[position_loc], m_positionTexture, 0);
	glBindTexture(GL_TEXTURE_2D, m_colorTexture);    
	glFramebufferTexture(GL_FRAMEBUFFER, draws[color_loc], m_colorTexture, 0);

	// check FBO status
	FBOstatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if(FBOstatus != GL_FRAMEBUFFER_COMPLETE) {
		printf("GL_FRAMEBUFFER_COMPLETE failed, CANNOT use FBO\n");
        checkFramebufferStatus(FBOstatus);
	}

	//Create Normals FBO (FBO to store Normals Data)
	glBindFramebuffer(GL_FRAMEBUFFER, m_normalsFBO);
	glReadBuffer(GL_NONE);
	GLint normal_loc = glGetFragDataLocation(m_normal_pass_program,"out_Normal");
	draws[normal_loc] = GL_COLOR_ATTACHMENT0;
	glDrawBuffers(1, draws);
	glBindTexture(GL_TEXTURE_2D, m_normalTexture);    
	glFramebufferTexture(GL_FRAMEBUFFER, draws[normal_loc], m_normalTexture, 0);

	// check FBO status
	FBOstatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if(FBOstatus != GL_FRAMEBUFFER_COMPLETE) {
		printf("GL_FRAMEBUFFER_COMPLETE failed, CANNOT use FBO\n");
        checkFramebufferStatus(FBOstatus);
	}

	//Create Background FBO (FBO to store Background Data)
	glBindFramebuffer(GL_FRAMEBUFFER, m_backgroundFBO);
	glReadBuffer(GL_NONE);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	glBindTexture(GL_TEXTURE_2D, m_backgroundTexture);    
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_backgroundTexture, 0);

	// check FBO status
	FBOstatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if(FBOstatus != GL_FRAMEBUFFER_COMPLETE) {
		printf("GL_FRAMEBUFFER_COMPLETE failed, CANNOT use FBO\n");
        checkFramebufferStatus(FBOstatus);
	}

	//Create Blurred Depth FBO (FBO to store Blurred Depth Data)
	glBindFramebuffer(GL_FRAMEBUFFER, m_blurDepthFBO);
	glReadBuffer(GL_NONE);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	glBindTexture(GL_TEXTURE_2D, m_blurDepthTexture);    
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_blurDepthTexture, 0);

	// check FBO status
	FBOstatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if(FBOstatus != GL_FRAMEBUFFER_COMPLETE) {
		printf("GL_FRAMEBUFFER_COMPLETE failed, CANNOT use FBO\n");
        checkFramebufferStatus(FBOstatus);
	}

	//Create Thickness FBO (FBO to store Thickness Data)
	glBindFramebuffer(GL_FRAMEBUFFER, m_thickFBO);
	glReadBuffer(GL_NONE);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	glBindTexture(GL_TEXTURE_2D, m_thickTexture);    
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_thickTexture, 0);

	// check FBO status
	FBOstatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if(FBOstatus != GL_FRAMEBUFFER_COMPLETE) {
		printf("GL_FRAMEBUFFER_COMPLETE failed, CANNOT use FBO\n");
        checkFramebufferStatus(FBOstatus);
	}

	// switch back to window-system-provided framebuffer
	glClear(GL_DEPTH_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void ParticleRenderer::_bindFBO(GLuint FBO) {
    glDisable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D,0); //Bad mojo to unbind the framebuffer using the texture
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);
    glClear(GL_DEPTH_BUFFER_BIT);
    //glColorMask(false,false,false,false);
    glEnable(GL_DEPTH_TEST);
}

void ParticleRenderer::_initQuad() {
	vertex2_t verts [] = { {vec3(-1,1,0),vec2(0,1)},
	{vec3(-1,-1,0),vec2(0,0)},
	{vec3(1,-1,0),vec2(1,0)},
	{vec3(1,1,0),vec2(1,1)}};

	unsigned short indices[] = { 0,1,2,0,2,3};

	//Allocate vertex array
	//Vertex arrays encapsulate a set of generic vertex attributes and the buffers they are bound too
	//Different vertex array per mesh.
	glGenVertexArrays(1, &(m_device_quad.vertex_array));
    glBindVertexArray(m_device_quad.vertex_array);

    
	//Allocate vbos for data
	glGenBuffers(1,&(m_device_quad.vbo_data));
	glGenBuffers(1,&(m_device_quad.vbo_indices));
    
	//Upload vertex data
	glBindBuffer(GL_ARRAY_BUFFER, m_device_quad.vbo_data);
	glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    //Use of strided data, Array of Structures instead of Structures of Arrays
	glVertexAttribPointer(quad_attributes::POSITION, 3, GL_FLOAT, GL_FALSE,sizeof(vertex2_t),0);
	glVertexAttribPointer(quad_attributes::TEXCOORD, 2, GL_FLOAT, GL_FALSE,sizeof(vertex2_t),(void*)sizeof(vec3));
	glEnableVertexAttribArray(quad_attributes::POSITION);
	glEnableVertexAttribArray(quad_attributes::TEXCOORD);

    //indices
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_device_quad.vbo_indices);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6*sizeof(GLushort), indices, GL_STATIC_DRAW);
    m_device_quad.num_indices = 6;
	//Unplug Vertex Array
    glBindVertexArray(0);
}

void ParticleRenderer::_freeFBO() {
	glDeleteTextures(1,&m_depthTexture);
    glDeleteTextures(1,&m_normalTexture);
	glDeleteTextures(1,&m_positionTexture);
	glDeleteTextures(1,&m_colorTexture);
    glDeleteFramebuffers(1,&m_FBO);
}

void ParticleRenderer::_setTextures() {
    glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D,0); 
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glColorMask(true,true,true,true);
    glDisable(GL_DEPTH_TEST);
	glClear(GL_COLOR_BUFFER_BIT);
}