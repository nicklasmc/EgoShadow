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
#include <string>
#include <vector>
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <ctime>
////

#define ALPHA 1;
#define LIGHT_BLUE 0x79d7fd;
#define BLUE 0x00bbfa;
#define NAVY 0x00183e;
#define RED 0xe10000;
#define BLACK 0x000000;
#define WHITE 0xffffff;

// Define constants for maximum HP values
const int MAX_BOSS_HP = 1000;
const int MAX_BOSS_SP = 100;

const int JOKER_HP = 159;
const int JOKER_SP = 101;
const int MONA_HP = 139;
const int MONA_SP = 105;
const int PANTHER_HP = 171;
const int PANTHER_SP = 95;
const int SKULL_HP = 205;
const int SKULL_SP = 127;
class Global;
class X11_wrapper;
class Character;
int check_keys2(XEvent *e);
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
Image img[17] = {
  "fixed_titlecard.png", 
  "solaire.png", 
  "abyss.png", 
  "ds.jpg", 
  "mona2.png", 
  "joker.png", 
  "pantherfixed.png", 
  "skullfixed.png", 
  "arsene.png",
  "monahead.png",
  "jokerhead.png",
  "pantherhead.png",
  "skullhead.png",
  "arsenetext.png",
  "jokertext.png",
  "monatext.png",
  "panthertext.png"
  };

class Texture {
  public:
    Image *backImage;
    GLuint backTexture;
    float xc[2];
    float yc[2];
};


class Global {
  public:
    bool introDone;
    int xres, yres;
    int show_credits;
    Texture tex;
    Texture solaire;
    Texture abyss;
    Texture city;
    Texture mona;
    Texture joker;
    Texture panther;
    Texture skull;
    Texture arsene;
    Texture jokertext;
    Texture monatext;
    Texture panthertext;

    Texture monahead;
    Texture jokerhead;
    Texture pantherhead;
    Texture skullhead;
    Texture arsenetext;


		Global() {
			//xres=1024, yres=1024;
			xres=1080, yres=720;
			show_credits = 0;
		}
} g;

class BossHealthBar { 
	public:
		float max_hp;
		float current_health;
		float previous_health;
		float hb_length;
		float percentage;
		int hb_max_length;
		int hb_container_length;
	
	
	BossHealthBar() {
		hb_container_length = g.xres/2;
		hb_max_length = g.xres/2 - 10;
		max_hp = MAX_BOSS_HP;
		current_health = MAX_BOSS_HP;
		previous_health = current_health;
		percentage =  current_health / max_hp;
		hb_length = hb_max_length * percentage;

	}
} bossBar;


class X11_wrapper {
	public:
		Global *global;
		Display *dpy;
		Window win;
		GLXContext glc;
		X11_wrapper() {
			GLint att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
			//GLint att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, None };
			setup_screen_res(1080, 720);
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
			XStoreName(dpy, win, "4490 Game Dev");
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

class heroSprite {
public:
  int s1x, s2x, s3x, s4x; // sprite1x, sprite2x, etc. for x coords
  int s1y, s2y, s3y, s4y; // sprite1y, sprite2y, etc. for y coords
  int width;
  int height;
  heroSprite()
  {
    width = ((g.xres / 2) / 4) - 10;
    s1x = (g.xres / 2) + 10;
    s2x = s1x + width;
    s3x = s2x + width;
    s4x = s3x + width;

    height = g.yres / 4 - 10;
    s1y = height;
    s2y = height;
    s3y = height;
    s4y = height;
  }
} hs;

class heroHeads {
public:
  int width, height, gap;
  int h1x, h2x, h3x, h4x; // head1x, head2x, etc. for x coords
  int h1y, h2y, h3y, h4y; // head1y, head2y, etc. for y coords
  heroHeads()
  {
    width = 75;
    height = 75;
    gap = 40;
    h1x = (g.xres) - width - 10;
    h2x = h1x;
    h3x = h2x;
    h4x = h3x;

    h1y = g.yres - height - 10;
    h2y = h1y - height - gap;
    h3y = h2y - height - gap;
    h4y = h3y - height - gap;
  }
} hh;


class heroHealthBars {
public:
		float max_hp;
		float current_health;
		float previous_health;

		int hb_length;
		float percentage;
		int hb_max_length;
		int hb_container_length;
    int lower_bound;
    int upper_bound;
    int actual_length;
    int reduce_factor;
    int max_actual_length;
    int target_length;

  heroHealthBars(int hp, int length, int lower) {

    current_health = hp;
    previous_health = hp;
    max_hp = hp;

    upper_bound = length;
    lower_bound = lower;
    actual_length = upper_bound - lower_bound;
    max_actual_length = actual_length;

    reduce_factor = actual_length;
    percentage = current_health / max_hp;
    
    hb_max_length = length;
    hb_length = hb_max_length * percentage;
    target_length = actual_length;
  }
};

heroHealthBars monaHB(MONA_HP, (hh.h1x + hh.width - 2), (hh.h1x + 2));
heroHealthBars jokerHB(JOKER_HP, (hh.h2x + hh.width - 2), (hh.h2x + 2));
heroHealthBars pantherHB(PANTHER_HP, (hh.h3x + hh.width - 2), (hh.h3x + 2));
heroHealthBars skullHB(SKULL_HP, (hh.h4x + hh.width - 2), (hh.h4x + 2));


class Character {
  private:
  public:
    enum ActionList { JOKER_ACTIONS, MONA_ACTIONS, PANTHER_ACTIONS, SKULL_ACTIONS, BOSS_ACTIONS };
    ActionList actionList;

    // Character Constructor
    Character(std::string _name, int _hp, int _max_hp, int _sp, int _max_sp, std::string _weaknesses, std::string _resistances, ActionList _actionList, bool _isBoss)
      : name(_name), hp(_hp), max_hp(_max_hp), sp(_sp), max_sp(_max_sp), weakness(_weaknesses), resistance(_resistances), isDowned(false), actionList(_actionList), isBoss(_isBoss) {}

    int hp;
    int sp; // Spell points
    int max_hp;
    int max_sp;
    std::string name;
    std::string weakness;
    std::string resistance;
    bool isDowned;
    bool isBoss;


    // Member function to reduce character's HP
    void takeDamage(int damage) {
      hp -= damage;
      if (hp <= 0) {
        hp = 0;  // Ensure HP doesn't go below zero
        isDowned = true; // Set isDowned to true if HP hits zero
      }
    }

    void healDamage(int heal) {
      if (!isDowned) { // Check if the character is not downed
        int potential_hp = hp + heal;
        if (potential_hp > max_hp) {
          potential_hp = max_hp; // Cap healing at max_hp
          std::cout << "Healed " << (max_hp - hp) << " HP.\n";
          hp = potential_hp;
        } else {
          std::cout << "Healed " << heal << " HP.\n";
          hp = potential_hp;
        }
      } else {
        std::cout << name << " is downed and cannot be healed.\n";
      }
    }

    // Member function to reduce character's SP
    void reduceSP(int spellCost) {
      sp -= spellCost;
      std::cout << "Cost " << spellCost << " SP.\n\n";
      if (sp < 0) {
        sp = 0;  // Ensure SP doesn't go below zero
      }
    }

    // Member function to heal character's SP
    void healSP(int healSP) {
      int potential_sp = sp + healSP;
      if (potential_sp > max_sp) {
        potential_sp = max_sp; // Cap healing at max_sp
        std::cout << "Restored " << (max_sp - sp) << " SP.\n";
        sp = potential_sp;
      } else {
        std::cout << "Restored " << healSP << " SP.\n";
        sp = potential_sp;
      }
    }

    // Member function to check if the character is downed
    bool isCharacterDowned() {
      return isDowned;
    }

    //------------------------------------------------------ ALL SKILLS

    // Member function to perform a physical attack
    void bash(Character& caster, Character& target) {
      if (!caster.isDowned) {
        if (!target.isDowned) {
          std::cout << caster.name << " uses Bash!\n";
          srand(time(nullptr));
          int miss_chance = rand() % 20 + 1; // Random number between 1 and 20 for miss chance
          if (miss_chance < 5) {
            std::cout << caster.name << "'s attack misses!\n";
          } else {
            int damage = rand() % 26 + 100;
            int critical = rand() % 20 + 1;
            if (critical == 20) {
              // If critical, double damage
              damage *= 2;
              std::cout << caster.name << " lands a critical hit! \n";
            }
            std::cout << target.name << " takes " << damage << " damage \n\n";
            target.takeDamage(damage);
            if (target.name == "MONA") {
                monaHB.percentage = target.hp / monaHB.max_hp;
                monaHB.target_length = monaHB.max_actual_length * monaHB.percentage;
            } else if (target.name == "JOKER") {
                jokerHB.percentage = target.hp / jokerHB.max_hp;
                jokerHB.target_length = jokerHB.max_actual_length * jokerHB.percentage;
            }
            else if (target.name == "PANTHER") {
                pantherHB.percentage = target.hp / pantherHB.max_hp;
                pantherHB.target_length = pantherHB.max_actual_length * pantherHB.percentage;
            } else if (target.name == "SKULL") {
                skullHB.percentage = target.hp / skullHB.max_hp;
                skullHB.target_length = skullHB.max_actual_length * skullHB.percentage;
            }
            //HP cost for phys skills
            caster.takeDamage(20);
            if (caster.name == "MONA")
            {
              monaHB.percentage = target.hp / monaHB.max_hp;
              monaHB.target_length = monaHB.max_actual_length * monaHB.percentage;
            }
            else if (caster.name == "JOKER")
            {
              jokerHB.percentage = target.hp / jokerHB.max_hp;
              jokerHB.target_length = jokerHB.max_actual_length * jokerHB.percentage;
            }
            else if (caster.name == "PANTHER")
            {
              pantherHB.percentage = target.hp / pantherHB.max_hp;
              pantherHB.target_length = pantherHB.max_actual_length * pantherHB.percentage;
            }
            else if (caster.name == "SKULL")
            {
              skullHB.percentage = target.hp / skullHB.max_hp;
              skullHB.target_length = skullHB.max_actual_length * skullHB.percentage;
            }
          }
        }
        else {
          std::cout << target.name << " is downed and cannot take damage!\n\n";
        }
      }
      else {
        std::cout << caster.name << " is downed and cannot act!\n\n";
      }
    }

    void cleave(Character& caster, Character& target) {
      if (!caster.isDowned) {
        if (!target.isDowned) {
          std::cout << caster.name << " uses Cleave!\n";
          srand(time(nullptr));
          int miss_chance = rand() % 20 + 1; // Random number between 1 and 20 for miss chance
          if (miss_chance < 5) {
            std::cout << caster.name << "'s attack misses!\n";
          } else {
            int damage = rand() % 21 + 90;
            int critical = rand() % 20 + 1;
            if (critical == 20) {
              // If critical, double damage
              damage *= 2;
              std::cout << caster.name << " lands a critical hit! \n";
            }
            std::cout << target.name << " takes " << damage << " damage \n\n";
            target.takeDamage(damage);
            caster.takeDamage(25);
            if (target.name == "MONA")
            {

              monaHB.percentage = target.hp / monaHB.max_hp;
              monaHB.target_length = monaHB.max_actual_length * monaHB.percentage;
              std::cout << "Target length: " << monaHB.target_length << " Actual Length: " << monaHB.actual_length << std::endl;

            }
            else if (target.name == "JOKER")
            {
              jokerHB.percentage = target.hp / jokerHB.max_hp;
              jokerHB.target_length = jokerHB.max_actual_length * jokerHB.percentage;
              
            }
            else if (target.name == "PANTHER")
            {
              pantherHB.percentage = target.hp / pantherHB.max_hp;
              pantherHB.target_length = pantherHB.max_actual_length * pantherHB.percentage;
            }
            else if (target.name == "SKULL")
            {
              skullHB.percentage = target.hp / skullHB.max_hp;
              skullHB.target_length = skullHB.max_actual_length * skullHB.percentage;
            }
          }
        }
        else {
          std::cout << target.name << " is downed and cannot take damage!\n\n";
        }
      }
      else {
        std::cout << caster.name << " is downed and cannot act!\n\n";
      }
    }

    void shot(Character& caster, Character& target) {
      if (!caster.isDowned) {
        if (!target.isDowned) {
          std::cout << caster.name << " uses shot!\n";
          srand(time(nullptr));
          int miss_chance = rand() % 20 + 1; // Random number between 1 and 20 for miss chance
          if (miss_chance < 5) {
            std::cout << caster.name << "'s attack misses!\n";
          } else {
            int damage = rand() % 21 + 90;
            int critical = rand() % 20 + 1;
            if (critical == 20) {
              // If critical, double damage
              damage *= 2;
              std::cout << caster.name << " lands a critical hit! \n";
            }
            std::cout << target.name << " takes " << damage << " damage \n\n";
            target.takeDamage(damage);
            caster.takeDamage(22);
            if (target.name == "MONA")
            {
              
              monaHB.percentage = target.hp / monaHB.max_hp;
              monaHB.target_length = monaHB.max_actual_length * monaHB.percentage;
              std::cout << "Target length: " << monaHB.target_length << " Actual Length: " << monaHB.actual_length << std::endl;
            }
            else if (target.name == "JOKER")
            {
              jokerHB.percentage = target.hp / jokerHB.max_hp;
              jokerHB.target_length = jokerHB.max_actual_length * jokerHB.percentage;
            }
            else if (target.name == "PANTHER")
            {
              pantherHB.percentage = target.hp / pantherHB.max_hp;
              pantherHB.target_length = pantherHB.max_actual_length * pantherHB.percentage;
            }
            else if (target.name == "SKULL")
            {
              skullHB.percentage = target.hp / skullHB.max_hp;
              skullHB.target_length = skullHB.max_actual_length * skullHB.percentage;
            }
          }
        }
        else {
          std::cout << target.name << " is downed and cannot take damage!\n\n";
        }
      }
      else {
        std::cout << caster.name << " is downed and cannot act!\n\n";
      }
    }

    void solar(Character& caster, Character& target) {
      if (!caster.isDowned) {
        if (!target.isDowned) {
          if (caster.sp >= 10) {
            std::cout << caster.name << " casts solar!\n";
            int damage;
            if (caster.isBoss) {
              // If the caster is a boss, calculate different damage
              damage = rand() % 21 + 40; // Different calculation for boss damage
            } else {
                // If not a boss, use regular damage calculation
              damage = rand() % 41 + 80;
            }
            // Check if the target is weak to fire
            if (target.weakness == "fire") {
              // If the target is weak to fire, increase damage
              damage *= 2;
              std::cout << target.name << " is weak! \n";
            } else if (target.resistance == "fire"){
              // If the target is resistant to fire, decrease damage
              damage /= 2;
              std::cout << target.name << " resists! \n";
            }
            std::cout << target.name << " takes " << damage << " damage \n";
            target.takeDamage(damage);
            caster.reduceSP(10);
            if (target.name == "MONA")
            {
              monaHB.percentage = target.hp / monaHB.max_hp;
              monaHB.target_length = monaHB.max_actual_length * monaHB.percentage;
              std::cout << "Target length: " << monaHB.target_length << " Actual Length: " << monaHB.actual_length << std::endl;
            }
            else if (target.name == "JOKER")
            {
              jokerHB.percentage = target.hp / jokerHB.max_hp;
              jokerHB.target_length = jokerHB.max_actual_length * jokerHB.percentage;
            }
            else if (target.name == "PANTHER")
            {
              pantherHB.percentage = target.hp / pantherHB.max_hp;
              pantherHB.target_length = pantherHB.max_actual_length * pantherHB.percentage;
            }
            else if (target.name == "SKULL")
            {
              skullHB.percentage = target.hp / skullHB.max_hp;
              skullHB.target_length = skullHB.max_actual_length * skullHB.percentage;
            }
          } else {
            std::cout << caster.name << " Not enough SP to cast the spell!\n\n";
          }
        }
        else {
          std::cout << target.name << " is downed and cannot take damage!\n\n";
        }
      }
      else {
        std::cout << caster.name << " is downed and cannot act!\n\n";
      }
    }

    void tornado(Character& caster, Character& target) {
      if (!caster.isDowned) {
        if (!target.isDowned) {
          if (caster.sp >= 10) {
            std::cout << caster.name << " casts tornado!\n";
            int damage;
            if (caster.isBoss) {
              // If the caster is a boss, calculate different damage
              damage = rand() % 21 + 40; // Different calculation for boss damage
            } else {
                // If not a boss, use regular damage calculation
              damage = rand() % 41 + 80;
            }
            // Check if the target is weak to wind
            if (target.weakness == "wind") {
              // If the target is weak to wind, increase damage
              damage *= 2;
              std::cout << target.name << " is weak! \n";
            } else if (target.resistance == "wind"){
              // If the target is resistant to wind, decrease damage
              damage /= 2;
              std::cout << target.name << " resists! \n";
            }
            std::cout << target.name << " takes " << damage << " damage \n";
            target.takeDamage(damage);
            caster.reduceSP(10);
            if (target.name == "MONA")
            {
              monaHB.percentage = target.hp / monaHB.max_hp;
              monaHB.target_length = monaHB.max_actual_length * monaHB.percentage;
              std::cout << "Target length: " << monaHB.target_length << " Actual Length: " << monaHB.actual_length << std::endl;
            }
            else if (target.name == "JOKER")
            {
              jokerHB.percentage = target.hp / jokerHB.max_hp;
              jokerHB.target_length = jokerHB.max_actual_length * jokerHB.percentage;
            }
            else if (target.name == "PANTHER")
            {
              pantherHB.percentage = target.hp / pantherHB.max_hp;
              pantherHB.target_length = pantherHB.max_actual_length * pantherHB.percentage;
            }
            else if (target.name == "SKULL")
            {
              skullHB.percentage = target.hp / skullHB.max_hp;
              skullHB.target_length = skullHB.max_actual_length * skullHB.percentage;
            }
          } else {
            std::cout << caster.name << " Not enough SP to cast the spell!\n\n";
          }
        }
        else {
          std::cout << target.name << " is downed and cannot take damage!\n\n";
        }
      }
      else {
        std::cout << caster.name << " is downed and cannot act!\n\n";
      }
    }

    void thunder(Character& caster, Character& target) {
      if(!caster.isDowned) {
        if (!target.isDowned) {
          if (caster.sp >= 10) {
            std::cout << caster.name << " casts thunder!\n";
            int damage;
            if (caster.isBoss) {
              // If the caster is a boss, calculate different damage
              damage = rand() % 21 + 40; // Different calculation for boss damage
            } else {
                // If not a boss, use regular damage calculation
              damage = rand() % 41 + 80;
            }
            // Check if the target is weak to electric
            if (target.weakness == "electric") {
              // If the target is weak to electric, increase damage
              damage *= 2;
              std::cout << target.name << " is weak! \n";
            } else if (target.resistance == "electric"){
              // If the target is resistant to electric, decrease damage
              damage /= 2;
              std::cout << target.name << " resists! \n";
            }
            std::cout << target.name << " takes " << damage << " damage \n";
            target.takeDamage(damage);
            caster.reduceSP(10);
                        if (target.name == "MONA") {
                monaHB.percentage = target.hp / monaHB.max_hp;
                monaHB.target_length = monaHB.max_actual_length * monaHB.percentage;
                std::cout << "Target length: " << monaHB.target_length << " Actual Length: " << monaHB.actual_length << std::endl;
            } else if (target.name == "JOKER") {
                jokerHB.percentage = target.hp / jokerHB.max_hp;
                jokerHB.target_length = jokerHB.max_actual_length * jokerHB.percentage;
            }
            else if (target.name == "PANTHER") {
                pantherHB.percentage = target.hp / pantherHB.max_hp;
                pantherHB.target_length = pantherHB.max_actual_length * pantherHB.percentage;
            } else if (target.name == "SKULL") {
                skullHB.percentage = target.hp / skullHB.max_hp;
                skullHB.target_length = skullHB.max_actual_length * skullHB.percentage;
            }
          } else {
            std::cout << caster.name << " Not enough SP to cast the spell!\n\n";
          }
        }
        else {
          std::cout << target.name << " is downed and cannot take damage!\n\n";
        }
      }
      else {
        std::cout << caster.name << " is downed and cannot act!\n\n";
      }
    }

    void blizzard(Character& caster, Character& target) {
      if (!caster.isDowned) {
        if (!target.isDowned) {
          if (caster.sp >= 10) {
            std::cout << caster.name << " casts blizzard!\n";
            int damage;
            if (caster.isBoss) {
              // If the caster is a boss, calculate different damage
              damage = rand() % 21 + 40; // Different calculation for boss damage
            } else {
                // If not a boss, use regular damage calculation
              damage = rand() % 41 + 80;
            }
            // Check if the target is weak to ice
            if (target.weakness == "ice") {
              // If the target is weak to ice, increase damage
              damage *= 2;
              std::cout << target.name << " is weak! \n";
            } else if (target.resistance == "ice"){
              // If the target is resistant to ice, decrease damage
              damage /= 2;
              std::cout << target.name << " resists! \n";
            }
            std::cout << target.name << " takes " << damage << " damage \n";
            target.takeDamage(damage);
            caster.reduceSP(10);
            if (target.name == "MONA")
            {
              monaHB.percentage = target.hp / monaHB.max_hp;
              monaHB.target_length = monaHB.max_actual_length * monaHB.percentage;
              std::cout << "Target length: " << monaHB.target_length << " Actual Length: " << monaHB.actual_length << std::endl;
            }
            else if (target.name == "JOKER")
            {
              jokerHB.percentage = target.hp / jokerHB.max_hp;
              jokerHB.target_length = jokerHB.max_actual_length * jokerHB.percentage;
            }
            else if (target.name == "PANTHER")
            {
              pantherHB.percentage = target.hp / pantherHB.max_hp;
              pantherHB.target_length = pantherHB.max_actual_length * pantherHB.percentage;
            }
            else if (target.name == "SKULL")
            {
              skullHB.percentage = target.hp / skullHB.max_hp;
              skullHB.target_length = skullHB.max_actual_length * skullHB.percentage;
            }
          } else {
            std::cout << caster.name << " Not enough SP to cast the spell!\n\n";
          }
        }
        else {
          std::cout << target.name << " is downed and cannot take damage!\n\n";
        }
      }
      else {
        std::cout << caster.name << " is downed and cannot act!\n\n";
      }
    }

    void cure(Character& caster, Character& target) {
      if (!caster.isDowned) {
        if (!target.isDowned) {
          if (target.hp < target.max_hp) {
            if (caster.sp >= 10) {
              std::cout << caster.name << " casts cure!\n";
              int heal = rand() % 5 + 21;
              std::cout << target.name << " restores " << heal << " health\n";
              target.healDamage(heal);
              caster.reduceSP(10);
            } else {
              std::cout << caster.name << " Not enough SP to cast the spell!\n\n";
            }
          } else {
            std::cout << target.name << " is already at full health!\n\n";
          }
        }
        else {
          std::cout << target.name << " is downed and cannot regain health!\n\n";
        }
      }
      else {
        std::cout << caster.name << " is downed and cannot act!\n\n";
      }
    }

    // Heal all party members
    void cureAll(Character& caster, Character characters[]) {
      if (!caster.isDowned) {
        bool anyDamaged = false; // Flag to track if any character is damaged
        for (int i = 0; i < 4; ++i) {
          if (characters[i].hp < characters[i].max_hp && !characters[i].isDowned) {
            anyDamaged = true;
            break; // If any character is damaged and not downed, no need to check further
          }
        }

        if (!anyDamaged) {
          std::cout << "All party members are either at full health or downed!\n\n";
          return; // Exit the function early if all damaged characters are downed or at full health
        }
        if (!anyDamaged) {
          std::cout << "All party members are either at full health or downed!\n\n";
          return; // Exit the function early if all damaged characters are downed or at full health
        }

        if (caster.sp >= 10) {
          std::cout << caster.name << " casts cure all!\n";
          srand(time(NULL));
          for (int i = 0; i < 4; ++i) {
            if (characters[i].hp < characters[i].max_hp && !characters[i].isDowned) {
              int heal = rand() % 5 + 21;
              characters[i].healDamage(heal);
              std::cout << characters[i].name << " restores " << heal << " health\n";
            }
          }
          caster.reduceSP(10);
        } else {
          std::cout << caster.name << " Not enough SP to cast the spell!\n\n";
        }
      }
      else {
        std::cout << caster.name << " is downed and cannot act!\n\n";
      }
    }

    void spDrop(Character& caster, Character& target) {
      if (!caster.isDowned) {
        if (!target.isDowned) {
          if (target.sp < target.max_sp) {
            if (caster.sp >= 10) {
              std::cout << caster.name << " uses an SP Drop!\n";
              int healSP = rand() % 5 + 21;
              std::cout << target.name << " restores " << healSP << " SP\n";
              target.healSP(healSP);
            } else {
              std::cout << caster.name << " Not enough SP to cast the spell!\n\n";
            }
          } else {
            std::cout << target.name << " is already at full SP!\n\n";
          }
        }
        else {
          std::cout << target.name << " is downed and cannot regain SP!\n\n";
        }
      }
      else {
        std::cout << caster.name << " is downed and cannot act!\n\n";
      }
    }

    void rampage(Character& caster, Character& target) {
      if (!caster.isDowned) {
        if (!target.isDowned) {
          std::cout << caster.name << " uses rampage!\n";
          srand(time(nullptr));
          for (int i = 0; i < 3; i++) {
            int miss_chance = rand() % 20 + 1; // Random number between 1 and 20 for miss chance
            if (miss_chance < 5) {
              std::cout << caster.name << "'s attack misses!\n";
            } else {
              int damage = rand() % 14 + 15;
              int critical = rand() % 20 + 1;
              if (critical == 20) {
                // If critical, double damage
                damage *= 2;
                std::cout << caster.name << " lands a critical hit! \n";
              }
              std::cout << target.name << " takes " << damage << " damage \n\n";
              target.takeDamage(damage);
              if (target.name == "MONA")
              {
                monaHB.percentage = target.hp / monaHB.max_hp;
                monaHB.target_length = monaHB.max_actual_length * monaHB.percentage;
                std::cout << "Target length: " << monaHB.target_length << " Actual Length: " << monaHB.actual_length << std::endl;
              }
              else if (target.name == "JOKER")
              {
                jokerHB.percentage = target.hp / jokerHB.max_hp;
                jokerHB.target_length = jokerHB.max_actual_length * jokerHB.percentage;
              }
              else if (target.name == "PANTHER")
              {
                pantherHB.percentage = target.hp / pantherHB.max_hp;
                pantherHB.target_length = pantherHB.max_actual_length * pantherHB.percentage;
              }
              else if (target.name == "SKULL")
              {
                skullHB.percentage = target.hp / skullHB.max_hp;
                skullHB.target_length = skullHB.max_actual_length * skullHB.percentage;
              }
            }
          }
        }
        else {
          std::cout << target.name << " is downed and cannot take damage!\n\n";
        }
      }
      else {
        std::cout << caster.name << " is downed and cannot act!\n\n";
      }
    }

    void gKnife(Character& caster, Character& target) {
      if (!caster.isDowned) {
        if (!target.isDowned) {
          std::cout << caster.name << " uses Golden Knife!\n";
          srand(time(nullptr));
          int miss_chance = rand() % 20 + 1; // Random number between 1 and 20 for miss chance
          if (miss_chance < 5) {
            std::cout << caster.name << "'s attack misses!\n";
          } else {
            int damage = rand() % 14 + 15;
            int critical = rand() % 20 + 1;
            if (critical == 20) {
              // If critical, double damage
              damage *= 2;
              std::cout << caster.name << " lands a critical hit! \n";
            }
            std::cout << target.name << " takes " << damage << " damage \n\n";
            target.takeDamage(damage);
            if (target.name == "MONA")
            {
              monaHB.percentage = target.hp / monaHB.max_hp;
              monaHB.target_length = monaHB.max_actual_length * monaHB.percentage;
              std::cout << "Target length: " << monaHB.target_length << " Actual Length: " << monaHB.actual_length << std::endl;
            }
            else if (target.name == "JOKER")
            {
              jokerHB.percentage = target.hp / jokerHB.max_hp;
              jokerHB.target_length = jokerHB.max_actual_length * jokerHB.percentage;
            }
            else if (target.name == "PANTHER")
            {
              pantherHB.percentage = target.hp / pantherHB.max_hp;
              pantherHB.target_length = pantherHB.max_actual_length * pantherHB.percentage;
            }
            else if (target.name == "SKULL")
            {
              skullHB.percentage = target.hp / skullHB.max_hp;
              skullHB.target_length = skullHB.max_actual_length * skullHB.percentage;
            }
          }
        }
        else {
          std::cout << target.name << " is downed and cannot take damage!\n\n";
        }
      }
      else {
        std::cout << caster.name << " is downed and cannot act!\n\n";
      }
    }

    void almighty(Character& caster, Character characters[]) {
      if (!caster.isDowned) {
        if (caster.sp >= 10) {
          std::cout << caster.name << " casts almighty!\n";
          srand(time(NULL));
          for (int i = 0; i < 4; ++i) {
            if (!characters[i].isDowned) {
              int damage = rand() % 26 + 40;
              characters[i].takeDamage(damage);
              std::cout << characters[i].name << " takes " << damage << " damage \n";
            } else {
              std::cout << characters[i].name << " is downed and cannot take damage!\n";
            }
          }
          caster.reduceSP(10);
        } else {
          std::cout << caster.name << " Not enough SP to cast the spell!\n\n";
        }
      }
      else {
        std::cout << caster.name << " is downed and cannot act!\n\n";
      }
    }

    void revive(Character& caster, Character& target) {
      if (!caster.isDowned) {
        if (target.isDowned) {
          std::cout << caster.name << " casts revive on " << target.name << "!\n";
          target.isDowned = false; // Revive the target by setting isDowned to false
          int heal = target.max_hp / 2; // Heal the target to half of their max HP
          std::cout << target.name << " has been revived!" << "!\n\n";
          target.healDamage(heal);
        } else {
          std::cout << target.name <<" is not incapacitated!\n";
        }
      }
      else {
        std::cout << caster.name << " is downed and cannot act!\n\n";
      }
    }


    void selectAction(Character characters[], Character boss[], int characterIndex) {
      // Implement action selection based on the character's action list
      switch (characters[characterIndex].actionList) {
        // JOKER's Turns
        case JOKER_ACTIONS: {
                              int action = NULL;
                              bool validAction = false;
                              while (!validAction){
                                std::cout << "What should JOKER do?\n";
                                std::cout << "1. Blizzard\n";
                                std::cout << "2. Cleave\n";
                                std::cout << "3. Cure\n";
                                //std::cin >> action;
                                while (!action) {
                                  XEvent e = x11.getXNextEvent();
                                  action = check_keys2(&e);
                                }
                                std::cout << "checkKeys2 is " << action << std::endl;
                                switch (action) {
                                  // Blizzard
                                  case 1: {
                                            characters[0].blizzard(characters[0], boss[0]);
                                            validAction = true;
                                            break;
                                          }
                                          // Bash
                                  case 2: {
                                            characters[0].cleave(characters[0], boss[0]);
                                            validAction = true;
                                            break;
                                          }
                                          // Cure
                                  case 3: {
                                            int target = NULL;
                                            bool validTarget = false;
                                            while (!validTarget) {
                                              std::cout << "Select a target:\n";
                                              std::cout << "1. MONA\n";
                                              std::cout << "2. PANTHER\n";
                                              std::cout << "3. SKULL\n";
                                              while (!target)
                                              {
                                                XEvent e = x11.getXNextEvent();
                                                target = check_keys2(&e);
                                              }
                                              switch (target) {
                                                case 1:
                                                  characters[0].cure(characters[0], characters[1]);
                                                  validTarget = true;
                                                  break;
                                                case 2:
                                                  characters[0].cure(characters[0], characters[2]);
                                                  validTarget = true;
                                                  break;
                                                case 3:
                                                  characters[0].cure(characters[0], characters[3]);
                                                  validTarget = true;
                                                  break;
                                                default:
                                                  std::cout << "Invalid target!\n";
                                                  break;
                                              }
                                            }
                                            validAction = true;
                                            break;
                                          }
                                  default:
                                          std::cout << "Invalid action!\n";
                                          break;
                                }
                              }
                              break;
                            }
                            // MONA's Turns
        case MONA_ACTIONS: {
                             // Implement MONA's action selection logic
                             int action = NULL;
                             bool validAction = false;
                             while (!validAction){
                               std::cout << "What should MONA do?\n";
                               std::cout << "1. Tornado\n";
                               std::cout << "2. Cure\n";
                               std::cout << "3. Revive\n";
                               while (!action)
                               {
                                 XEvent e = x11.getXNextEvent();
                                 action = check_keys2(&e);
                               }

                               switch (action)
                               {
                                 case 1:
                                   {
                                     characters[1].tornado(characters[1], boss[0]);
                                     validAction = true;
                                     break;
                                   }
                                 case 2: {
                                           int target = NULL;
                                           bool validTarget = false;
                                           while (!validTarget) {
                                             std::cout << "Select a target:\n";
                                             std::cout << "1. JOKER\n";
                                             std::cout << "2. PANTHER\n";
                                             std::cout << "3. SKULL\n";
                                             while (!target)
                                             {
                                               XEvent e = x11.getXNextEvent();
                                               target = check_keys2(&e);
                                             }
                                             switch (target) {
                                               case 1:
                                                 characters[1].cure(characters[1], characters[0]);
                                                 validTarget = true;
                                                 break;
                                               case 2:
                                                 characters[1].cure(characters[1], characters[2]); 
                                                 validTarget = true;
                                                 break;
                                               case 3:
                                                 characters[1].cure(characters[1], characters[3]);
                                                 validTarget = true;
                                                 break;
                                               default:
                                                 std::cout << "Invalid target!\n";
                                                 break;
                                             }
                                           }
                                           validAction = true;
                                           break;
                                         }
                                 case 3: {
                                           int target = NULL;
                                           bool validTarget = false;
                                           while (!validTarget) {
                                             std::cout << "Select a target:\n";
                                             std::cout << "1. JOKER\n";
                                             std::cout << "2. PANTHER\n";
                                             std::cout << "3. SKULL\n";
                                             while (!target)
                                             {
                                               XEvent e = x11.getXNextEvent();
                                               target = check_keys2(&e);
                                             }
                                             switch (target) {
                                               case 1:
                                                 characters[1].revive(characters[1], characters[0]);
                                                 validTarget = true;
                                                 break;
                                               case 2:
                                                 characters[1].revive(characters[1], characters[2]); 
                                                 validTarget = true;
                                                 break;
                                               case 3:
                                                 characters[1].revive(characters[1], characters[3]);
                                                 validTarget = true;
                                                 break;
                                               default:
                                                 std::cout << "Invalid target!\n";
                                                 break;
                                             }
                                           }
                                           validAction = true;
                                           break;
                                         }
                                 default:
                                         std::cout << "Invalid action!\n";
                                         break;
                               }
                             }
                             break;
                           }
                           // PANTHER's Turns
        case PANTHER_ACTIONS: {
                                int action = NULL;
                                bool validAction = false;
                                while (!validAction){
                                  std::cout << "What should PANTHER do?\n";
                                  std::cout << "1. SOLAR\n";
                                  std::cout << "2. SHOT\n";
                                  std::cout << "3. Cure\n";
                                  while (!action)
                                  {
                                    XEvent e = x11.getXNextEvent();
                                    action = check_keys2(&e);
                                  }

                                  switch (action)
                                  {
                                    case 1:
                                      {
                                        characters[2].solar(characters[2], boss[0]);
                                        validAction = true;
                                        break;
                                      }
                                    case 2: {
                                              characters[2].shot(characters[2], boss[0]);
                                              validAction = true;
                                              break;
                                            }
                                    case 3: {
                                              int target = NULL;
                                              bool validTarget = false;
                                              while (!validTarget) {
                                                std::cout << "Select a target:\n";
                                                std::cout << "1. JOKER\n";
                                                std::cout << "2. MONA\n";
                                                std::cout << "3. SKULL\n";
                                                while (!target)
                                                {
                                                  XEvent e = x11.getXNextEvent();
                                                  target = check_keys2(&e);
                                                }
                                                switch (target)
                                                {
                                                  case 1:
                                                    characters[2].cure(characters[2], characters[0]);
                                                    validTarget = true;
                                                    break;
                                                  case 2:
                                                    characters[2].cure(characters[2], characters[1]);
                                                    validTarget = true;
                                                    break;
                                                  case 3:
                                                    characters[2].cure(characters[2], characters[3]);
                                                    validTarget = true;
                                                    break;
                                                  default:
                                                    std::cout << "Invalid target!\n";
                                                    break;
                                                }
                                              }
                                              validAction = true;
                                              break;
                                            }
                                    default:
                                            std::cout << "Invalid action!\n";
                                            break;
                                  }
                                }
                                break;
                              }
                              // SKULL's Turns
        case SKULL_ACTIONS: {
                              int action = NULL;
                              bool validAction = false;
                              while (!validAction){
                                std::cout << "What should SKULL do?\n";
                                std::cout << "1. THUNDER\n";
                                std::cout << "2. CLEAVE\n";
                                std::cout << "3. BASH\n";
                                while (!action)
                                {
                                  XEvent e = x11.getXNextEvent();
                                  action = check_keys2(&e);
                                }

                                switch (action)
                                {
                                  case 1:
                                    {
                                      characters[3].thunder(characters[3], boss[0]);
                                      validAction = true;
                                      break;
                                    }
                                  case 2: {
                                            characters[3].cleave(characters[3], boss[0]);
                                            validAction = true;
                                            break;
                                          }
                                  case 3: {
                                            characters[3].bash(characters[3], boss[0]);
                                            validAction = true;
                                            break;
                                          }
                                  default:
                                          std::cout << "Invalid action!\n";
                                          break;
                                }
                              }
                              break;
                            }
                            // BOSS's Turns
                            case BOSS_ACTIONS: {
                                std::cout << "ARSENE's TURN\n";
                                int randomAction = rand() % 8 + 1; // Random number between 1 and 8 for the available actions
                                switch (randomAction) {
                                    // Heal Himself
                                    case 1: {
                                        boss[0].cure(boss[0], boss[0]);
                                        break;
                                    }
                                    // Perform rampage
                                    case 2: {
                                      int target = rand() % 4 + 1;
                                      switch (target) {
                                        case 1:
                                          boss[0].rampage(boss[0], characters[0]);
                                          break;
                                        case 2:
                                          boss[0].rampage(boss[0], characters[1]);
                                          break;
                                        case 3:
                                          boss[0].rampage(boss[0], characters[2]);
                                          break;
                                        case 4:
                                          boss[0].rampage(boss[0], characters[3]);
                                          break;
                                      }
                                      break;
                                    }
                                    // Perform gKnife
                                    case 3: {
                                      int target = rand() % 4 + 1;
                                      switch (target) {
                                        case 1:
                                          boss[0].gKnife(boss[0], characters[0]);
                                          break;
                                        case 2:
                                          boss[0].gKnife(boss[0], characters[1]);
                                          break;
                                        case 3:
                                          boss[0].gKnife(boss[0], characters[2]);
                                          break;
                                        case 4:
                                          boss[0].gKnife(boss[0], characters[3]);
                                          break;
                                      }
                                      break;
                                    }
                                    // Perform solar
                                    case 4: {
                                      int target = rand() % 4 + 1;
                                      switch (target) {
                                        case 1:
                                          boss[0].solar(boss[0], characters[0]);
                                          break;
                                        case 2:
                                          boss[0].solar(boss[0], characters[1]);
                                          break;
                                        case 3:
                                          boss[0].solar(boss[0], characters[2]);
                                          break;
                                        case 4:
                                          boss[0].solar(boss[0], characters[3]);
                                          break;
                                      }
                                      break;
                                    }
                                    // Perform tornado
                                    case 5: {
                                      int target = rand() % 4 + 1;
                                      switch (target) {
                                        case 1:
                                          boss[0].tornado(boss[0], characters[0]);
                                          break;
                                        case 2:
                                          boss[0].tornado(boss[0], characters[1]);
                                          break;
                                        case 3:
                                          boss[0].tornado(boss[0], characters[2]);
                                          break;
                                        case 4:
                                          boss[0].tornado(boss[0], characters[3]);
                                          break;
                                      }
                                      break;
                                    }
                                    // Perform thunder
                                    case 6: {
                                      int target = rand() % 4 + 1;
                                      switch (target) {
                                        case 1:
                                          boss[0].thunder(boss[0], characters[0]);
                                          break;
                                        case 2:
                                          boss[0].thunder(boss[0], characters[1]);
                                          break;
                                        case 3:
                                          boss[0].thunder(boss[0], characters[2]);
                                          break;
                                        case 4:
                                          boss[0].thunder(boss[0], characters[3]);
                                          break;
                                      }
                                      break;
                                    }
                                    // Perform blizzard
                                    case 7: {
                                      int target = rand() % 4 + 1;
                                      switch (target) {
                                        case 1:
                                          boss[0].blizzard(boss[0], characters[0]);
                                          break;
                                        case 2:
                                          boss[0].blizzard(boss[0], characters[1]);
                                          break;
                                        case 3:
                                          boss[0].blizzard(boss[0], characters[2]);
                                          break;
                                        case 4:
                                          boss[0].blizzard(boss[0], characters[3]);
                                          break;
                                      }
                                      break;
                                    }
                                    // Perform almighty
                                    case 8: {
                                        boss[0].almighty(boss[0], characters);
                                        break;
                                    }
                                }
                                break;
                            }
      }
    }

};

class gameLogic {
  public: 
    bool turnDone;
    bool battleDone;
    int currentActor;
    int gameOrder[5];
    gameLogic() {
      battleDone = 0;
      turnDone = 0;
      currentActor = 0;
      for (int i = 0; i < 5; i++) {
        gameOrder[i] = i;
      }
    }
} game;





Character characters[4] = {
  Character("JOKER", JOKER_HP, JOKER_HP, JOKER_SP, JOKER_SP, "ice", "fire", Character::JOKER_ACTIONS, false),
  Character("MONA", MONA_HP, MONA_HP, MONA_SP, MONA_SP, "electric", "wind", Character::MONA_ACTIONS, false),
  Character("PANTHER", PANTHER_HP, PANTHER_HP, PANTHER_SP, PANTHER_SP, "wind", "fire", Character::PANTHER_ACTIONS, false),
  Character("SKULL", SKULL_HP, SKULL_HP, SKULL_SP, SKULL_SP, "fire", "electric", Character::SKULL_ACTIONS, false),
};

Character boss[1] = {
  Character("ARSENE", MAX_BOSS_HP, MAX_BOSS_HP, MAX_BOSS_SP, MAX_BOSS_SP, "none", "none", Character::BOSS_ACTIONS, true)
};



void init_opengl(void);
void check_mouse(XEvent *e);
int check_keys(XEvent *e);
void check_keys_intro(XEvent *e);
void physics(void);
void render(void);
void render_actual();
void play_game();
void init();
void background();
void move_background();
void display_credits();
void display_battleMenu();
void display_game_over();
void display_options();
void return_to_menu();
void display_menu();
void display_startup();
void display_hp();
int* generate_initiative(int);
void render_screen();
void display_bossHealthBar();
void reduce_bossHealthBar();
void reduce_monaHB();
void reduce_jokerHB();
void reduce_pantherHB();
void reduce_skullHB();

void render_currentHero();
void render_monaSprite();
void render_jokerSprite();
void render_pantherSprite();
void render_skullSprite();
void render_heroHeads();
void render_boss();

int* generate_initiative(int arr[]) {
  std::random_shuffle(&arr[0], &arr[5]);
  return arr;
}




//===========================================================================
//===========================================================================
int main()
{
  init_opengl();
  display_startup();
  // Access and manipulate individual characters
  for (int i = 0; i < 4; ++i) {
    std::cout << "Character " << i + 1 << " name: " << characters[i].name << std::endl;
    std::cout << "Character " << i + 1 << " HP: " << characters[i].hp << std::endl;
    std::cout << "Target length: " << monaHB.target_length << " Actual Length: " << monaHB.actual_length << std::endl;
    std::cout << std::endl;
  }

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
  // OpenGL initialization
  glViewport(0, 0, g.xres, g.yres);
  // Initialize matrices
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  // This sets 2D mode (no perspective)
  glOrtho(0, g.xres, 0, g.yres, -1, 1);
  // Clear the screen
  glClearColor(1.0, 1.0, 1.0, 1.0);
  // glClear(GL_COLOR_BUFFER_BIT);
  // Do this to allow texture maps
  glEnable(GL_TEXTURE_2D);
  initialize_fonts();
  //
  // load the images file into a ppm structure.
  //
  // BACKGROUND GENERATION
  // create opengl texture elements
  /*
     g.tex.backImage = &img[0];
     glGenTextures(1, &g.tex.backTexture);
     int w = g.tex.backImage->width;
     int h = g.tex.backImage->height;
     glBindTexture(GL_TEXTURE_2D, g.tex.backTexture);
     glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
     glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
     glTexImage2D(GL_TEXTURE_2D, 0, 3, w, h, 0,
     GL_RGB, GL_UNSIGNED_BYTE, g.tex.backImage->data);
     g.tex.xc[0] = 0.0;
     g.tex.xc[1] = 1.0;
     g.tex.yc[0] = 0.0;
     g.tex.yc[1] = 1.0;
     */

  /* -------- WORKING PNG VERSION -----------*/
  unsigned char *data0 = buildAlphaData(&img[0]);
  // unsigned char *data2 = new unsigned char[h * w * 4];
  g.tex.backImage = &img[0];
  glGenTextures(1, &g.tex.backTexture);
  int w = g.tex.backImage->width;
  int h = g.tex.backImage->height;
  glBindTexture(GL_TEXTURE_2D, g.tex.backTexture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D, 0, 4, w, h, 0,
               GL_RGBA, GL_UNSIGNED_BYTE, data0);
  // g.tex.xc[0] = 1.0;
  // g.tex.xc[1] = 0.0;
  // g.tex.yc[0] = 1.0;
  // g.tex.yc[1] = 0.0;
  /*-----------------------------------------*/

  unsigned char *data1 = buildAlphaData(&img[1]);
  // unsigned char *data2 = new unsigned char[h * w * 4];
  g.solaire.backImage = &img[1];
  glGenTextures(1, &g.solaire.backTexture);
  w = g.solaire.backImage->width;
  h = g.solaire.backImage->height;
  glBindTexture(GL_TEXTURE_2D, g.solaire.backTexture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D, 0, 4, w, h, 0,
               GL_RGBA, GL_UNSIGNED_BYTE, data1);
  g.solaire.xc[0] = 0.0;
  g.solaire.xc[1] = 1.0;
  g.solaire.yc[0] = 0.0;
  g.solaire.yc[1] = 1.0;

  unsigned char *data2 = buildAlphaData(&img[2]);
  // abyss
  // unsigned char *data2 = new unsigned char[h * w * 4];
  g.abyss.backImage = &img[2];
  glGenTextures(1, &g.abyss.backTexture);
  w = g.abyss.backImage->width;
  h = g.abyss.backImage->height;
  glBindTexture(GL_TEXTURE_2D, g.abyss.backTexture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D, 0, 4, w, h, 0,
               GL_RGBA, GL_UNSIGNED_BYTE, data2);
  g.abyss.xc[0] = 0.0;
  g.abyss.xc[1] = 1.0;
  g.abyss.yc[0] = 0.0;
  g.abyss.yc[1] = 1.0;

  // city
  g.city.backImage = &img[3];
  glGenTextures(1, &g.city.backTexture);
  w = g.city.backImage->width;
  h = g.city.backImage->height;
  glBindTexture(GL_TEXTURE_2D, g.city.backTexture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D, 0, 3, w, h, 0,
               GL_RGB, GL_UNSIGNED_BYTE, g.city.backImage->data);
  g.city.xc[0] = 0.0;
  g.city.xc[1] = 1.0;
  g.city.yc[0] = 0.0;
  g.city.yc[1] = 1.0;

  // mona
  g.mona.backImage = &img[4];
  glGenTextures(1, &g.mona.backTexture);
  w = g.mona.backImage->width;
  h = g.mona.backImage->height;
  glBindTexture(GL_TEXTURE_2D, g.mona.backTexture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D, 0, 3, w, h, 0,
               GL_RGB, GL_UNSIGNED_BYTE, g.mona.backImage->data);
  g.mona.xc[0] = 0.0;
  g.mona.xc[1] = 1.0;
  g.mona.yc[0] = 0.0;
  g.mona.yc[1] = 1.0;

  // joker
  g.joker.backImage = &img[5];
  glGenTextures(1, &g.joker.backTexture);
  w = g.joker.backImage->width;
  h = g.joker.backImage->height;
  glBindTexture(GL_TEXTURE_2D, g.joker.backTexture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D, 0, 3, w, h, 0,
               GL_RGB, GL_UNSIGNED_BYTE, g.joker.backImage->data);
  g.joker.xc[0] = 0.0;
  g.joker.xc[1] = 1.0;
  g.joker.yc[0] = 0.0;
  g.joker.yc[1] = 1.0;

  // panther
  g.panther.backImage = &img[6];
  glGenTextures(1, &g.panther.backTexture);
  w = g.panther.backImage->width;
  h = g.panther.backImage->height;
  glBindTexture(GL_TEXTURE_2D, g.panther.backTexture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D, 0, 3, w, h, 0,
               GL_RGB, GL_UNSIGNED_BYTE, g.panther.backImage->data);
  g.panther.xc[0] = 0.0;
  g.panther.xc[1] = 1.0;
  g.panther.yc[0] = 0.0;
  g.panther.yc[1] = 1.0;

  // skull
  g.skull.backImage = &img[7];
  glGenTextures(1, &g.skull.backTexture);
  w = g.skull.backImage->width;
  h = g.skull.backImage->height;
  glBindTexture(GL_TEXTURE_2D, g.skull.backTexture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D, 0, 3, w, h, 0,
               GL_RGB, GL_UNSIGNED_BYTE, g.skull.backImage->data);
  g.skull.xc[0] = 0.0;
  g.skull.xc[1] = 1.0;
  g.skull.yc[0] = 0.0;
  g.skull.yc[1] = 1.0;

  // arsene
  g.arsene.backImage = &img[8];
  glGenTextures(1, &g.arsene.backTexture);
  w = g.arsene.backImage->width;
  h = g.arsene.backImage->height;
  glBindTexture(GL_TEXTURE_2D, g.arsene.backTexture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D, 0, 3, w, h, 0,
               GL_RGB, GL_UNSIGNED_BYTE, g.arsene.backImage->data);
  g.arsene.xc[0] = 0.0;
  g.arsene.xc[1] = 1.0;
  g.arsene.yc[0] = 0.0;
  g.arsene.yc[1] = 1.0;

  g.monahead.backImage = &img[9];
  glGenTextures(1, &g.monahead.backTexture);
  w = g.monahead.backImage->width;
  h = g.monahead.backImage->height;
  glBindTexture(GL_TEXTURE_2D, g.monahead.backTexture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D, 0, 3, w, h, 0,
               GL_RGB, GL_UNSIGNED_BYTE, g.monahead.backImage->data);
  g.monahead.xc[0] = 0.0;
  g.monahead.xc[1] = 1.0;
  g.monahead.yc[0] = 0.0;
  g.monahead.yc[1] = 1.0;

  g.jokerhead.backImage = &img[10];
  glGenTextures(1, &g.jokerhead.backTexture);
  w = g.jokerhead.backImage->width;
  h = g.jokerhead.backImage->height;
  glBindTexture(GL_TEXTURE_2D, g.jokerhead.backTexture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D, 0, 3, w, h, 0,
               GL_RGB, GL_UNSIGNED_BYTE, g.jokerhead.backImage->data);
  g.jokerhead.xc[0] = 0.0;
  g.jokerhead.xc[1] = 1.0;
  g.jokerhead.yc[0] = 0.0;
  g.jokerhead.yc[1] = 1.0;

  g.pantherhead.backImage = &img[11];
  glGenTextures(1, &g.pantherhead.backTexture);
  w = g.pantherhead.backImage->width;
  h = g.pantherhead.backImage->height;
  glBindTexture(GL_TEXTURE_2D, g.pantherhead.backTexture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D, 0, 3, w, h, 0,
               GL_RGB, GL_UNSIGNED_BYTE, g.pantherhead.backImage->data);
  g.pantherhead.xc[0] = 0.0;
  g.pantherhead.xc[1] = 1.0;
  g.pantherhead.yc[0] = 0.0;
  g.pantherhead.yc[1] = 1.0;

  g.skullhead.backImage = &img[12];
  glGenTextures(1, &g.skullhead.backTexture);
  w = g.skullhead.backImage->width;
  h = g.skullhead.backImage->height;
  glBindTexture(GL_TEXTURE_2D, g.skullhead.backTexture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D, 0, 3, w, h, 0,
               GL_RGB, GL_UNSIGNED_BYTE, g.skullhead.backImage->data);
  g.skullhead.xc[0] = 0.0;
  g.skullhead.xc[1] = 1.0;
  g.skullhead.yc[0] = 0.0;
  g.skullhead.yc[1] = 1.0;


  // unsigned char *data13 = buildAlphaData(&img[13]);
  g.arsenetext.backImage = &img[13];
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glGenTextures(1, &g.arsenetext.backTexture);
  w = g.arsenetext.backImage->width;
  h = g.arsenetext.backImage->height;
  glBindTexture(GL_TEXTURE_2D, g.arsenetext.backTexture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D, 0, 3, w, h, 0,
               GL_RGB, GL_UNSIGNED_BYTE, g.arsenetext.backImage->data);
  g.arsenetext.xc[0] = 0.0;
  g.arsenetext.xc[1] = 1.0;
  g.arsenetext.yc[0] = 0.0;
  g.arsenetext.yc[1] = 1.0;

  unsigned char *data14 = buildAlphaData(&img[14]);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  g.jokertext.backImage = &img[14];
  glGenTextures(1, &g.jokertext.backTexture);
  w = g.jokertext.backImage->width;
  h = g.jokertext.backImage->height;
  glBindTexture(GL_TEXTURE_2D, g.jokertext.backTexture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0,
               GL_RGB, GL_UNSIGNED_BYTE, g.jokertext.backImage->data);
  g.jokertext.xc[0] = 0.0;
  g.jokertext.xc[1] = 1.0;
  g.jokertext.yc[0] = 0.0;
  g.jokertext.yc[1] = 1.0;

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  g.monatext.backImage = &img[15];
  glGenTextures(1, &g.monatext.backTexture);
  w = g.monatext.backImage->width;
  h = g.monatext.backImage->height;
  glBindTexture(GL_TEXTURE_2D, g.monatext.backTexture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0,
               GL_RGB, GL_UNSIGNED_BYTE, g.monatext.backImage->data);
  g.monatext.xc[0] = 0.0;
  g.monatext.xc[1] = 1.0;
  g.monatext.yc[0] = 0.0;
  g.monatext.yc[1] = 1.0;

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  g.panthertext.backImage = &img[15];
  glGenTextures(1, &g.panthertext.backTexture);
  w = g.panthertext.backImage->width;
  h = g.panthertext.backImage->height;
  glBindTexture(GL_TEXTURE_2D, g.panthertext.backTexture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0,
               GL_RGB, GL_UNSIGNED_BYTE, g.panthertext.backImage->data);
  g.panthertext.xc[0] = 0.0;
  g.panthertext.xc[1] = 1.0;
  g.panthertext.yc[0] = 0.0;
  g.panthertext.yc[1] = 1.0;
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
    switch (key) {
      case XK_1: 
        break;
      case XK_2:
        break;
      case XK_3:
        break;
      case XK_4:
        break;
      case XK_5:
        break;
      case XK_6:
        break;
      case XK_7:
        break;
      case XK_8:
        break;
      case XK_9:
        break;
      case XK_Escape:
        return 1;
    }
  }
  return 0;
}

int check_keys2(XEvent *e)
{
  //Was there input from the keyboard?
  if (e->type == KeyPress) {
    int key = XLookupKeysym(&e->xkey, 0);
    switch (key) {
      case XK_1:
        return 1;
        break;
      case XK_2:
        return 2;
        break;
      case XK_3:
        return 3;
        break;
      case XK_4:
        return 4;
        break;
    }
  }
  return 0;
}



//Added Play Game - Nicklas Chiang
void play_game()
{
	srand(time(NULL));
	int done = 0;
	// generate initiative
	generate_initiative(game.gameOrder);
	// -- Print Initiative For Debugging -- //
	for (int i = 0; i < 5; i++) {
		std::cout << game.gameOrder[i] << std::endl;
	}
	// -- ----------------------------- -- //
	int i = 0;
	while (!done) {
		while (x11.getXPending()) {
			XEvent e = x11.getXNextEvent();
			x11.check_resize(&e);
			check_mouse(&e);
			done = check_keys(&e);
		}
		// Combat sequence
		// Simulate combat rounds
		// for (int i = 0; i < 5; i++) {
		// 	std::cout << "Game order: " << game.gameOrder[i] << std::endl;
		// }
		game.currentActor = game.gameOrder[i];
		//game.currentActor = 4;
		game.turnDone = 0;
		while(!game.turnDone){
			render_actual();
			render();
			physics();

   
			x11.swapBuffers();
		}
		i++;
		if (i == 5) i = 0;
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

//Function to display hp for characters array
void display_hp()
{

	Rect r;
	unsigned int c = BLACK;

  // Set the position for displaying HP in the top right corner
  r.bot = g.yres - 20;
  r.left = g.xres - 150;
  r.center = 0;
  // Display HP and SP for each character in the global array
  for (int i = 0; i < 4; ++i) {
    int current_hp = characters[i].hp;
    int current_sp = characters[i].sp;
    int max_hp = characters[i].max_hp;
    // Calculate the percentage of health remaining
    if (current_hp == 0) {
      // If HP is zero, display "down" instead of percentage
      ggprint8b(&r, 16, c, "%s HP: Downed", characters[i].name.c_str());
    } else {
      // Calculate the percentage of health remaining
      int percentage = (int)(((float)current_hp / max_hp) * 100);
      // Display HP using ggprint8b function
      ggprint8b(&r, 16, c, "%s HP: %d/%d (%d%%)", characters[i].name.c_str(), current_hp, max_hp, percentage);
    }
    r.bot -= 20;
    // Display SP
    ggprint8b(&r, 16, c, "%s SP: %d/%d", characters[i].name.c_str(), current_sp, characters[i].max_sp);
    // Move to next line
    r.bot -= 20;
  }

  // Display HP and SP for the enemy character in the global array

  if (boss[0].hp == 0) {
    ggprint8b(&r, 16, 0xFF0000, "%s HP: Downed", boss[0].name.c_str());
  } else {
    int bossPercentage = (int)(((float)boss[0].hp / boss[0].max_hp) * 100);
    ggprint8b(&r, 16, 0xFF0000, "%s HP: %d/%d (%d%%)", boss[0].name.c_str(), boss[0].hp, boss[0].max_hp, bossPercentage);
  }
  r.bot -= 20;
  // Display SP
  ggprint8b(&r, 16, 0xFF0000, "%s SP: %d/%d", boss[0].name.c_str(), boss[0].sp, boss[0].max_sp);
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

// |
void display_battleMenu() {
  // ----- Red Box ----- //
  // draw in counter-clockwise order
  // botleft -> botright -> topright -> topleft
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// main container border
	glColor3ub(0, 24, 62);
	glBegin(GL_QUADS); 
	glVertex2i(0, g.yres/4); // topleft: (x,y)
	glVertex2i(g.xres + 10, g.yres/4); // topright: (x,y)
	glVertex2i(g.xres + 10, 0); // botright: (x,y)
	glVertex2i(0, 0); // botleft: (x,y)
	glEnd();
	// main container inner
	glColor3ub(255,255,255);
	glBegin(GL_QUADS); 
	glVertex2i(10, (g.yres/4) - 10); // topleft: (x,y)
	glVertex2i(g.xres - 10, (g.yres/4) - 10); // topright: (x,y)
	glVertex2i(g.xres - 10, 10); // botright: (x,y)
	glVertex2i(10,10); // botleft: (x,y)
	glDisable(GL_BLEND);
	glEnd();

  int width = 500;
  int height = 100;
  int x = 15;
  int y = (g.yres/4) - 10;

  glColor3f(1.0, 1.0, 1.0);
  glEnable(GL_TEXTURE_2D);

  if (game.currentActor == 0)
  {
    glBindTexture(GL_TEXTURE_2D, g.monatext.backTexture);
  }
  else if (game.currentActor == 1)
  {
    glBindTexture(GL_TEXTURE_2D, g.jokertext.backTexture);
  }
  else if (game.currentActor == 2)
  {
    glBindTexture(GL_TEXTURE_2D, g.panther.backTexture);
  }
  else if (game.currentActor == 3)
  {
    glBindTexture(GL_TEXTURE_2D, g.skull.backTexture);
  }
  else if (game.currentActor == 4)
  {
    return;
  }
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glBegin(GL_QUADS);
  // 1, 1 = top right
  // 1, 0 = bottom right
  // 0, 0 = bottom left
  // 0, 1 = top left
  glTexCoord2f(0.0, 1.0); glVertex2i(10,10); // botleft: (x,y)
  glTexCoord2f(1.0, 1.0); glVertex2i(g.xres - 10, 10); // botright: (x,y)
  glTexCoord2f(1.0, 0.0); glVertex2i(g.xres - 10, (g.yres/4) - 10); // topright: (x,y)
  glTexCoord2f(0.0, 0.0); 	glVertex2i(10, (g.yres/4) - 10); // topleft: (x,y)
	glDisable(GL_BLEND);
	glEnd();

  // x11.swapBuffers();
  // glXSwapBuffers(x11.dpy, x11.win);
  // usleep(1000);
  // std::cout << "Battle Menu init" << std::endl;
}

// void display_bossHealthBar() {
// 	glEnable(GL_BLEND);
// 	// boss health bar container
// 	glColor3ub(0, 0, 0);
// 	glBegin(GL_QUADS); 
// 	glVertex2i(20, g.yres-40); // topleft: (x,y)
// 	glVertex2i(bossBar.hb_container_length, g.yres-40); // topright: (x,y)
// 	glVertex2i(bossBar.hb_container_length, g.yres-80); // botright: (x,y)
// 	glVertex2i(20, g.yres-80); // botleft: (x,y)
// 	glEnd();


// 	bossBar.current_health = boss[0].hp;
// 	bossBar.percentage = bossBar.previous_health / bossBar.max_hp;
// 	bossBar.hb_length = bossBar.hb_max_length * bossBar.percentage;
// 	// std::cout << boss[0].max_hp << std::endl;
// 	// std::cout << boss[0].hp << std::endl;
// 	// std::cout << g.xres/2 - 10 << std::endl;
// 	// std::cout << "previous --->" << bossBar.previous_health << std::endl;
// 	// std::cout << "Current Percentage: " << bossBar.percentage << std::endl;
	

// 	// healthbar
// 	glBegin(GL_QUADS); 
// 	glColor3ub(238, 75, 62);
// 	glVertex2i(32, g.yres-45); // topleft: (x,y)
// 	glVertex2i(bossBar.hb_length, g.yres-45); // topright: (x,y) X NEEDS TO CHANGE 
// 	glVertex2i(bossBar.hb_length, g.yres-75); // botright: (x,y) X NEEDS TO CHANGE
// 	glVertex2i(32, g.yres-75); // botleft: (x,y)
// 	glEnd();
// 	glDisable(GL_BLEND);

// }

void reduce_bossHealthBar() {
		bossBar.previous_health = bossBar.previous_health - 2.0;
}

void reduce_monaHB()
{
  //std::cout << "<Mona> actual: " << monaHB.actual_length << "<Mona> Target: " << monaHB.target_length << std::endl;
  monaHB.actual_length = monaHB.actual_length - 1.0;
  monaHB.hb_length = monaHB.hb_length - 1.0;
}

void reduce_jokerHB()
{
  //std::cout << "Joker actual: " << jokerHB.actual_length << "Joker Target: " << jokerHB.target_length << std::endl;
  jokerHB.actual_length = jokerHB.actual_length - 1.0;
  jokerHB.hb_length = jokerHB.hb_length - 1.0;
}

void reduce_pantherHB()
{
  //std::cout << "Panther actual: " << pantherHB.actual_length << "Panther Target: " << pantherHB.target_length << std::endl;
  pantherHB.actual_length = pantherHB.actual_length - 1.0;
  pantherHB.hb_length = pantherHB.hb_length - 1.0;
}

void reduce_skullHB()
{
  //std::cout << "Skull actual: " << skullHB.actual_length << "Skull Target: " << skullHB.target_length << std::endl;
  skullHB.actual_length = skullHB.actual_length - 1.0;
  skullHB.hb_length = skullHB.hb_length - 1.0;
}

//Function to Display Main Menu
void display_menu() {
  int selectedOption = 0; // Track the selected option
  bool menuActive = true; // Control variable for the menu loop
  float alpha = 0.0;
  float introDuration = 5.0;
  float fadeRate = (1 / introDuration);
  // ---------------------------------Intro --------------------------------
  // Timers
  time_t startTime;
  time_t now;
  float elapsedTime = 0;
  int boxWidth = g.xres/2;
  int centerY = g.yres/2;
  int centerX = g.xres/2;
  int boxHeight = g.yres/3;
  time(&startTime);
  // for the square
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glClear(GL_COLOR_BUFFER_BIT);
  glClearColor(0, 0, 0, 1);
  // REMOVE TO SEE INTRO
  g.introDone = 1;
  if (!g.introDone) {
    while (elapsedTime < introDuration) {
      now = time(NULL);
      elapsedTime = difftime(now, startTime);

      glClear(GL_COLOR_BUFFER_BIT); // clear screen
      glClearColor(fadeRate, fadeRate, fadeRate, 1); 

      // ----- Text ----- //
      Rect r;
      r.bot = g.xres / 2;
      r.left = g.xres / 2;
      r.center = 1;
      ggprint16(&r, 20, 0x00ff0011, "EgoShadow");
      glPopMatrix();

      // ----- Red Box ----- //
      // draw in counter-clockwise order
      // botleft -> botright -> topright -> topleft
      glColor4f(1.0, 0.0, 0.0, alpha); // red
      glBegin(GL_QUADS); 
      glVertex2i(centerX - boxWidth/2, centerY + boxHeight/2 - 5); // topleft: (x,y)
      glVertex2i(centerX + boxWidth/2, centerY + boxHeight/2 - 5); // topright: (x,y)
      glVertex2i(centerX + boxWidth/2, centerY - boxHeight/2 - 5); // botright: (x,y) 
      glVertex2i(centerX - boxWidth/2, centerY - boxHeight/2 - 5); // botleft:  (x,y) -> center - width/2 
      glEnd();

      // ----- Texture on box ----- //
      glBindTexture(GL_TEXTURE_2D, g.city.backTexture);

      glBegin(GL_QUADS);
      // 0,0 = top left
      // 1,1 = bottom right
      glTexCoord2f(0.0, 0.0); glVertex2i(centerX - boxWidth/2, centerY + boxHeight/2 - 5);
      glTexCoord2f(1.0, 0.0); glVertex2i(centerX + boxWidth/2, centerY + boxHeight/2 - 5);
      glTexCoord2f(1.0, 1.0); glVertex2i(centerX + boxWidth/2, centerY - boxHeight/2 - 5);
      glTexCoord2f(0.0, 1.0); glVertex2i(centerX - boxWidth/2, centerY - boxHeight/2 - 5);
      glEnd();

      // draw white boxes to reveal logo
      // ------------------------------------------------------------------------------------------------------

      // ------------------------------------------------------------------------------------------------------



      alpha += .025;      
      fadeRate -=  .003;      
      glXSwapBuffers(x11.dpy, x11.win);
      usleep(1000); 
    }
    g.introDone = 1;
  }
  // ---------------------------------Intro --------------------------------
  glDisable(GL_BLEND);

  while (menuActive) {
    // Clear the screen
    //glClear(GL_COLOR_BUFFER_BIT);

    background();
    //move_background();

    // Display menu options
    Rect r;
    r.bot = g.yres / 2;
    r.left = g.xres / 2;
    r.center = 1;
    unsigned int color = 0xffffff;

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
  glClearColor(1.0, 1.0, 1.0, 1.0);
  glClear(GL_COLOR_BUFFER_BIT);

  glBindTexture(GL_TEXTURE_2D, g.tex.backTexture);
  glBegin(GL_QUADS);
  // 0,0 = top left
  // 1,1 = bottom right
  glTexCoord2f(0.0, 0.0); glVertex2i(0,g.yres);
  glTexCoord2f(1.0, 0.0); glVertex2i(g.xres, g.yres);
  glTexCoord2f(1.0, 1.0); glVertex2i(g.xres, 0);
  glTexCoord2f(0.0, 1.0); glVertex2i(0, 0);
  glEnd();

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

void render_heroHeads()
{
  // glColor3ub(1, 1, 1);
  glEnable(GL_TEXTURE_2D);

  // ====================== Mona ======================
  glBindTexture(GL_TEXTURE_2D, g.monahead.backTexture);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glBegin(GL_QUADS);
  // 1, 0 = bottom right
  // 1, 1 = top right
  // 0, 1 = top left
  // 0, 0 = bottom left
  glTexCoord2f(1.0, 0.0);
  glVertex2i(hh.h1x + hh.width, hh.h1y + hh.height);
  glTexCoord2f(1.0, 1.0);
  glVertex2i(hh.h1x + hh.width, hh.h1y);
  glTexCoord2f(0.0, 1.0);
  glVertex2i(hh.h1x, hh.h1y);
  glTexCoord2f(0.0, 0.0);
  glVertex2i(hh.h1x, hh.h1y + hh.height);
  glEnd();

  // health bar container
    glColor3ub(0, 0, 0);
  glBegin(GL_QUADS);
  glVertex2i(hh.h1x + hh.width, hh.h1y - 20); // botright
  glVertex2i(hh.h1x + hh.width, hh.h1y - 5);  // top right
  glVertex2i(hh.h1x, hh.h1y - 5);             // top left
  glVertex2i(hh.h1x, hh.h1y - 20);            // bot left
  glEnd();

  monaHB.current_health = characters[1].hp;

  // std::cout << "Percentage: " << monaHB.percentage << std::endl;
  // std::cout << "length: " << monaHB.hb_length << std::endl;
  // std::cout << "length after: " << monaHB.hb_length << std::endl;

  // health bar
  glBegin(GL_QUADS);
  glColor3ub(238, 75, 62);
  glVertex2i(monaHB.hb_length, hh.h1y - 18); // botright
  glVertex2i(monaHB.hb_length, hh.h1y - 7);  // top right
  glVertex2i(hh.h1x + 2, hh.h1y - 7);             // top left
  glVertex2i(hh.h1x + 2, hh.h1y - 18);            // bot left
  glEnd();
  // ==================================================

  glColor3f(1.0, 1.0, 1.0);

  // ====================== Joker ======================
  glBindTexture(GL_TEXTURE_2D, g.jokerhead.backTexture);
  glBegin(GL_QUADS);
  // 1, 0 = bottom right
  // 1, 1 = top right
  // 0, 1 = top left
  // 0, 0 = bottom left
  glTexCoord2f(1.0, 0.0);
  glVertex2i(hh.h2x + hh.width, hh.h2y + hh.height);
  glTexCoord2f(1.0, 1.0);
  glVertex2i(hh.h2x + hh.width, hh.h2y);
  glTexCoord2f(0.0, 1.0);
  glVertex2i(hh.h2x, hh.h2y);
  glTexCoord2f(0.0, 0.0);
  glVertex2i(hh.h2x, hh.h2y + hh.height);
  glEnd();

  jokerHB.current_health = characters[2].hp;

  // healthbar container
  glBegin(GL_QUADS);
  glColor3ub(0, 0, 0);
  glVertex2i(hh.h2x + hh.width, hh.h2y - 20); // botright
  glVertex2i(hh.h2x + hh.width, hh.h2y - 5);  // top right
  glVertex2i(hh.h2x, hh.h2y - 5);             // top left
  glVertex2i(hh.h2x, hh.h2y - 20);            // bot left
  glEnd();

  // health bar
  glBegin(GL_QUADS);
  glColor3ub(238, 75, 62);
  glVertex2i(jokerHB.hb_length, hh.h2y - 18); // botright
  glVertex2i(jokerHB.hb_length, hh.h2y - 7);  // top right
  glVertex2i(hh.h2x + 2, hh.h2y - 7);             // top left
  glVertex2i(hh.h2x + 2, hh.h2y - 18);            // bot left
  glEnd();
  // ===================================================

  glColor3f(1.0, 1.0, 1.0);

  // ====================== Panther ======================
  glBindTexture(GL_TEXTURE_2D, g.pantherhead.backTexture);
  glBegin(GL_QUADS);
  // 1, 0 = bottom right
  // 1, 1 = top right
  // 0, 1 = top left
  // 0, 0 = bottom left
  glTexCoord2f(1.0, 0.0);
  glVertex2i(hh.h3x + hh.width, hh.h3y + hh.height);
  glTexCoord2f(1.0, 1.0);
  glVertex2i(hh.h3x + hh.width, hh.h3y);
  glTexCoord2f(0.0, 1.0);
  glVertex2i(hh.h3x, hh.h3y);
  glTexCoord2f(0.0, 0.0);
  glVertex2i(hh.h3x, hh.h3y + hh.height);
  glEnd();

  // healthbar container
  glBegin(GL_QUADS);
  glColor3ub(0, 0, 0);
  glVertex2i(hh.h3x + hh.width, hh.h3y - 20); // botright
  glVertex2i(hh.h3x + hh.width, hh.h3y - 5);  // top right
  glVertex2i(hh.h3x, hh.h3y - 5);             // top left
  glVertex2i(hh.h3x, hh.h3y - 20);            // bot left
  glEnd();

  pantherHB.current_health = characters[3].hp;

  // health bar
  glBegin(GL_QUADS);
  glColor3ub(238, 75, 62);
  glVertex2i(pantherHB.hb_length, hh.h3y - 18); // botright
  glVertex2i(pantherHB.hb_length, hh.h3y - 7);  // top right
  glVertex2i(hh.h3x + 2, hh.h3y - 7);             // top left
  glVertex2i(hh.h3x + 2, hh.h3y - 18);            // bot left
  glEnd();
  // =====================================================

  glColor3f(1.0, 1.0, 1.0);

  // ====================== Skull ======================
  glBindTexture(GL_TEXTURE_2D, g.skullhead.backTexture);
  glBegin(GL_QUADS);
  // 1, 0 = bottom right
  // 1, 1 = top right
  // 0, 1 = top left
  // 0, 0 = bottom left
  glTexCoord2f(1.0, 0.0);
  glVertex2i(hh.h4x + hh.width, hh.h4y + hh.height);
  glTexCoord2f(1.0, 1.0);
  glVertex2i(hh.h4x + hh.width, hh.h4y);
  glTexCoord2f(0.0, 1.0);
  glVertex2i(hh.h4x, hh.h4y);
  glTexCoord2f(0.0, 0.0);
  glVertex2i(hh.h4x, hh.h4y + hh.height);
  glEnd();


  // healthbar container
  glBegin(GL_QUADS);
  glColor3ub(0, 0, 0);
  glVertex2i(hh.h4x + hh.width, hh.h4y - 20); // botright
  glVertex2i(hh.h4x + hh.width, hh.h4y - 5);  // top right
  glVertex2i(hh.h4x, hh.h4y - 5);             // top left
  glVertex2i(hh.h4x, hh.h4y - 20);            // bot left
  glEnd();

  skullHB.current_health = characters[4].hp;

  // health bar
  glBegin(GL_QUADS);
  glColor3ub(238, 75, 62);
  glVertex2i(skullHB.hb_length, hh.h4y - 18); // botright
  glVertex2i(skullHB.hb_length, hh.h4y - 7);  // top right
  glVertex2i(hh.h4x + 2, hh.h4y - 7);             // top left
  glVertex2i(hh.h4x + 2, hh.h4y - 18);            // bot left
  glEnd();

  // ===================================================

  glDisable(GL_BLEND);
  glDisable(GL_TEXTURE_2D);
}

void render_monaSprite()
{
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, g.mona.backTexture);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glBegin(GL_QUADS);
  // 1, 1 = top right
  // 1, 0 = bottom right
  // 0, 0 = bottom left
  // 0, 1 = top left
  glTexCoord2f(0.0, 0.0); glVertex2i(hs.s1x, hs.s1y);
  glTexCoord2f(1.0, 0.0); glVertex2i(hs.s1x + hs.width, hs.s1y);
  glTexCoord2f(1.0, 1.0); glVertex2i(hs.s1x + hs.width,  0);
  glTexCoord2f(0.0, 1.0); glVertex2i(hs.s1x, 0);


  glEnd();
  glDisable(GL_BLEND);
  glDisable(GL_TEXTURE_2D);
}
void render_jokerSprite()
{
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, g.joker.backTexture);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glBegin(GL_QUADS);
  // 1, 1 = top right
  // 1, 0 = bottom right
  // 0, 0 = bottom left
  // 0, 1 = top left
  glTexCoord2f(0.0, 0.0); glVertex2i(hs.s2x, hs.s2y);
  glTexCoord2f(1.0, 0.0); glVertex2i(hs.s2x + hs.width, hs.s2y);
  glTexCoord2f(1.0, 1.0); glVertex2i(hs.s2x + hs.width,  0);
  glTexCoord2f(0.0, 1.0); glVertex2i(hs.s2x, 0);


  glEnd();
  glDisable(GL_BLEND);
  glDisable(GL_TEXTURE_2D);
}

void render_pantherSprite()
{
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, g.panther.backTexture);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glBegin(GL_QUADS);
  // 1, 1 = top right
  // 1, 0 = bottom right
  // 0, 0 = bottom left
  // 0, 1 = top left
  glTexCoord2f(0.0, 0.0); glVertex2i(hs.s3x, hs.s3y);
  glTexCoord2f(1.0, 0.0); glVertex2i(hs.s3x + hs.width, hs.s3y);
  glTexCoord2f(1.0, 1.0); glVertex2i(hs.s3x + hs.width,  0);
  glTexCoord2f(0.0, 1.0); glVertex2i(hs.s3x, 0);


  glEnd();
  glDisable(GL_BLEND);
  glDisable(GL_TEXTURE_2D);
}
void render_skullSprite()
{
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, g.skull.backTexture);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glBegin(GL_QUADS);
  // 1, 1 = top right
  // 1, 0 = bottom right
  // 0, 0 = bottom left
  // 0, 1 = top left
  glTexCoord2f(0.0, 0.0); glVertex2i(hs.s4x, hs.s4y);
  glTexCoord2f(1.0, 0.0); glVertex2i(hs.s4x + hs.width, hs.s4y);
  glTexCoord2f(1.0, 1.0); glVertex2i(hs.s4x + hs.width,  0);
  glTexCoord2f(0.0, 1.0); glVertex2i(hs.s4x, 0);


  glEnd();
  glDisable(GL_BLEND);
  glDisable(GL_TEXTURE_2D);
}

void render_currentHero() {
  glEnable(GL_TEXTURE_2D);
  glColor3f(1.0, 1.0, 1.0);
  //std::cout << " Current actor: " << game.currentActor << std::endl;
  if (game.currentActor == 0) {
    glBindTexture(GL_TEXTURE_2D, g.mona.backTexture);
  }
  else if (game.currentActor == 1)
  {
    glBindTexture(GL_TEXTURE_2D, g.joker.backTexture);
  }
  else if (game.currentActor == 2)
  {
    glBindTexture(GL_TEXTURE_2D, g.panther.backTexture);
  }
  else if (game.currentActor == 3)
  {
    glBindTexture(GL_TEXTURE_2D, g.skull.backTexture);
  } 
  else if (game.currentActor == 4)
  {
    return;
  } 
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glBegin(GL_QUADS);
  // 1, 1 = top right
  // 1, 0 = bottom right
  // 0, 0 = bottom left
  // 0, 1 = top left
  int x = g.xres/2 + 200;
  int y = g.yres/4 + 10;
  int width = 150;
  int height = g.yres - 100;

  glTexCoord2f(0.0, 1.0); glVertex2i(x, y); // bot left
  glTexCoord2f(1.0, 1.0); glVertex2i(x + width, y); // bot right
  glTexCoord2f(1.0, 0.0); glVertex2i(x + width, height); // top right
  glTexCoord2f(0.0, 0.0); glVertex2i(x, height); // top left


  glEnd();
  glDisable(GL_BLEND);
  glDisable(GL_TEXTURE_2D);
}

void display_bossHealthBar() {
  glColor3f(1.0, 1.0, 1.0);
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, g.arsenetext.backTexture);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glBegin(GL_QUADS);
  // 1, 1 = top right
  // 1, 0 = bottom right
  // 0, 0 = bottom left
  // 0, 1 = top left
  int width = bossBar.hb_container_length / 2;
  glTexCoord2f(0.0, 1.0); glVertex2i(20, g.yres - 30); // bot left
  glTexCoord2f(1.0, 1.0); glVertex2i(20 + width, g.yres - 30); // bot right
  glTexCoord2f(1.0, 0.0); glVertex2i(20 + width,  g.yres - 5); // top right
  glTexCoord2f(0.0, 0.0); glVertex2i(20, g.yres - 5); // top left


  glEnd();
  glDisable(GL_BLEND);
  glDisable(GL_TEXTURE_2D);


	glEnable(GL_BLEND);

	glColor3ub(0, 0, 0);
	glBegin(GL_QUADS); 
	glVertex2i(20, g.yres-40); // topleft: (x,y)
	glVertex2i(bossBar.hb_container_length, g.yres-40); // topright: (x,y)
	glVertex2i(bossBar.hb_container_length, g.yres-80); // botright: (x,y)
	glVertex2i(20, g.yres-80); // botleft: (x,y)
	glEnd();


	bossBar.current_health = boss[0].hp;
	bossBar.percentage = bossBar.previous_health / bossBar.max_hp;
	bossBar.hb_length = bossBar.hb_max_length * bossBar.percentage;
	// std::cout << boss[0].max_hp << std::endl;
	// std::cout << boss[0].hp << std::endl;
	// std::cout << g.xres/2 - 10 << std::endl;
	// std::cout << "previous --->" << bossBar.previous_health << std::endl;
	// std::cout << "Current Percentage: " << bossBar.percentage << std::endl;
	

	// healthbar
	glBegin(GL_QUADS); 
	glColor3ub(238, 75, 62);
	glVertex2i(32, g.yres-45); // topleft: (x,y)
	glVertex2i(bossBar.hb_length, g.yres-45); // topright: (x,y) X NEEDS TO CHANGE 
	glVertex2i(bossBar.hb_length, g.yres-75); // botright: (x,y) X NEEDS TO CHANGE
	glVertex2i(32, g.yres-75); // botleft: (x,y)
	glEnd();
	glDisable(GL_BLEND);

}

void render_boss() {
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, g.arsene.backTexture);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glBegin(GL_QUADS);
  // 1, 1 = top right
  // 1, 0 = bottom right
  // 0, 0 = bottom left
  // 0, 1 = top left
  glTexCoord2f(1.0, 0.0);
  glVertex2i(g.xres / 2, g.yres / 4 + 10); // bot right
  glTexCoord2f(1.0, 1.0);
  glVertex2i(g.xres / 2, g.yres - 100); // top right
  glTexCoord2f(0.0, 1.0);
  glVertex2i(5, g.yres - 100); // top left
  glTexCoord2f(0.0, 0.0);
  glVertex2i(5, g.yres / 4 + 10); // bot left

  glEnd();
  glDisable(GL_BLEND);
  glDisable(GL_TEXTURE_2D);
}


void render_actual()
{
  glClearColor(1.0, 1.0, 1.0, 0.0);
  glClear(GL_COLOR_BUFFER_BIT);
  glColor3f(1.0, 1.0, 1.0);
  XEvent e = x11.getXNextEvent();
  x11.check_resize(&e);
  // render_monaSprite();
  // render_jokerSprite();
  // render_pantherSprite();
  // render_skullSprite();
  render_currentHero();
  display_battleMenu();
  render_boss();
  render_heroHeads();
  display_bossHealthBar();
  render_currentHero();

  x11.swapBuffers();
}
void render()
{

	Rect r;
	// Set the position for displaying HP in the top left corner
	r.bot = g.yres - 20;
	r.left = 20;
	r.center = 0;
	
	// display_game_over
	while (bossBar.current_health < bossBar.previous_health)
	{
		reduce_bossHealthBar();
		display_bossHealthBar();
		x11.swapBuffers();
	}

  while (monaHB.target_length < monaHB.actual_length)
	{
		reduce_monaHB();
    render_heroHeads();
		x11.swapBuffers();
	}
  while (jokerHB.target_length < jokerHB.actual_length)
	{
		reduce_jokerHB();
    render_heroHeads();
		x11.swapBuffers();
	}

    while (pantherHB.target_length < pantherHB.actual_length)
	{
		reduce_pantherHB();
    render_heroHeads();
		x11.swapBuffers();
	}
    while (skullHB.target_length < skullHB.actual_length)
	{
		reduce_skullHB();
    render_heroHeads();
		x11.swapBuffers();
	}


  std::cout << "Mona HP: " << characters[1].hp << std::endl;
  if (game.currentActor == 0) {
    characters[0].selectAction(characters, boss, 0);
    // ggprint8b(&r, 16, 0xFFFFFF, "press 1 for joker actions");
    r.bot -= 20;
    game.turnDone = 1;
  } else if (game.currentActor == 1) {
    characters[1].selectAction(characters, boss, 1);
    // ggprint8b(&r, 16, 0xFFFFFF, "press 2 for mona actions");
    r.bot -= 20;
    game.turnDone = 1;
  } else if (game.currentActor == 2) {
    characters[2].selectAction(characters, boss, 2);
    // ggprint8b(&r, 16, 0xFFFFFF, "press 3 for panther actions");
    r.bot -= 20;
    game.turnDone = 1;
  } else if (game.currentActor == 3) {
    characters[3].selectAction(characters, boss, 3);
    // ggprint8b(&r, 16, 0xFFFFFF, "press 4 for skull actions");
    r.bot -= 20;
    game.turnDone = 1;
  } else if (game.currentActor == 4) {
    boss[0].selectAction(characters, boss, 4);
    // ggprint8b(&r, 16, 0xFFFFFF, "press 5 for boss to attack all");
    r.bot -= 20;
    game.turnDone = 1;
  }
}













