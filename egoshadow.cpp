#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <GL/glx.h>
#include "log.h"
//#include "ppm.h"
#include "fonts.h"
#include "nchiang.h"
#include "egoshadow.h"

#define ALPHA 1;

//function prototypes
void initOpengl();
void checkMouse(XEvent *e);
int checkKeys(XEvent *e);
void init();
void physics();
void render();
void play_game();

//-----------------------------------------------------------------------------
//Setup timers
class Timers {
    public:
        double physicsRate;
        double oobillion;
        struct timespec timeStart, timeEnd, timeCurrent;
        struct timespec walkTime;
        Timers() {
            physicsRate = 1.0 / 30.0;
            oobillion = 1.0 / 1e9;
        }
        double timeDiff(struct timespec *start, struct timespec *end) {
            return (double)(end->tv_sec - start->tv_sec ) +
                (double)(end->tv_nsec - start->tv_nsec) * oobillion;
        }
        void timeCopy(struct timespec *dest, struct timespec *source) {
            memcpy(dest, source, sizeof(struct timespec));
        }
        void recordTime(struct timespec *t) {
            clock_gettime(CLOCK_REALTIME, t);
        }
} timers;
//-----------------------------------------------------------------------------

Sprite::Sprite() {
    onoff = 0;
    frame = 0;
    image = NULL;
    delay = 0.1;
}

Global::~Global() {
    logClose();
}

Global::Global() {
    logOpen();
    camera[0] = camera[1] = 0.0;
    movie=0;
    movieStep=2;
    xres=800;
    yres=600;
    show_credits = 0;
    max_hp = 100;
    memset(keys, 0, 65536);
} Global gl;

//X Windows variables

X11_wrapper::~X11_wrapper() {
    XDestroyWindow(dpy, win);
    XCloseDisplay(dpy);
}
void X11_wrapper::setTitle() {
    //Set the window title bar.
    XMapWindow(dpy, win);
    XStoreName(dpy, win, "EgoShadow");
}
void X11_wrapper::setupScreenRes(const int w, const int h) {
    gl.xres = w;
    gl.yres = h;
}
X11_wrapper::X11_wrapper() {
    GLint att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
    //GLint att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, None };
    XSetWindowAttributes swa;
    setupScreenRes(gl.xres, gl.yres);
    dpy = XOpenDisplay(NULL);
    if (dpy == NULL) {
        printf("\n\tcannot connect to X server\n\n");
        exit(EXIT_FAILURE);
    }
    Window root = DefaultRootWindow(dpy);
    XVisualInfo *vi = glXChooseVisual(dpy, 0, att);
    if (vi == NULL) {
        printf("\n\tno appropriate visual found\n\n");
        exit(EXIT_FAILURE);
    } 
    Colormap cmap = XCreateColormap(dpy, root, vi->visual, AllocNone);
    swa.colormap = cmap;
    swa.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask |
        PointerMotionMask | MotionNotify | ButtonPress | ButtonRelease |
        StructureNotifyMask | SubstructureNotifyMask;
    win = XCreateWindow(dpy, root, 0, 0, gl.xres, gl.yres, 0,
            vi->depth, InputOutput, vi->visual,
            CWColormap | CWEventMask, &swa);
    GLXContext glc = glXCreateContext(dpy, vi, NULL, GL_TRUE);
    glXMakeCurrent(dpy, win, glc);
    setTitle();
}
void X11_wrapper::reshapeWindow(int width, int height) {
    //window has been resized.
    setupScreenRes(width, height);
    glViewport(0, 0, (GLint)width, (GLint)height);
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    glOrtho(0, gl.xres, 0, gl.yres, -1, 1);
    setTitle();
}
void X11_wrapper::checkResize(XEvent *e) {
    //The ConfigureNotify is sent by the
    //server if the window is resized.
    if (e->type != ConfigureNotify)
        return;
    XConfigureEvent xce = e->xconfigure;
    if (xce.width != gl.xres || xce.height != gl.yres) {
        //Window size did change.
        reshapeWindow(xce.width, xce.height);
    }
}
bool X11_wrapper::getXPending() {
    return XPending(dpy);
}
XEvent X11_wrapper::getXNextEvent() {
    XEvent e;
    XNextEvent(dpy, &e);
    return e;
}
void X11_wrapper::swapBuffers() {
    glXSwapBuffers(dpy, win);
}
X11_wrapper x11;

Image::~Image() {
     delete [] data;
}

Image::Image(const char *fname) {
    if (fname[0] == '\0')
        return;
    //printf("fname **%s**\n", fname);
    int ppmFlag = 0;
    char name[40];
    strcpy(name, fname);
    int slen = strlen(name);
    char ppmname[80];
    if (strncmp(name+(slen-4), ".ppm", 4) == 0)
        ppmFlag = 1;
    if (ppmFlag) {
        strcpy(ppmname, name);
    } else {
        name[slen-4] = '\0';
        //printf("name **%s**\n", name);
        sprintf(ppmname,"%s.ppm", name);
        //printf("ppmname **%s**\n", ppmname);
        char ts[100];
        //system("convert eball.jpg eball.ppm");
        sprintf(ts, "convert %s %s", fname, ppmname);
        system(ts);
    }
    //sprintf(ts, "%s", name);
    //printf("read ppm **%s**\n", ppmname); fflush(stdout);
    FILE *fpi = fopen(ppmname, "r");
    if (fpi) {
        char line[200];
        fgets(line, 200, fpi);
        fgets(line, 200, fpi);
        //skip comments and blank lines
        while (line[0] == '#' || strlen(line) < 2)
            fgets(line, 200, fpi);
        sscanf(line, "%i %i", &width, &height);
        fgets(line, 200, fpi);
        //get pixel data
        int n = width * height * 3;			
        data = new unsigned char[n];			
        for (int i=0; i<n; i++)
            data[i] = fgetc(fpi);
        fclose(fpi);
    } else {
    printf("ERROR opening image: %s\n",ppmname);
    exit(0);
    }
    if (!ppmFlag)
    unlink(ppmname);
}

/*Image img[3] = {
    "./images/walk.gif",
    "./images/exp.png",
    "./images/exp44.png" };*/


int main(void)
{
    initOpengl();
    init();
    display_menu(); // Calls from nchiang.cpp for a main menu

    return 0;
}

unsigned char *buildAlphaData(Image *img)
{
    //add 4th component to RGB stream...
    int i;
    unsigned char *newdata, *ptr;
    unsigned char *data = (unsigned char *)img->data;
    newdata = (unsigned char *)malloc(img->width * img->height * 4);
    ptr = newdata;
    unsigned char a,b,c;
    //use the first pixel in the image as the transparent color.
    unsigned char t0 = *(data+0);
    unsigned char t1 = *(data+1);
    unsigned char t2 = *(data+2);
    for (i=0; i<img->width * img->height * 3; i+=3) {
        a = *(data+0);
        b = *(data+1);
        c = *(data+2);
        *(ptr+0) = a;
        *(ptr+1) = b;
        *(ptr+2) = c;
        *(ptr+3) = 1;
        if (a==t0 && b==t1 && c==t2)
            *(ptr+3) = 0;
        //-----------------------------------------------
        ptr += 4;
        data += 3;
    }
    return newdata;
}

void initOpengl(void)
{
    //OpenGL initialization
    glViewport(0, 0, gl.xres, gl.yres);
    //Initialize matrices
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    //This sets 2D mode (no perspective)
    glOrtho(0, gl.xres, 0, gl.yres, -1, 1);
    //
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_FOG);
    glDisable(GL_CULL_FACE);
    //
    //Clear the screen
    glClearColor(1.0, 1.0, 1.0, 1.0);
    //glClear(GL_COLOR_BUFFER_BIT);
    //Do this to allow fonts
    glEnable(GL_TEXTURE_2D);
    initialize_fonts();
}

void init() {

}

void checkMouse(XEvent *e)
{
    //printf("checkMouse()...\n"); fflush(stdout);
    //Did the mouse move?
    //Was a mouse button clicked?
    static int savex = 0;
    static int savey = 0;
    //
    if (e->type != ButtonRelease && e->type != ButtonPress &&
            e->type != MotionNotify)
        return;
    if (e->type == ButtonRelease) {
        return;
    }
    if (e->type == ButtonPress) {
        if (e->xbutton.button==1) {
            //Left button is down
        }
        if (e->xbutton.button==3) {
            //Right button is down
        }
    }
    if (e->type == MotionNotify) {
        if (savex != e->xbutton.x || savey != e->xbutton.y) {
            //Mouse moved
            savex = e->xbutton.x;
            savey = e->xbutton.y;
        }
    }
}

//Added Play Game - Nicklas Chiang
void play_game()
{
    int done = 0;
    while (!done) {
        while (x11.getXPending()) {
            XEvent e = x11.getXNextEvent();
            x11.checkResize(&e);
            checkMouse(&e);
            done = checkKeys(&e);
        }
        physics();
        render();
        x11.swapBuffers();
    }
    cleanup_fonts();
}

void screenCapture()
{
    static int fnum = 0;
    static int vid = 0;
    if (!vid) {
        system("mkdir ./vid");
        vid = 1;
    }
    unsigned char *data = (unsigned char *)malloc(gl.xres * gl.yres * 3);
    glReadPixels(0, 0, gl.xres, gl.yres, GL_RGB, GL_UNSIGNED_BYTE, data);
    char ts[32];
    sprintf(ts, "./vid/pic%03i.ppm", fnum);
    FILE *fpo = fopen(ts,"w");	
    if (fpo) {
        fprintf(fpo, "P6\n%i %i\n255\n", gl.xres, gl.yres);
        unsigned char *p = data;
        //go backwards a row at a time...
        p = p + ((gl.yres-1) * gl.xres * 3);
        unsigned char *start = p;
        for (int i=0; i<gl.yres; i++) {
            for (int j=0; j<gl.xres*3; j++) {
                fprintf(fpo, "%c",*p);
                ++p;
            }
            start = start - (gl.xres*3);
            p = start;
        }
        fclose(fpo);
        char s[256];
        sprintf(s, "convert ./vid/pic%03i.ppm ./vid/pic%03i.gif", fnum, fnum);
        system(s);
        unlink(ts);
    }
    ++fnum;
}

int checkKeys(XEvent *e)
{
    if (e->type != KeyPress && e->type != KeyRelease)
        //time_key_press(false);
        return 0;
    int key = XLookupKeysym(&e->xkey, 0);

    switch (key) {
        case XK_C:
        case XK_c:
            gl.show_credits ^= 1;
            break;
        case XK_R:
            break;
        case XK_r:
            break;
        case XK_s:
            break;
        case XK_g:
            break;
        case XK_m:
            break;
        case XK_w:
            break;
        case XK_e:
            break;
        case XK_f:
            break;
        case XK_Left:
            break;
        case XK_Right:
            break;
        case XK_Up:
            break;
        case XK_Down:
            break;
        case XK_Escape:
            return 1;
            break;
    }
    return 0;
}

Flt VecNormalize(Vec vec)
{
    Flt len, tlen;
    Flt xlen = vec[0];
    Flt ylen = vec[1];
    Flt zlen = vec[2];
    len = xlen*xlen + ylen*ylen + zlen*zlen;
    if (len == 0.0) {
        MakeVector(vec, 0.0, 0.0, 1.0);
        return 1.0;
    }
    len = sqrt(len);
    tlen = 1.0 / len;
    vec[0] = xlen * tlen;
    vec[1] = ylen * tlen;
    vec[2] = zlen * tlen;
    return(len);
}

void physics(void)
{
}

void render(void)
{
    Rect r;
    //Clear the screen
    glClearColor(0.1, 0.1, 0.1, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    r.bot = gl.yres - 20;
    r.left = 10;
    r.center = 0;

    //Calls from nchiang.cpp to display credits
    if (gl.show_credits) {
        display_credits();
    }
    /*
    //Calls from nchiang.cpp to display hp on screen
    gl.current_hp = 100;
    display_hp(gl.current_hp, gl.max_hp); */

}