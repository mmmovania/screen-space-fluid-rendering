/*
Terry Kaleas
CIS497 Physically Based Phase Changing Materials
*/

// Graphics includes
#include <GL/glew.h>
#if defined (_WIN32)
	#include <GL/wglew.h>
#endif

#ifndef M_PI
#define M_PI    3.1415926535897932384626433832795
#endif

// Utilities and system includes
#include <math.h>
#include <stdio.h>
#include <assert.h>
#include <iostream>
#include <algorithm>
#include "paramgl.h"
#include "fps.h"
#include <IL/il.h>
#include <IL/ilu.h>
#include <IL/ilut.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_projection.hpp>
#include <glm/gtc/matrix_operation.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/transform2.hpp>

#include "render_particles.h"
#include "fluid_system.h"

#define DEBUG_MATRIX


//FPS
mmc::FpsTracker theFpsTracker;

const int binIdx = 0;   // choose the proper sReferenceBin

struct uint3
{
  unsigned int x, y, z;
};

typedef unsigned int uint;

using namespace std;

//Screen Resolution
const uint width = 640, height = 480;
const float NEARP = 1.0f;
const float FARP = 400.0f;

// View Parameters
int ox, oy;
int buttonState = 0;
float camera_trans[] = {0, 0, -75.0f};
float camera_rot[]   = {0, 0, 0};
float camera_trans_lag[] = {0, 0, -3};
float camera_rot_lag[] = {0, 0, 0};
const float inertia = 1.0;

ParticleRenderer *renderer = 0;

FluidSystem fluidSystem;

//Display Info
int mode = 0;
bool bPause = true;
bool displaySliders = false;

//Recording Param
bool bRecording = false;
unsigned int frameNum = 0;
unsigned int screenShotNum = 0;

//View Mode or Move Mode
uint numParticles = 0;
uint3 gridSize;
int numIterations = 0; // Run Until Exit

// Simulation Parameters
float timestep = 0.001;
int max_particles = 20000;
float viscocity = 0.5f;
float pRadius = 0.0003f;
float gravity = 0.0003f;
float surfTension = 0.003f;

//Rendering Parameters
float modelView[16];
float projection[16];

ParamListGL *params;

#define MAX(a,b) ((a > b) ? a : b)

// initialize particle system
void initParticleSystem(int numParticles, uint3 gridSize, bool bUseOpenGL)
{
    if (bUseOpenGL) {
		
		//Initialize Particle System
		fluidSystem.Initialize(BFLUID, numParticles);
		fluidSystem.SPH_CreateExample( 0, numParticles);
		
		//Initialize Renderer
		renderer = new ParticleRenderer(width,height);
		renderer->setParticleRadius(1.0f);
		renderer->setVertexBuffer(fluidSystem.getPositionVBO(), numParticles);
        renderer->setColorBuffer(fluidSystem.getColorVBO());
		renderer->loadSkyBoxTexture("../media/violentdays_large.jpg");
		renderer->loadCubeMapTexture("../media/violentdays_large.jpg");
    }
}

void cleanup()
{

}

// initialize OpenGL
void initGL(int *argc, char **argv)
{  
    glutInit(argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowSize(width, height);
    glutCreateWindow("MultiPhase SPH Simulation");

    glewInit();
    if (!glewIsSupported("GL_VERSION_2_0 GL_VERSION_1_5 GL_ARB_multitexture GL_ARB_vertex_buffer_object")) {
        fprintf(stderr, "Required OpenGL extensions missing.");
        exit(-1);
    }

#if defined (_WIN32)
    if (wglewIsSupported("WGL_EXT_swap_control")) {
        // disable vertical sync
        wglSwapIntervalEXT(0);
    }
#endif

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	
	glEnable(GL_BLEND);
    glEnable(GL_ALPHA_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glShadeModel(GL_SMOOTH);

    glEnable(GL_NORMALIZE);
    glDisable(GL_LIGHTING);
    glCullFace(GL_BACK);

    glutReportErrors();
}

void drawOverlay()
{
	//Draw Sliders
	if (displaySliders) {
        glDisable(GL_DEPTH_TEST);
        glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ZERO); // invert color
        glEnable(GL_BLEND);
        params->Render(0, 0);
        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
	}
  
	// Draw Text Overlay
	 glColor4f(1.0, 1.0, 1.0, 1.0);
	 glPushAttrib(GL_LIGHTING_BIT);
     glDisable(GL_LIGHTING);

     glMatrixMode(GL_PROJECTION);
	 glLoadIdentity();
     gluOrtho2D(0.0, 1.0, 0.0, 1.0);

     glMatrixMode(GL_MODELVIEW);
     glLoadIdentity();
     glRasterPos2f(-0.95,-0.95);

	 char * drawMode;
	 switch((int)renderer->getDisplayMode()){
		 
		case DISPLAY_DEPTH:
			 drawMode = "Drawing Depth";
			 break;
		case DISPLAY_NORMAL:
			 drawMode = "Drawing Normal";
			 break;
		case DISPLAY_POSITION:
			drawMode = "Drawing Position";
			 break;
		case DISPLAY_COLOR:
			drawMode = "Drawing Color";
			 break;
		case DISPLAY_DIFFUSE:
			drawMode = "Drawing Diffuse";
			 break;
		case DISPLAY_DIFFUSE_SPEC:
			drawMode = "Drawing Diffuse + Spec";
			 break;
		case DISPLAY_FRESNEL:
			drawMode = "Drawing Fresnel";
			 break;
		case DISPLAY_REFLECTION:
			drawMode = "Drawing Reflection";
			 break;
		case DISPLAY_FRES_REFL:
			drawMode = "Drawing Fresnel * Reflection";
			 break;
		case DISPLAY_THICKNESS:
			drawMode = "Drawing Thickness";
			 break;
		case DISPLAY_REFRAC:
			drawMode = "Drawing Refraction";
			break;
		case DISPLAY_TOTAL:
		default:
			 drawMode = "Drawing Total";
			 break;
	 }
     
     char info[1024];
     sprintf(info, "%s \t Framerate: %3.1f \t %s %d",
		 drawMode,
         theFpsTracker.fpsAverage(),
		 bRecording ? "Recording..." : "",
		 frameNum);
 
     for (unsigned int i = 0; i < strlen(info); i++)
     {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, info[i]);
     }

  glPopAttrib();
}

void grabScreen(char * anim_filename)  
{
    unsigned int image;
    ilGenImages(1, &image);
    ilBindImage(image);

    ILenum error = ilGetError();
    assert(error == IL_NO_ERROR);

    ilTexImage(640, 480, 1, 3, IL_RGB, IL_UNSIGNED_BYTE, NULL);

    error = ilGetError();
    assert(error == IL_NO_ERROR);

    unsigned char* data = ilGetData();

    error = ilGetError();
    assert(error == IL_NO_ERROR);

    for (int i=479; i>=0; i--) 
    {
	    glReadPixels(0,i,640,1,GL_RGB, GL_UNSIGNED_BYTE, 
		    data + (640 * 3 * i));
    }

   //char anim_filename[2048];
   //sprintf_s(anim_filename, 2048, "../media/img/MULTIPHASE_%04d.png", frameNum++); 

    ilSave(IL_PNG, anim_filename);

    error = ilGetError();
    assert(error == IL_NO_ERROR);

    ilDeleteImages(1, &image);

    error = ilGetError();
    assert(error == IL_NO_ERROR);
}

void display()
{	
	//Time Track
	theFpsTracker.timestamp();
	
	if (!bPause) {
			//Update Fluid System Parameters 
			fluidSystem.SetParam(SPH_VISC,viscocity);
			fluidSystem.SetParam(SPH_TIMESTEP,timestep);
			
			//Update Fluid System
			fluidSystem.Run();

			//Record Image
			if(bRecording){
				char anim_filename[2048];
				sprintf_s(anim_filename, 2048, "../media/img/MULTIPHASE_%04d.png", frameNum++); 
				grabScreen(anim_filename);
			}
	}
    
	// clear the screen

    //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);  

    // view transform
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
    gluPerspective(60.0,(float)width/(float)height, 1.0, 1000.0);
    
	glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    for (int c = 0; c < 3; ++c)
    {
        camera_trans_lag[c] += (camera_trans[c] - camera_trans_lag[c]) * inertia;
        camera_rot_lag[c] += (camera_rot[c] - camera_rot_lag[c]) * inertia;
    }
    glTranslatef(camera_trans_lag[0], camera_trans_lag[1], camera_trans_lag[2]);
    glRotatef(camera_rot_lag[0], 1.0, 0.0, 0.0);
    glRotatef(camera_rot_lag[1], 0.0, 1.0, 0.0);

   

    // cube
    glColor3f(0.75, 1.0, 0.75);
	fluidSystem.SPH_DrawDomain();

	// show axis

	//render Particles
    
	if (renderer)
    {
		//Particle Renderer Display
		glGetFloatv(GL_MODELVIEW_MATRIX, modelView);
		glGetFloatv(GL_PROJECTION_MATRIX, projection);
		glm::mat4 persp = glm::perspective(60.0f,(float)width/(float)height,NEARP,FARP);
		renderer->setNearFarPlane(NEARP,FARP);
		renderer->setModelViewMatrix(modelView);
		renderer->setProjectionMatrix(persp);
		renderer->setWindowSize(width,height);
		renderer->setCamPos(camera_trans_lag[0],camera_trans_lag[1],camera_trans_lag[2]);
        renderer->display();
    }
	
	//Display HUD Info
	drawOverlay();

	glutSwapBuffers();
    glutReportErrors();
}

inline float frand()
{
    return rand() / (float) RAND_MAX;
}

void reshape(int w, int h)
{
    glMatrixMode(GL_PROJECTION);
    //glLoadIdentity();
    //gluPerspective(60.0, (float) w / (float) h, 1.0, 500.0);

    glMatrixMode(GL_MODELVIEW);
    glViewport(0, 0, w, h);

    if (renderer) {
        renderer->setWindowSize(w, h);
        renderer->setFOV(60.0);
    }
}

void mouse(int button, int state, int x, int y)
{
    int mods;

    if (state == GLUT_DOWN)
        buttonState |= 1<<button;
    else if (state == GLUT_UP)
        buttonState = 0;

    mods = glutGetModifiers();
    if (mods & GLUT_ACTIVE_SHIFT) {
        buttonState = 2;
    } else if (mods & GLUT_ACTIVE_CTRL) {
        buttonState = 3;
    }

    ox = x; oy = y;

    if (displaySliders) {
        if (params->Mouse(x, y, button, state)) {
            glutPostRedisplay();
            return;
        }
    }

    glutPostRedisplay();
}

// transfrom vector by matrix
void xform(float *v, float *r, GLfloat *m)
{
  r[0] = v[0]*m[0] + v[1]*m[4] + v[2]*m[8] + m[12];
  r[1] = v[0]*m[1] + v[1]*m[5] + v[2]*m[9] + m[13];
  r[2] = v[0]*m[2] + v[1]*m[6] + v[2]*m[10] + m[14];
}

// transform vector by transpose of matrix
void ixform(float *v, float *r, GLfloat *m)
{
  r[0] = v[0]*m[0] + v[1]*m[1] + v[2]*m[2];
  r[1] = v[0]*m[4] + v[1]*m[5] + v[2]*m[6];
  r[2] = v[0]*m[8] + v[1]*m[9] + v[2]*m[10];
}

void ixformPoint(float *v, float *r, GLfloat *m)
{
    float x[4];
    x[0] = v[0] - m[12];
    x[1] = v[1] - m[13];
    x[2] = v[2] - m[14];
    x[3] = 1.0f;
    ixform(x, r, m);
}

void motion(int x, int y)
{
    float dx, dy;
    dx = x - ox;
    dy = y - oy;

    if (displaySliders) {
        if (params->Motion(x, y)) {
            ox = x; oy = y;
            glutPostRedisplay();
            return;
        }
    }

    if (buttonState == 3) {
        // left+middle = zoom
        camera_trans[2] += (dy / 50.0) * 0.5 * fabs(camera_trans[2]);
    } 
    else if (buttonState & 2) {
        // middle = translate
        camera_trans[0] += dx / 15.0;
        camera_trans[1] -= dy / 15.0;
    }
    else if (buttonState & 1) {
        // left = rotate
        camera_rot[0] += dy / 5.0;
        camera_rot[1] += dx / 5.0;
	}

    ox = x; oy = y;

    glutPostRedisplay();
}



void screenShot()
{
	char anim_filename[2048];
    sprintf_s(anim_filename, 2048, "../media/screen/MULTIPHASE_%04d.png", screenShotNum++);
	grabScreen(anim_filename);
	cout << "Screen Shot: " << anim_filename << endl;
}

void key(unsigned char key, int x, int y)
{
    switch (key) 
    {
    case ' ':
        bPause = !bPause;
        break;
	case '\033':
    case 'q':
        exit(0);
        break;
    case 'r':
        bRecording = !bRecording;
		if(bRecording) frameNum = 0;
        break;
	case 's':
		screenShot();
		break;
    case 'h':
        displaySliders = !displaySliders;
        break;
	case 'p':
		//fluidSystem.Reset(fluidSystem.NumPoints());
		fluidSystem.SPH_CreateExample( 0, fluidSystem.NumPoints());
        break;
	//Colors
	case '1':
		renderer->setDisplayMode(DISPLAY_DEPTH);
		break;
	case '2':
		renderer->setDisplayMode(DISPLAY_NORMAL);
		break;
	case '3':
		renderer->setDisplayMode(DISPLAY_POSITION);
		break;
	case '4':
		renderer->setDisplayMode(DISPLAY_COLOR);
		break;
	case '5':
		renderer->setDisplayMode(DISPLAY_DIFFUSE);
		break;
	case '6':
		renderer->setDisplayMode(DISPLAY_DIFFUSE_SPEC);
		break;
	case '7':
		renderer->setDisplayMode(DISPLAY_FRESNEL);
		break;
	case '8':
		renderer->setDisplayMode(DISPLAY_REFLECTION);
		break;
	case '9':
		renderer->setDisplayMode(DISPLAY_FRES_REFL);
		break;
	case '!':
		renderer->setDisplayMode(DISPLAY_THICKNESS);
		break;
	case '@':
		renderer->setDisplayMode(DISPLAY_FRES_REFL);
		break;
	case '#':
		renderer->setDisplayMode(DISPLAY_REFRAC);
		break;
	case '0':
		renderer->setDisplayMode(DISPLAY_TOTAL);
		break;
    }

    glutPostRedisplay();
}

void initIL()
{
	ilInit();
    iluInit();
    ilEnable(IL_FILE_OVERWRITE);
    ilutRenderer(ILUT_OPENGL);
}




void special(int k, int x, int y)
{
    if (displaySliders) {
        params->Special(k, x, y);
    }
}

void simTimer(int value)
{	
	glutTimerFunc(100, simTimer, 0);
	if (!bPause)
	{
		//Update Particle System
		//fluidSystem.Run();
		//Record From Screen
		/*if (bRecording) {
			char anim_filename[2048];
			sprintf_s(anim_filename, 2048, "../media/img/MULTIPHASE_%04d.png", frameNum++); 
			grabScreen(anim_filename);
		}*/
	}

	glutPostRedisplay();

}

void idle(void)
{
    glutPostRedisplay();
}

void initParams()
{
    // create a new parameter list
    params = new ParamListGL("Misc.");
    params->AddParam(new Param<float>("time step", timestep, 0, 0.01, 0.0001, &timestep));
    params->AddParam(new Param<float>("art viscocity", viscocity, 0.0, 1.0, 0.01, &viscocity));
	params->AddParam(new Param<float>("particle radius", pRadius, 0.0, 1.0, 0.01, &pRadius));
	params->AddParam(new Param<float>("surface tension", surfTension, 0.0, 1.0, 0.01, &surfTension));
}

void mainMenu(int i)
{
    key((unsigned char) i, 0, 0);
}

void initMenus()
{
	int colorMenu = glutCreateMenu(mainMenu);
    glutAddMenuEntry("Display Velocity [1]", '1');
    glutAddMenuEntry("Display Pressure [2]", '2');
    glutAddMenuEntry("Display Temperature [3]", '3');
	glutAddMenuEntry("Display Original Color [4]", '4');

    glutCreateMenu(mainMenu);
    glutAddMenuEntry("Toggle animation [ ]", ' ');
	glutAddMenuEntry("Take screenshot [s]", 's');
	glutAddMenuEntry("Toggle recording [r]", 'r');
    glutAddMenuEntry("Toggle sliders [h]", 'h');
	glutAddSubMenu("Display Color", colorMenu);
    glutAddMenuEntry("Quit (esc)", '\033');
    glutAttachMenu(GLUT_RIGHT_BUTTON);
}

////////////////////////////////////////////////////////////////////////////////
// Program Main
////////////////////////////////////////////////////////////////////////////////
int
main(int argc, char** argv) 
{
	cout << argv[0] << " Starting...\n\n" << endl;

	cout << "particles: " << max_particles << endl;

	//Image Library Init
	initIL();

	//OpenGL Init
	initGL(&argc, argv);

	initParticleSystem(max_particles, gridSize, true);
    initParams();
	initMenus();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutKeyboardFunc(key);
	glutTimerFunc(100,simTimer, 0);
    glutSpecialFunc(special);
    glutIdleFunc(idle);

    atexit(cleanup);

    glutMainLoop();
}
