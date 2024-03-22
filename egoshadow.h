#ifndef EGO_HEADER
#define EGO_HEADER

//defined types
typedef double Flt;
typedef double Vec[3];
typedef Flt	Matrix[4][4];

//macros
#define rnd() (((double)rand())/(double)RAND_MAX)
#define random(a) (rand()%a)
#define MakeVector(v, x, y, z) (v)[0]=(x),(v)[1]=(y),(v)[2]=(z)
#define VecCopy(a,b) (b)[0]=(a)[0];(b)[1]=(a)[1];(b)[2]=(a)[2]
#define VecDot(a,b)	((a)[0]*(b)[0]+(a)[1]*(b)[1]+(a)[2]*(b)[2])
#define VecSub(a,b,c) (c)[0]=(a)[0]-(b)[0]; \
                             (c)[1]=(a)[1]-(b)[1]; \
(c)[2]=(a)[2]-(b)[2]

extern void play_game();

class Image;

class Sprite {
    public:
        int onoff;
        int frame;
        double delay;
        Vec pos;
        Image *image;
        GLuint tex;
        struct timespec time;
        Sprite();
};

class Global {
    public:
        unsigned char keys[65536];
        int xres, yres;
        int movie, movieStep;
        int walk;
        int walkFrame;
        int isJumping;
        int jumpFrame;
        int maxJumpFrames;
        float max_hp;
        float current_hp;
        double transitionTime;
        double elapsedTime;
        double delay;
        int show_name;
        int statistics;
        int show_credits;
        Image *walkImage;
        GLuint walkTexture;
        Vec box[20];
        Sprite exp;
        Sprite exp44;
        Vec ball_pos;
        Vec ball_vel;
        //camera is centered at (0,0) lower-left of screen. 
        Flt camera[2];
        ~Global();
        Global();
};

class X11_wrapper {
    private:
    public:
        Display *dpy;
        Window win;
        GC gc;
        ~X11_wrapper();
        void setTitle();
        void setupScreenRes(const int w, const int h);
        X11_wrapper();
        void reshapeWindow(int width, int height);
        void checkResize(XEvent *e);
        bool getXPending();
        XEvent getXNextEvent();
        void swapBuffers();
};

class Image {
    public:
        int width, height;
        unsigned char *data;
        ~Image();
        Image(const char *fname);
};

#endif