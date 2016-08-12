///////////////////////////////////////////////////////////////////////////////
//
// GLCA
// Coded Aperture System Response from OpenGL GPU-Projection
// P.Barton 8/11/16
//
// Ubuntu 14.04
// sudo apt-get install libglew-dev freeglut3-dev libglm-dev libconfig-dev
// g++ main.cpp model_ply.cpp LoadShaders.cpp chealpix.c -o glca -lGL -lglut -lGLEW -lconfig
// time ./glca config.text
//
///////////////////////////////////////////////////////////////////////////////

#define GLM_FORCE_RADIANS // (supress glm warning)
#define BUFFER_OFFSET(offset) ((void *)(offset))

#include <iostream>
#include <string>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <GL/glew.h> // libglew-dev
#include <GL/glut.h> // freeglut3-dev
#include <glm/glm.hpp> // libglm-dev
#include <glm/common.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <libconfig.h> // libconfig-dev

#include "LoadShaders.h"
#include "model_ply.h"
#include "chealpix.h"
using namespace std;

int printOglError() { 
	printf("GL_Error: %x \n\n...exiting.", glGetError()); exit(1); 
}
// ErrorCodes from GL.h
// #define GL_NO_ERROR                       0
// #define GL_INVALID_ENUM                   0x0500
// #define GL_INVALID_VALUE                  0x0501
// #define GL_INVALID_OPERATION              0x0502
// #define GL_STACK_OVERFLOW                 0x0503
// #define GL_STACK_UNDERFLOW                0x0504
// #define GL_OUT_OF_MEMORY                  0x0505

enum VAO_IDs { Triangles, NumVAOs };
enum Buffer_IDs { ArrayBuffer, NumBuffers };
enum Attrib_IDs { vPosition = 0, vColor = 1 };
GLuint vaoDetectors;
GLuint VAOs[NumVAOs];
GLuint Buffers[NumBuffers];
GLuint colorbuffer;
GLuint NumVertices;
Model_PLY ply;
GLuint program;
char fnModel[60];
bool detTriVis[12] = { 1,1,1,1,  1,1,1,1,  1,1,1,1 };

// rotation
GLuint transformLoc = 1;
glm::mat4 transform, transform0;
int nside, npix;
GLdouble pixVec[3];
glm::vec3 v3Center = glm::vec3(0, 0, 0);
glm::vec3 v3Up = glm::vec3(0, 1, 0);
int ithRotation = 0;
bool bAnimate = true;
bool bHealpix = false;

// mask
char maskHexName[49]; // with null
bool bMask[192], bMask0[192], bMaskRand[192];
bool bNonMask = false;
bool bDOI = false;

// display / control
int winSize = 200;
int xMouse;
int yMouse;
float fBW = 0.0;
int frameCounter = 0, myTime, startTime = 0, pausedTime;
bool paused = false;
bool bShowBuffers = true;
bool bFPS = false;

// hist
unsigned short* mapR;
unsigned short* mapRGB;
GLubyte* myPixelsR;
GLubyte* myPixelsRGB;
int myCountR[192];
int myCountRGB[3][192];

void init(void)
{
	npix = 12 * nside * nside;
	mapRGB = (unsigned short*)malloc(npix * 192 * 3 * sizeof(unsigned short));
	mapR = (unsigned short*)malloc(npix * 192 * sizeof(unsigned short));
	xMouse = winSize / 2;
	yMouse = winSize / 2;
	myPixelsR = (GLubyte*)malloc(1 * winSize * winSize); // non-DOI image
	myPixelsRGB = (GLubyte*)malloc(3 * winSize * winSize); // DOI image

	// Mask Hex to Bool
	char *endptr;
	for (int i = 0; i < 48; i++) {
		char c = maskHexName[i];
		
		long hexDigit = strtol(&c, &endptr, 16);
		for (int j = 3; j != -1; --j) {
			bMask[(4 * i + j)] = hexDigit & 1;
			hexDigit /= 2;
		}
	}
	printf("MaskHex: %s \n", maskHexName);

	// Load PRISM 1.0 Model
	ply.Load(fnModel);
	NumVertices = ply.NumFaces * 3;
	// printf("NumVertices: %d \n\n", NumVertices);

	// Load Vertex and Color Data
	GLfloat * g_color_buffer_data = (GLfloat*) malloc(NumVertices * 3 * sizeof(float));
	GLfloat * g_position_buffer_data = (GLfloat*) malloc(NumVertices * 3 * sizeof(float));
	int myIndex, myIndex2;
	int i = 0;
	for (int iterator = 0; iterator < 192; iterator++) // cubes
	{
		if (bMask[iterator]) // masked detectors
		{
			for (int j = 0; j < 12; j++) // triangles
			{
				for (int k = 0; k < 3; k++) // vertices
				{
					myIndex = (i * 12 * 3 * 3) + (j * 3 * 3) + (k * 3);
					myIndex2 = (iterator * 12 * 3 * 3) + (j * 3 * 3) + (k * 3);

					g_color_buffer_data[myIndex + 0] = fBW; // R
					g_color_buffer_data[myIndex + 1] = fBW; // G
					g_color_buffer_data[myIndex + 2] = fBW; // B

					if (detTriVis[j]) // select faces (with 1,2,3 keys)
					{
						if (bDOI) {
							if (j < 2)
								g_color_buffer_data[myIndex + 0] = (191-iterator +1) / 255.0;
							else if ((j >= 2) & (j < 4))
								g_color_buffer_data[myIndex + 1] = (191-iterator +1) / 255.0;
							else if (j >= 4)
								g_color_buffer_data[myIndex + 2] = (191-iterator +1) / 255.0;
						}
						else {
							g_color_buffer_data[myIndex + 0] = (iterator + 1) / 255.0;  // correct.
						}
					}

					g_position_buffer_data[myIndex + 0] = ply.FaceVertices[myIndex2 + 0] * 0.0125; // X *** fix 0.0125 scale! with transform
					g_position_buffer_data[myIndex + 1] = ply.FaceVertices[myIndex2 + 1] * 0.0125; // Y
					g_position_buffer_data[myIndex + 2] = ply.FaceVertices[myIndex2 + 2] * 0.0125; // Z
				}
			}
			i++;
		}
	}

	glGenVertexArrays(NumVAOs, VAOs); // *** fix enum...
	glBindVertexArray(VAOs[Triangles]);

	glGenBuffers(NumBuffers, Buffers);
	glBindBuffer(GL_ARRAY_BUFFER, Buffers[ArrayBuffer]);	
	glBufferData(GL_ARRAY_BUFFER, NumVertices * 3 * sizeof(float), g_position_buffer_data, GL_STATIC_DRAW);
	glVertexAttribPointer(vPosition, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
	glEnableVertexAttribArray(vPosition);
	
	glGenBuffers(1, &colorbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
	glBufferData(GL_ARRAY_BUFFER, NumVertices * 3 * sizeof(float), g_color_buffer_data, GL_STATIC_DRAW);
	glVertexAttribPointer(vColor, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
	glEnableVertexAttribArray(vColor);

	ShaderInfo shaders[] = {
		{ GL_VERTEX_SHADER, "triangles.vert" },
		{ GL_FRAGMENT_SHADER, "triangles.frag" },
		{ GL_NONE, NULL }
	};
	program = LoadShaders(shaders);
	
	glEnable(GL_DEPTH_TEST);
	glClearColor(fBW, fBW, fBW, 1.0);
}

void display(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram(program);

	// Rotate Model
	if (bHealpix) { // by healpix indices
		if (ithRotation > npix - 1) {
			FILE* pFile;
			pFile = fopen(strcat(maskHexName,".bin"), "wb");
			if (bDOI)
				fwrite(mapRGB, sizeof(unsigned short), npix * 192 * 3, pFile);
			else
				fwrite(mapR, sizeof(unsigned short), npix * 192, pFile);
			fclose(pFile);
			exit(1);
		}
		pix2vec_ring(nside, ithRotation, pixVec);
		transform = glm::lookAt(glm::vec3(pixVec[0] / 80, pixVec[1] / 80, -pixVec[2] / 80), v3Center, v3Up);
		ithRotation++;
	}
	else if (bAnimate) { // by time
		transform = glm::rotate(transform0, myTime * 0.0002f, glm::vec3(1.0, 1.0, 0.0));
		transform = glm::rotate(transform, myTime * 0.0002f, glm::vec3(1.0, 0.0, 1.0));
	}
	else { // by mouse 
		transform = glm::rotate(transform0, -(xMouse - winSize / 2) * 0.003f, glm::vec3(0.0f, 1.0f, 0.0f));
		transform = glm::rotate(transform, -(yMouse - winSize / 2) * 0.003f, glm::vec3(1.0f, 0.0f, 0.0f));
	}
	transformLoc = glGetUniformLocation(program,"utransform");
	glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(transform));

	// Draw to Buffer
	glDrawArrays(GL_TRIANGLES, 0, NumVertices);
	
	// Generate Pixel Histograms	
	// DOI (RGB) Histogram
	if (bDOI) {
		glReadPixels(0, 0, winSize, winSize, GL_RGB, GL_UNSIGNED_BYTE, myPixelsRGB);
		memset(myCountRGB, 0, 3 * 192 * sizeof(int));
		for (int i = 0; i < (winSize * winSize * 3); i += 3)
			for (int j = 0; j < 3; j++)
				if (myPixelsRGB[i + j] > 0)
					myCountRGB[j][myPixelsRGB[i + j] - 1]++;
		// Pack Histograms for file writing
		for (int i = 0; i < 192; i++)
			for (int j = 0; j < 3; j++)
				mapRGB[(ithRotation - 1) * 192 * 3 + i * 3 + j] = myCountRGB[j][i];
	} // non-DOI (Red) Histogram
	else {
		glReadPixels(0, 0, winSize, winSize, GL_RED, GL_UNSIGNED_BYTE, myPixelsR);
		memset(myCountR, 0, 192 * sizeof(int));
		for (int i = 0; i < (winSize * winSize); i++)
				if (myPixelsR[i] > 0)
					myCountR[myPixelsR[i] - 1]++;
		// Pack Histograms for file writing
		if (bHealpix)
			for (int i = 0; i < 192; i++)
				mapR[(ithRotation - 1) * 192 + i] = myCountR[i];
	}

	// Render Buffer to Window
	if (bShowBuffers) 
		glutSwapBuffers();

	// Report
	if (bFPS) 	{
		myTime = glutGet(GLUT_ELAPSED_TIME);
		if ( paused ) myTime = pausedTime;
		if ( myTime - startTime > 1000 ) {
			printf("\n FPS: %4.0f \n", frameCounter * 1000.0 / (myTime - startTime));
			startTime = myTime;
			frameCounter = 0;
		}
		frameCounter++;
	}
}

void key(unsigned char k, int x, int y)
{
	switch (k) {
	case '1':
		detTriVis[0] = !detTriVis[0];
		detTriVis[1] = !detTriVis[1];
		break;
	case '2':
		detTriVis[2] = !detTriVis[2];
		detTriVis[3] = !detTriVis[3];
		break; 
	case '3':
		for (int i = 4; i < 12; i++)
			detTriVis[i] = !detTriVis[i];
		break;
	case 'a':
		bAnimate = !bAnimate;
		break;
	case 'p':
		paused = !paused;
		pausedTime = myTime;
		break;
	case 'q':
		exit(1);
		break;
	}

	init();
}

void MotionCallback(int x, int y)
{
		xMouse = x;
		yMouse = y;
}

void idle()
{
	glutPostRedisplay();
}


void config(char *fn) 
{

	config_t cfg;
	const char *str = NULL;
	int iBool;

	config_init(&cfg);
	config_read_file(&cfg, fn);

	config_lookup_int(&cfg, "nside", &nside);

	if (config_lookup_string(&cfg, "mask", &str))
		for (int i = 0; i < 48; i++)
			maskHexName[i] = str[i];
	

	config_lookup_string(&cfg, "model", &str);
	strcpy(fnModel, str);

	config_lookup_int(&cfg, "winSize", &winSize);

	if (config_lookup_bool(&cfg, "showBuffers", &iBool))
		bShowBuffers = (bool)iBool;
	if (config_lookup_bool(&cfg, "bHealpix", &iBool))
		bHealpix = (bool)iBool; 
	if (config_lookup_bool(&cfg, "bAnimate", &iBool))
		bAnimate = (bool)iBool;
	if (config_lookup_bool(&cfg, "bDOI", &iBool))
		bDOI = (bool)iBool;
	if (config_lookup_bool(&cfg, "bFPS", &iBool))
		bFPS = (bool)iBool;

	config_destroy(&cfg);
}

int main(int argc, char **argv)
{
	// Config File
	if (argc > 1)
		config(argv[1]);
	else {
		printf("Please provide a valid config file.\n\n");
		system("pause");
		exit(-1);
	}
	
	glutInit(&argc, argv);
	glutInitWindowPosition(500, 0);
	glutInitWindowSize(winSize, winSize);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutCreateWindow( "PRISM 1.0" );

	glutDisplayFunc(display);
	glutMotionFunc(MotionCallback);
	glutKeyboardFunc(key);
	glutIdleFunc(idle);	
	
	// glewExperimental = GL_TRUE;  // !!!
	glewInit();
	
	init();

	glutMainLoop();
	return 0;
}