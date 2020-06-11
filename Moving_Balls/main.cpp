#define HAVE_STRUCT_TIMESPEC
#include <Windows.h>
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <pthread.h>
#include <thread>
#include "Dependencies/glew-1.11.0/include/GL/glew.h"
#include "Dependencies/freeglut/include/GL/freeglut.h"
#include "tbb/task_scheduler_init.h"
#include "tbb/parallel_for.h"
#include "tbb/blocked_range.h"
#include "tbb/tick_count.h"
using namespace tbb;

#define PI 3.14159265f

char title[] = "Moving Balls by Dmytro Holombik";  // Window title
int windowWidth = 1024;     // Window width
int windowHeight = 678;     // Window height
int windowPosX = 150;       // Window's top-left corner x
int windowPosY = 150;       // Window's top-left corner y

//=====================================================================================================================================



int refreshMillis = 33;      // Screen refresh period
GLfloat grav = 0.001;  

static int numberBalls = 0;

pthread_mutex_t lock;
int badIterator = 0;



void iteratorUp() {
	pthread_mutex_lock(&lock);
	badIterator++;
	pthread_mutex_unlock(&lock);
}


//generate a number to determine how many additional balls
int random()
{
	int r;
	srand(time(NULL));
	r = rand()%7+2;
	std::cout << r + 1<< " balls on screen";
	return r;
};

//bouncy ball struct for additional balls
struct BouncyBall {
	GLfloat radius;
	GLfloat xSpawn, ySpawn;
	GLfloat XMaxBound, XMinBound, YMaxBound, YMinBound;
	GLfloat xSpeed, ySpeed;
	bool glock;
};
static struct BouncyBall ballArray[11];

//ball creating
GLfloat ballRadius = 0.1f;						// Radius
GLfloat ballX = -0.2f;							// x position
GLfloat ballY = 0.9f;							// y positipn
GLfloat ballXMax, ballXMin, ballYMax, ballYMin; // Ball's center (x, y) bounds
GLfloat xSpeed = -0.06f;					    // Ball's speed in x direction
GLfloat ySpeed = 0.009f;						// Ball's speed in y direction



// Projection clipping area
GLdouble clipAreaXLeft, clipAreaXRight, clipAreaYBottom, clipAreaYTop;

// Initialize OpenGL Graphics /
void initGL() {
	glClearColor(0.0, 0.0, 0.0, 1.0); // Set background color
}

// Callback handler for window re-paint event
void display() {

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); //enable transparency

	glClear(GL_COLOR_BUFFER_BIT);  // Clear the color buffer
	glMatrixMode(GL_MODELVIEW);    // To operate on the model-view matrix
	glLoadIdentity();              // Reset model-view matrix

	//create all balls in BallArray
	int numSegments = 100;
	for (int i = 0; i <= (sizeof(ballArray) / sizeof(struct BouncyBall)); i++)
	{
		glBegin(GL_TRIANGLE_FAN);
		if (i % 3 == 0) {
			glColor4f(1.0f, 0.0f, 0.0f, 0.5f);
		}
		else if (i % 3 == 1) {
			glColor4f(0.0f, 1.0f, 0.0f, 0.5f);
		}
		else if (i % 3 == 2) {
			glColor4f(0.0f, 0.0f, 1.0f, 0.5f);
		}
		glVertex2f(ballArray[i].xSpawn, ballArray[i].ySpawn);
		GLfloat angleX;
		for (int j = 0; j <= numSegments; j++) {
			angleX = j * 2.0f * PI / numSegments;
			glVertex2f(ballArray[i].xSpawn + cos(angleX) * ballArray[i].radius, ballArray[i].ySpawn + sin(angleX) * ballArray[i].radius);
		}
		glEnd();
	}

	glutSwapBuffers();  // Swap front and back buffers (of double buffered mode)

	//location for balls in array
	for (int i = 0; i <= (sizeof(ballArray) / sizeof(struct BouncyBall)); i++) {
		ballArray[i].xSpawn += ballArray[i].xSpeed;
		ballArray[i].ySpawn += ballArray[i].ySpeed;
	}

	
	for (int i = 0; i <= (sizeof(ballArray) / sizeof(struct BouncyBall)); i++) {
		ballArray[i].glock = true;
	}

	//check if array balls exceed the edges
	for (int i = 0; i <= (sizeof(ballArray) / sizeof(struct BouncyBall)); i++)
	{
		if (ballArray[i].xSpawn > ballXMax) {
			ballArray[i].xSpawn = ballXMax;
			ballArray[i].xSpeed = -ballArray[i].xSpeed;
		}
		else if (ballArray[i].xSpawn < ballXMin) {
			ballArray[i].xSpawn = ballXMin;
			ballArray[i].xSpeed = -ballArray[i].xSpeed;
		}
		if (ballArray[i].ySpawn > ballYMax) {
			ballArray[i].ySpawn = ballYMax;
			ballArray[i].ySpeed = -ballArray[i].ySpeed;
		}
		else if (ballArray[i].ySpawn < ballArray[i].YMinBound) {
			ballArray[i].ySpawn = ballArray[i].YMinBound;
			ballArray[i].ySpeed = -ballArray[i].ySpeed;
			ballArray[i].glock = true;
		}
	}

	//check if balls are going crazy
	for (int i = 0; i <= numberBalls; i++) {
		if (ballArray[i].ySpeed < -0.3f)
			ballArray[i].ySpeed = -0.0006f;
		if (ballArray[i].ySpeed > 0.3f)
			ballArray[i].ySpeed = 0.0006f;
	}


	//TODO: colision

}


/* Call back when the windows is re-sized */
void reshape(GLsizei width, GLsizei height) {
	// Compute aspect ratio of the new window
	if (height == 0) height = 1;                // To prevent divide by 0
	GLfloat aspect = (GLfloat)width / (GLfloat)height;

	// Set the viewport to cover the new window
	glViewport(0, 0, width, height);

	// Set the aspect ratio of the clipping area to match the viewport
	glMatrixMode(GL_PROJECTION);  // To operate on the Projection matrix
	glLoadIdentity();             // Reset the projection matrix
	if (width >= height) {
		clipAreaXLeft = -1.0 * aspect;
		clipAreaXRight = 1.0 * aspect;
		clipAreaYBottom = -1.0;
		clipAreaYTop = 1.0;
	}
	else {
		clipAreaXLeft = -1.0;
		clipAreaXRight = 1.0;
		clipAreaYBottom = -1.0 / aspect;
		clipAreaYTop = 1.0 / aspect;
	}

	gluOrtho2D(clipAreaXLeft, clipAreaXRight, clipAreaYBottom, clipAreaYTop); //sets 2d orthographic viewing region. No touchy.


	ballXMin = clipAreaXLeft + ballRadius;
	ballXMax = clipAreaXRight - ballRadius;
	ballYMin = clipAreaYBottom + ballRadius;
	ballYMax = clipAreaYTop - ballRadius;

	//array balls
	for (int i = 0; i <= (sizeof(ballArray) / sizeof(struct BouncyBall)); i++) {

		ballArray[i].XMinBound = clipAreaXLeft + ballArray[i].radius;
		ballArray[i].XMaxBound = clipAreaXRight - ballArray[i].radius;
		ballArray[i].YMinBound = clipAreaYBottom + ballArray[i].radius;
		ballArray[i].YMaxBound = clipAreaYTop - ballArray[i].radius;
	}

}

/* Called back when the timer expired */
void Timer(int value) {
	glutPostRedisplay();    // Post a paint request to activate display()
	glutTimerFunc(refreshMillis, Timer, 0); // subsequent timer call at milliseconds
}

//tbb function
class TbbBallMaker {
public:
	void operator() (const blocked_range<size_t>& r) const {
		for (size_t i = r.begin(); i != r.end(); i++) {
			badIterator++;
			int no = badIterator;
			if (no % 2 == 0) {
				struct BouncyBall a = { (0.10f) + ((no % 3) * 0.01f), 0.35f * (no + 1), 0.56f * (no + 1), 0.0f,0.0f,0.0f,0.0f, 0.009f * (((no + 1) % 3) + 1), 0.0004f * (((no + 1) % 3) + 1), true };
				ballArray[no] = a;
			}
			else {
				struct BouncyBall a = { (0.05f) * ((no % 3) + 1), -0.35f * (no + 1), 0.56f * (no + 1), 0.0f,0.0f,0.0f,0.0f, -0.009f * (((no + 1) % 3) + 1), -0.0004f * (((no + 1) % 3) + 1), true };
				ballArray[no] = a;
			}
		}
	}
};

/// Main function
int main(int argc, char** argv) {

	pthread_t tid;

	task_scheduler_init init; //Let's boot up our task scheduler for TBB

	if (pthread_mutex_init(&lock, NULL) != 0) {
		std::cout << "ERROR";
		return 1;
	}

	int r = random();
	numberBalls = r; //also the number of times BallMaker will run

	struct BouncyBall firstBall = { 0.10f, -0.3f, 0.5f, 0.0f,0.0f,0.0f,0.0f, 0.03f, 0.002f, true };
	ballArray[0] = firstBall;

	//Go, TBB!
	parallel_for(blocked_range<size_t>(0, numberBalls), TbbBallMaker());


	//GL stuff
	glutInit(&argc, argv);            // Boot up GLUT
	glutInitDisplayMode(GLUT_DOUBLE); // For double-buffered mode
	glutInitWindowSize(windowWidth, windowHeight);  // Create a window with given height and width
	glutInitWindowPosition(windowPosX, windowPosY); // Where the window shows up on screen
	glutCreateWindow(title);      // Create window with given title
	glutDisplayFunc(display);     // For repaint
	glutReshapeFunc(reshape);     // For reshaping the window
	glutTimerFunc(0, Timer, 0);   // Start the timer
	initGL();                     // Our own OpenGL initialization
	glutMainLoop();               // Event loop
	pthread_mutex_destroy(&lock);

	return 0;
}
