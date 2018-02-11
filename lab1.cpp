//
//modified by: Joshua Cancellier
//date: 01-25-18
//
//3350 Spring 2018 Lab-1
//This program demonstrates the use of OpenGL and XWindows
//
//Assignment is to modify this program.
//You will follow along with your instructor.
//
//Elements to be learned in this lab...
// .general animation framework
// .animation loop
// .object definition and movement
// .collision detection
// .mouse/keyboard interaction
// .object constructor
// .coding style
// .defined constants
// .use of static variables
// .dynamic memory allocation
// .simple opengl components
// .git
//
//elements we will add to program...
//   .Game constructor
//   .multiple particles
//   .gravity
//   .collision detection
//   .more objects
//
#include <iostream>
using namespace std;
#include <stdio.h>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <cmath>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <GL/glx.h>
#include "fonts.h"

const int MAX_PARTICLES = 10000;
const float GRAVITY = 0.13;
const int BOXES = 5;
const int SPOKES = 16;
//some structures

struct Vec {
	float x, y, z;
};

struct Color{
	int r, g, b;
};

struct Shape {
	float width, height;
	float radius;
	Vec center;
	Color color;
};

struct Particle {
	Shape s;
	Vec velocity;
	Color color;
};

struct Circle{
	float radius;
	Vec center;
	Color color;
};

struct Spoke{
    	float radius;
	float angle;
	Vec center;
};

class Global {
public:
	int xres, yres;
	Shape box[BOXES];
	Particle particle[MAX_PARTICLES];
	Circle myCircle;
	Spoke mySpokes[SPOKES];
	int n;
	Global() {
	    	//screen resolution
		xres = 500;
		yres = 350;

		//define a box shape
		box[0].width = 60;
		box[0].height = 10;
		box[0].center.x = xres * 0.20;
		box[0].center.y = yres * 0.80;
		box[0].color.r = 0;
		box[0].color.g = 0;
		box[0].color.b = 0;

		//Create circle
		myCircle.radius = 120;
		myCircle.center.x = 450;
		myCircle.center.y = -40;
		myCircle.center.z = 0;
		myCircle.color.r = 128;
		myCircle.color.g = 43;
		myCircle.color.b = 0;

		//Create spoke
		for(int i=0; i<SPOKES; i++){
		    mySpokes[i].radius = myCircle.radius;
		    mySpokes[i].center = myCircle.center;
		    mySpokes[i].angle = 0 + 2*M_PI/(SPOKES)*(i);
		}

		//Create boxes for water to land on
		for(int i = 1; i < BOXES; i++){
			box[i].width = box[0].width;
			box[i].height = box[0].height;
			box[i].center.x = box[i-1].center.x + 50;
			box[i].center.y = box[i-1].center.y - 40;
			
		}
		n = 0;
	}
} g;

class X11_wrapper {
private:
	Display *dpy;
	Window win;
	GLXContext glc;
public:
	~X11_wrapper() {
		XDestroyWindow(dpy, win);
		XCloseDisplay(dpy);
	}
	X11_wrapper() {
		GLint att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
		int w = g.xres, h = g.yres;
		dpy = XOpenDisplay(NULL);
		if (dpy == NULL) {
			cout << "\n\tcannot connect to X server\n" << endl;
			exit(EXIT_FAILURE);
		}
		Window root = DefaultRootWindow(dpy);
		XVisualInfo *vi = glXChooseVisual(dpy, 0, att);
		if (vi == NULL) {
			cout << "\n\tno appropriate visual found\n" << endl;
			exit(EXIT_FAILURE);
		} 
		Colormap cmap = XCreateColormap(dpy, root, vi->visual, AllocNone);
		XSetWindowAttributes swa;
		swa.colormap = cmap;
		swa.event_mask =
			ExposureMask | KeyPressMask | KeyReleaseMask |
			ButtonPress | ButtonReleaseMask |
			PointerMotionMask |
			StructureNotifyMask | SubstructureNotifyMask;
		win = XCreateWindow(dpy, root, 0, 0, w, h, 0, vi->depth,
			InputOutput, vi->visual, CWColormap | CWEventMask, &swa);
		set_title();
		glc = glXCreateContext(dpy, vi, NULL, GL_TRUE);
		glXMakeCurrent(dpy, win, glc);
	}
	void set_title() {
		//Set the window title bar.
		XMapWindow(dpy, win);
		XStoreName(dpy, win, "3350 Lab1");
	}
	bool getXPending() {
		//See if there are pending events.
		return XPending(dpy);
	}
	XEvent getXNextEvent() {
		//Get a pending event.
		XEvent e;
		XNextEvent(dpy, &e);
		return e;
	}
	void swapBuffers() {
		glXSwapBuffers(dpy, win);
	}
} x11;

//Function prototypes
void init_opengl(void);
void check_mouse(XEvent *e);
int check_keys(XEvent *e);
void movement();
void render();

//=====================================
// MAIN FUNCTION IS HERE
//=====================================
int main()
{
	srand(time(NULL));
	init_opengl();
	//Main animation loop
	int done = 0;
	while (!done) {
		//Process external events.
		while (x11.getXPending()) {
			XEvent e = x11.getXNextEvent();
			check_mouse(&e);
			done = check_keys(&e);
		}
		movement();
		render();
		x11.swapBuffers();
	}
	return 0;
}

void init_opengl(void)
{
	//OpenGL initialization
	glViewport(0, 0, g.xres, g.yres);
	glEnable(GL_TEXTURE_2D);	
	initialize_fonts();
	//Initialize matrices
	glMatrixMode(GL_PROJECTION); glLoadIdentity();
	glMatrixMode(GL_MODELVIEW); glLoadIdentity();
	//Set 2D mode (no perspective)
	glOrtho(0, g.xres, 0, g.yres, -1, 1);
	//Set the screen background color
	glClearColor(0, 0, 0, 100);

}

void makeParticle(int x, int y)
{
	if (g.n >= MAX_PARTICLES)
		return;

	//position of particle
	Particle *p = &g.particle[g.n];
	p->s.center.x = x;
	p->s.center.y = y;
	p->velocity.y = (float) rand() / (float)(RAND_MAX) * 1.0;
	p->velocity.x = (float) rand() / (float)(RAND_MAX) * 1.0 - 0.5;
	
	//assign particle's color
	p->color.r = (rand() % (116 - 90)) + 90;
	p->color.g = (rand() % (204 - 188)) + 188;
	p->color.b = (rand() % (244 - 200)) + 200;
	++g.n;
}

void drawCircle(){
	GLint sides = 360;
    	Circle *c = &g.myCircle;
	glColor3ub(c->color.r,c->color.g,c->color.b);
	GLint vertices = sides + 2;
	GLfloat doublePi = 2.0f * 3.14159;
	GLfloat circleVerticesX[sides];
	GLfloat circleVerticesY[sides];
	GLfloat circleVerticesZ[sides];

	circleVerticesX[0] = c->center.x;
	circleVerticesY[0] = c->center.y;
	circleVerticesZ[0] = c->center.z;

	for(int i = 1; i < vertices; i++){
		circleVerticesX[i] = c->center.x + (c->radius * cos(i*doublePi/sides));
		circleVerticesY[i] = c->center.y + (c->radius * sin(i*doublePi/sides));
		circleVerticesZ[i] = c->center.z;
	}

	GLfloat allCircleVertices[vertices * 3];
	
	for(int i = 0; i < vertices; i++){
		allCircleVertices[i * 3] = circleVerticesX[i];
		allCircleVertices[(i * 3) + 1] = circleVerticesY[i];
		allCircleVertices[(i * 3) + 2] = circleVerticesZ[i];
	}

	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, allCircleVertices);
	glDrawArrays(GL_TRIANGLE_FAN, 0, vertices);
	glDisableClientState(GL_VERTEX_ARRAY);

}

void check_mouse(XEvent *e)
{
	if (e->type != ButtonRelease &&
		e->type != ButtonPress &&
		e->type != MotionNotify) {
		//This is not a mouse event that we care about.
		return;
	}
	//
	if (e->type == ButtonRelease) {
		return;
	}
	/*
	if (e->type == ButtonPress) {
		if (e->xbutton.button==1) {
			//Left button was pressed
			int y = g.yres - e->xbutton.y;
			for(int i = 0; i < 50; i++)
				makeParticle(e->xbutton.x, y);
			return;
		}
		if (e->xbutton.button==3) {
			//Right button was pressed
			return;
		}
	}
	if (e->type == MotionNotify) {
		//The mouse moved!
		if (savex != e->xbutton.x || savey != e->xbutton.y) {
			savex = e->xbutton.x;
			savey = e->xbutton.y;
			int y = g.yres - e->xbutton.y;
			for(int i = 0; i < 10; i++)
			    	makeParticle(e->xbutton.x, y);


		}
	}
	*/
}

int check_keys(XEvent *e)
{
	if (e->type != KeyPress && e->type != KeyRelease)
		return 0;
	int key = XLookupKeysym(&e->xkey, 0);
	if (e->type == KeyPress) {
		switch (key) {
			case XK_1:
				//Key 1 was pressed
				break;
			case XK_a:
				//Key A was pressed
				break;
			case XK_Escape:
				//Escape key was pressed
				return 1;
		}
	}
	return 0;
}

void movement()
{
	if (g.n <= 0)
		return;
	for(int i = 0; i < g.n; i++){
		Particle *p = &g.particle[i];
		p->s.center.x += p->velocity.x;
		p->s.center.y += p->velocity.y;
		p->velocity.y -= GRAVITY;

		//check for collision with shapes...
		for(int j = 0; j < BOXES; j++){
			Shape *s = &g.box[j];
			if(p->s.center.y < s->center.y + s->height && 
			    	p->s.center.y > s->center.y - s->height &&
			    	p->s.center.x > s->center.x - s->width &&
			    	p->s.center.x < s->center.x + s->width){
				p->velocity.y = -p->velocity.y;
				p->velocity.y *= 0.55;
				p->velocity.x += 0.005;
				s->color.r = 3;
				s->color.g = 180;
				s->color.b = 120;	

				//force particles to move right
				if(p->velocity.x < 0)
					p->velocity.x *= -1;
			}
			
		}

		//check for off-screen
		if (p->s.center.y < 0.0) {
			g.particle[i] = g.particle[--g.n];
		}

		//Check collision with circle
		float dist = sqrt(pow(p->s.center.x - g.myCircle.center.x,2) + pow(p->s.center.y - g.myCircle.center.y,2));
		if(dist < g.myCircle.radius){
			    p->velocity.y = -p->velocity.y;
			    p->velocity.y *= 0.55;
			    p->velocity.x -= 0.25;				
		    	    for(int i=0; i<SPOKES; i++)
			    	g.mySpokes[i].angle = g.mySpokes[i].angle+.0001;
			    	    
		}
	}
}

void render()
{

	glClear(GL_COLOR_BUFFER_BIT);
	//Draw shapes...
	//draw a box
	Shape *s;
	float w, h;
	for(int i = 0 ; i < BOXES; i++){
		s = &g.box[i];
		glColor3ub(s->color.r,s->color.g,s->color.b);
		glPushMatrix();
		glTranslatef(s->center.x, s->center.y, s->center.z);
		
		w = s->width;
		h = s->height;
		//boxes
		glBegin(GL_QUADS);
			glVertex2i(-w, -h);
			glVertex2i(-w,  h);
			glVertex2i( w,  h);
			glVertex2i( w, -h);
		glEnd();
		glPopMatrix();
	}
	s = &g.box[0];
	
	//Create particles
	for(int i = 0; i < 20; i++)
		makeParticle(s->center.x, s->center.y+50);	
	
	//render particles
	for(int i = 0; i < g.n; i++){
		glPushMatrix();	
		Vec *c = &g.particle[i].s.center;
		
		//set particle color
		glColor3ub(g.particle[i].color.r, g.particle[i].color.g, g.particle[i].color.b );

		w =
		h = 2;

		//render particle
		glBegin(GL_QUADS);
			glVertex2i(c->x-w, c->y-h);
			glVertex2i(c->x-w, c->y+h);
			glVertex2i(c->x+w, c->y+h);
			glVertex2i(c->x+w, c->y-h);
		glEnd();
		glPopMatrix();
	}
	
	//Draw Circle
	drawCircle();	

	//Draw text for boxes
    	Rect r;
    	char str[][13] = {"Requirements", "Design", "Coding", "Testing", "Maintenence"};
    	for(int i = 0; i < BOXES; i++){
    		r.bot = g.box[i].center.y - 6;
    		r.left = g.box[i].center.x;
    		ggprint8b(&r,16, 0, str[i]);
    	}

	//Draw spoke
	for(int i=0; i<SPOKES; i++){    
	    glLineWidth(4);
	    glColor3f(0,0,0);
	    glBegin(GL_LINES);
	    glVertex3f(g.mySpokes[i].center.x, g.mySpokes[i].center.y, 0);
	    glVertex3f(g.mySpokes[i].center.x + cos(g.mySpokes[i].angle)*g.mySpokes[i].radius,
		       g.mySpokes[i].center.y + sin(g.mySpokes[i].angle)*g.mySpokes[i].radius, 0);
	    glEnd();
	}

}


























































