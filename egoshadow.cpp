//
//Framework: background.cpp
//author:  Gordon Griesel
//date:    2017 - 2018
//
//The position of the background QUAD does not change.
//Just the texture coordinates change.
//In this example, only the x coordinates change.
//
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
#include "fonts.h"
#include <iostream>
#include <chrono>
#define ALPHA 1;

//24-bit color:  8 + 8 + 8 = 24
//               R   G   B
//how many colors?  256*256*256 = 16-million+
//
//32-bit color:  8 + 8 + 8     = 24
//               R   G   B   A
//               R   G   B     = 24
//
//char data[1000][3]  aaabbbcccdddeeefff
//char data[1000][4]  aaa bbb ccc ddd eee fff
//
//
class Image {
    public:
        int width, height;
        unsigned char *data;
        ~Image() { delete [] data; }
        Image(const char *fname) {
            if (fname[0] == '\0')
                return;
            //printf("fname **%s**\n", fname);
            char name[40];
            strcpy(name, fname);
            int slen = strlen(name);
            name[slen-4] = '\0';
            //printf("name **%s**\n", name);
            char ppmname[80];
            sprintf(ppmname,"%s.ppm", name);
            //printf("ppmname **%s**\n", ppmname);
            char ts[100];
            //system("convert eball.jpg eball.ppm");
            sprintf(ts, "convert %s %s", fname, ppmname);
            system(ts);
            //sprintf(ts, "%s", name);
            FILE *fpi = fopen(ppmname, "r");
            if (fpi) {
                char line[200];
                fgets(line, 200, fpi);
                fgets(line, 200, fpi);
                while (line[0] == '#')
                    fgets(line, 200, fpi);
                sscanf(line, "%i %i", &width, &height);
                //printf("%i %i\n", width, height);


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
            unlink(ppmname);
        }
};
Image img[3] = {"ds.jpg", "solaire.png", "abyss.png"};

class Texture {
    public:
        Image *backImage;
        GLuint backTexture;
        float xc[2];
        float yc[2];
};

class Global {
    public:
        int xres, yres;
        int show_credits;
        Texture tex;
        Texture solaire;
        Texture abyss;
        Global() {
            xres=1024, yres=1024;
            show_credits = 0;
        }
} g;

class X11_wrapper {
    public:
        Display *dpy;
        Window win;
        GLXContext glc;
        X11_wrapper() {
            GLint att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
            //GLint att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, None };
            setup_screen_res(800, 600);
            dpy = XOpenDisplay(NULL);
            if(dpy == NULL) {
                printf("\n\tcannot connect to X server\n\n");
                exit(EXIT_FAILURE);
            }
            Window root = DefaultRootWindow(dpy);
            XVisualInfo *vi = glXChooseVisual(dpy, 0, att);
            if(vi == NULL) {
                printf("\n\tno appropriate visual found\n\n");
                exit(EXIT_FAILURE);
            } 
            Colormap cmap = XCreateColormap(dpy, root, vi->visual, AllocNone);
            XSetWindowAttributes swa;
            swa.colormap = cmap;
            swa.event_mask =
                ExposureMask | KeyPressMask | KeyReleaseMask | PointerMotionMask |
                ButtonPressMask | ButtonReleaseMask |
                StructureNotifyMask | SubstructureNotifyMask;
            win = XCreateWindow(dpy, root, 0, 0, g.xres, g.yres, 0,
                    vi->depth, InputOutput, vi->visual,
                    CWColormap | CWEventMask, &swa);
            set_title();
            glc = glXCreateContext(dpy, vi, NULL, GL_TRUE);
            glXMakeCurrent(dpy, win, glc);
        }
        void cleanupXWindows() {
            XDestroyWindow(dpy, win);
            XCloseDisplay(dpy);
        }
        void setup_screen_res(const int w, const int h) {
            g.xres = w;
            g.yres = h;
        }
        void reshape_window(int width, int height) {
            //window has been resized.
            setup_screen_res(width, height);
            glViewport(0, 0, (GLint)width, (GLint)height);
            glMatrixMode(GL_PROJECTION); glLoadIdentity();
            glMatrixMode(GL_MODELVIEW); glLoadIdentity();
            glOrtho(0, g.xres, 0, g.yres, -1, 1);
            set_title();
        }
        void set_title() {
            //Set the window title bar.
            XMapWindow(dpy, win);
            XStoreName(dpy, win, "scrolling background (seamless)");
        }
        bool getXPending() {
            return XPending(dpy);
        }
        XEvent getXNextEvent() {
            XEvent e;
            XNextEvent(dpy, &e);
            return e;
        }
        void swapBuffers() {
            glXSwapBuffers(dpy, win);
        }
        void check_resize(XEvent *e) {
            //The ConfigureNotify is sent by the
            //server if the window is resized.
            if (e->type != ConfigureNotify)
                return;
            XConfigureEvent xce = e->xconfigure;
            if (xce.width != g.xres || xce.height != g.yres) {
                //Window size did change.
                reshape_window(xce.width, xce.height);
            }
        }
} x11;

void init_opengl(void);
void check_mouse(XEvent *e);
int check_keys(XEvent *e);
void physics(void);
void render(void);
void play_game();
void init();
void background();
void move_background();
void display_credits();
void display_game_over();
void display_options();
void return_to_menu();
void display_menu();
void display_startup();

//===========================================================================
//===========================================================================
int main()
{
    init_opengl();
    display_startup();

    display_menu();

    return 0;
}

unsigned char *buildAlphaData(Image *img)
{   
    //add 4th component to RGB stream...
    int i;
    int a,b,c;
    unsigned char *newdata, *ptr;
    unsigned char *data = (unsigned char *)img->data;
    newdata = (unsigned char *)malloc(img->width * img->height * 4);
    ptr = newdata;
    for (i=0; i<img->width * img->height * 3; i+=3) {
        a = *(data+0);
        b = *(data+1);
        c = *(data+2);
        *(ptr+0) = a;
        *(ptr+1) = b;
        *(ptr+2) = c;
        //-----------------------------------------------
        //get largest color component...
        //*(ptr+3) = (unsigned char)((
        //      (int)*(ptr+0) +
        //      (int)*(ptr+1) +
        //      (int)*(ptr+2)) / 3);
        //d = a;
        //if (b >= a && b >= c) d = b;
        //if (c >= a && c >= b) d = c;
        //*(ptr+3) = d;
        //-----------------------------------------------
        //this code optimizes the commented code above.
        if(a==255 && b==255 && c==255) {
            *(ptr+3) = 0; 
        } else {
            *(ptr+3) = 1; 
        }
        //-----------------------------------------------
        ptr += 4;
        data += 3;
    }
    return newdata;
}

void init_opengl(void)
{
    //OpenGL initialization
    glViewport(0, 0, g.xres, g.yres);
    //Initialize matrices
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    //This sets 2D mode (no perspective)
    glOrtho(0, g.xres, 0, g.yres, -1, 1);
    //Clear the screen
    glClearColor(1.0, 1.0, 1.0, 1.0);
    //glClear(GL_COLOR_BUFFER_BIT);
    //Do this to allow texture maps
    glEnable(GL_TEXTURE_2D);
    initialize_fonts();
    //
    //load the images file into a ppm structure.
    //
    g.tex.backImage = &img[0];
    //create opengl texture elements
    glGenTextures(1, &g.tex.backTexture);
    int w = g.tex.backImage->width;
    int h = g.tex.backImage->height;
    glBindTexture(GL_TEXTURE_2D, g.tex.backTexture);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, 3, w, h, 0,
            GL_RGB, GL_UNSIGNED_BYTE, g.tex.backImage->data);
    g.tex.xc[0] = 0.0;
    g.tex.xc[1] = 0.25;
    g.tex.yc[0] = 0.0;
    g.tex.yc[1] = 1.0;



    unsigned char *data1 = buildAlphaData(&img[1]);
    //unsigned char *data2 = new unsigned char[h * w * 4];
    g.solaire.backImage = &img[1];
    glGenTextures(1, &g.solaire.backTexture);
    w = g.solaire.backImage->width;
    h = g.solaire.backImage->height;
    glBindTexture(GL_TEXTURE_2D, g.solaire.backTexture);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, 4, w, h, 0,
            GL_RGBA, GL_UNSIGNED_BYTE, data1);
    g.solaire.xc[0] = 0.0;
    g.solaire.xc[1] = 1.0;
    g.solaire.yc[0] = 0.0;
    g.solaire.yc[1] = 1.0;



    unsigned char *data2 = buildAlphaData(&img[2]);
    // abyss
    //unsigned char *data2 = new unsigned char[h * w * 4]; 
    g.abyss.backImage = &img[2];
    glGenTextures(1, &g.abyss.backTexture);
    w = g.abyss.backImage->width;
    h = g.abyss.backImage->height;
    glBindTexture(GL_TEXTURE_2D, g.abyss.backTexture);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, 4, w, h, 0,
            GL_RGBA, GL_UNSIGNED_BYTE, data2);
    g.abyss.xc[0] = 0.0;
    g.abyss.xc[1] = 1.0;
    g.abyss.yc[0] = 0.0;
    g.abyss.yc[1] = 1.0;





}

void init() {

}

void check_mouse(XEvent *e)
{
    //Did the mouse move?
    //Was a mouse button clicked?
    static int savex = 0;
    static int savey = 0;
    //
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
    if (savex != e->xbutton.x || savey != e->xbutton.y) {
        //Mouse moved
        savex = e->xbutton.x;
        savey = e->xbutton.y;
    }
}

int check_keys(XEvent *e)
{
    //Was there input from the keyboard?
    if (e->type == KeyPress) {
        int key = XLookupKeysym(&e->xkey, 0);
        if (key == XK_Escape) {
            return 1;
        }
    }
    return 0;
}

//Added Play Game - Nicklas Chiang
void play_game()
{
    int done = 0;
    while (!done) {
        while (x11.getXPending()) {
            XEvent e = x11.getXNextEvent();
            x11.check_resize(&e);
            check_mouse(&e);
            done = check_keys(&e);
        }
        physics();
        render();
        x11.swapBuffers();
    }
    cleanup_fonts();
}

//Function to display credits
void display_credits() {

    int windowWidth = g.xres;
    int windowHeight = g.yres;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4ub(0, 0, 0, 200);

    glBegin(GL_QUADS);
    glVertex2f(0, 0);
    glVertex2f(0, windowHeight);
    glVertex2f(windowWidth, windowHeight);
    glVertex2f(windowWidth, 0);
    glEnd();

    // Center the text within the window
    Rect r;
    r.bot = windowHeight / 2;
    r.left = windowWidth / 2;
    r.center = 1;

    ggprint13(&r, 20, 0x00ff0000, "Credits:");
    ggprint13(&r, 16, 0x00ff0000, "Nicklas Chiang");
    ggprint13(&r, 16, 0x00ff0000, "Nicholas Romasanta");

    glDisable(GL_BLEND);

    return_to_menu();
}

//Function to display game over screen
//Right now need to manually set the hp to 0.
void display_game_over() {
    int windowWidth = g.xres;
    int windowHeight = g.yres;

    // Set the background color and transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4ub(0, 0, 0, 200);

    // Draw a rectangle to cover the entire window as the background
    glBegin(GL_QUADS);
    glVertex2f(0, 0);
    glVertex2f(0, windowHeight);
    glVertex2f(windowWidth, windowHeight);
    glVertex2f(windowWidth, 0);
    glEnd();

    // Display the "Game Over" text centered on the screen
    Rect r;
    r.bot = windowHeight / 2 + 20;  // Adjusted for placement of "Press R to restart"
    r.left = windowWidth / 2;
    r.center = 1;

    ggprint13(&r, 30, 0x00ff0000, "Game Over");

    // Display the prompt to restart the game
    r.bot -= 40;
    ggprint13(&r, 16, 0x00ffffff, "Press 'R' to restart");

    glDisable(GL_BLEND);
}

//Function to display options on screen
void display_options()
{   
    int windowWidth = g.xres;
    int windowHeight = g.yres;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4ub(0, 0, 0, 200);

    glBegin(GL_QUADS);
    glVertex2f(0, 0);
    glVertex2f(0, windowHeight);
    glVertex2f(windowWidth, windowHeight);
    glVertex2f(windowWidth, 0);
    glEnd();

    // Center the text within the window
    Rect r;
    r.bot = windowHeight / 2;
    r.left = windowWidth / 2;
    r.center = 1;

    ggprint8b(&r, 16, 0xffffffff, "Test");
    ggprint8b(&r, 16, 0xffffffff, "Test");
    ggprint8b(&r, 16, 0xffffffff, "Test");
    ggprint8b(&r, 16, 0xffffffff, "Test");
    ggprint8b(&r, 16, 0xffffffff, "Test");

    glDisable(GL_BLEND);

    return_to_menu();
}

//Function to allow returning to menu
void return_to_menu()
{
    bool returnToMenu = false;
    while (!returnToMenu) {
        XEvent event;
        while (XPending(x11.dpy)) {
            XNextEvent(x11.dpy, &event);
            switch (event.type) {
                case KeyPress:
                    // Check if 'q' is pressed to return to the menu
                    if (XLookupKeysym(&event.xkey, 0) == XK_q) {
                        returnToMenu = true;
                    }
                    break;
            }
        }
        // Swap buffers
        glXSwapBuffers(x11.dpy, x11.win);
        usleep(1000);
    }

    display_menu(); // Return to the menu after options
}

//Function to Display Main Menu
void display_menu() {
    int selectedOption = 0; // Track the selected option
    bool menuActive = true; // Control variable for the menu loop

    while (menuActive) {
        // Clear the screen
        glClear(GL_COLOR_BUFFER_BIT);

        background();
        move_background();

        // Display menu options
        Rect r;
        r.bot = g.yres / 2;
        r.left = g.xres / 2;
        r.center = 1;
        unsigned int color = 0x00ff0000;

        ggprint13(&r, 45, 0x00ff0000, "EgoShadow");
        ggprint13(&r, 20, color, selectedOption == 0 ? "> Play Game" : "Play Game");
        ggprint13(&r, 20, color, selectedOption == 1 ? "> Credits" : "Credits");
        ggprint13(&r, 45, color, selectedOption == 2 ? "> Options" : "Options");
        ggprint13(&r, 45, color, " Press Enter to select.");
        ggprint13(&r, 45, color, " Press q to return.");

        // Handle user input
        XEvent event;
        while (XPending(x11.dpy)) {
            XNextEvent(x11.dpy, &event);
            switch (event.type) {
                case KeyPress:
                    KeySym key;
                    char text[10];
                    int i = XLookupString(&event.xkey, text, 10, &key, 0);
                    if (i == 1) {
                        if (text[0] == '\r' || text[0] == '\n') {
                            // Perform actions based on selected option
                            switch (selectedOption) {
                                case 0:
                                    play_game();
                                    menuActive = false;
                                    break;
                                case 1:
                                    display_credits();
                                    break;
                                case 2:
                                    display_options();
                                    break;
                            }
                        } else if (text[0] == 'w' || text[0] == 'W') {
                            // Move selection up on 'W' or 'w'
                            selectedOption = (selectedOption - 1 + 3) % 3;
                        } else if (text[0] == 's' || text[0] == 'S') {
                            // Move selection down on 'S' or 's'
                            selectedOption = (selectedOption + 1) % 3;
                        } 
                    }
                    break;
            }
        }

        // Swap buffers
        glXSwapBuffers(x11.dpy, x11.win);
        usleep(1000);
    }
}
void display_startup() {
    std::cout << "hi" << std::endl;
}
void background()
{
    glClear(GL_COLOR_BUFFER_BIT);
    glColor3f(1.0, 1.0, 1.0);
    //draw bg
    glBindTexture(GL_TEXTURE_2D, g.tex.backTexture);
    glBegin(GL_QUADS);
    glTexCoord2f(g.tex.xc[0], g.tex.yc[1]); glVertex2i(0, 0);
    glTexCoord2f(g.tex.xc[0], g.tex.yc[0]); glVertex2i(0, g.yres);
    glTexCoord2f(g.tex.xc[1], g.tex.yc[0]); glVertex2i(g.xres, g.yres);
    glTexCoord2f(g.tex.xc[1], g.tex.yc[1]); glVertex2i(g.xres, 0);
    glEnd();
    //draw solaire
    glBindTexture(GL_TEXTURE_2D, g.solaire.backTexture);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.0f);
    glColor4ub(255,255,255,255);
    glBindTexture(GL_TEXTURE_2D, g.solaire.backTexture);

    glBegin(GL_QUADS);
    glTexCoord2f(g.solaire.xc[0], g.solaire.yc[1]); glVertex2i(g.xres - 200, 0);
    glTexCoord2f(g.solaire.xc[0], g.solaire.yc[0]); glVertex2i(g.xres - 200, 200);
    glTexCoord2f(g.solaire.xc[1], g.solaire.yc[0]); glVertex2i(g.xres, 200);
    glTexCoord2f(g.solaire.xc[1], g.solaire.yc[1]); glVertex2i(g.xres, 0);
    glEnd();
    glDisable(GL_ALPHA_TEST);
    // draw abyss
    glBindTexture(GL_TEXTURE_2D, g.abyss.backTexture);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.0f);
    glColor4ub(255,255,255,255);
    glBindTexture(GL_TEXTURE_2D, g.abyss.backTexture);
    glBegin(GL_QUADS);
    glTexCoord2f(g.abyss.xc[0], g.abyss.yc[1]); glVertex2i(0, 0);
    glTexCoord2f(g.abyss.xc[0], g.abyss.yc[0]); glVertex2i(0, 200);
    glTexCoord2f(g.abyss.xc[1], g.abyss.yc[0]); glVertex2i(200, 200);
    glTexCoord2f(g.abyss.xc[1], g.abyss.yc[1]); glVertex2i(200, 0);
    glEnd();
    glDisable(GL_ALPHA_TEST);
}

void move_background()
{
    //move the background
    g.tex.xc[0] += 0.001;
    g.tex.xc[1] += 0.001;
}

void physics()
{
}

void render()
{
    glClear(GL_COLOR_BUFFER_BIT);
    glColor3f(1.0, 1.0, 1.0);
}













