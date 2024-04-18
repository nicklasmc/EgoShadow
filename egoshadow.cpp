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
Image img[4] = {"fixed_titlecard.png", "solaire.png", "abyss.png", "ds.jpg"};

class Texture {
    public:
        Image *backImage;
        GLuint backTexture;
        float xc[2];
        float yc[2];
};

class Character {
private:
public:
    enum ActionList { JOKER_ACTIONS, MONA_ACTIONS, PANTHER_ACTIONS, SKULL_ACTIONS, KAMOSHIDA_ACTIONS };
    ActionList actionList;

    // Character Constructor
    Character(std::string _name, int _hp, int _max_hp, int _sp, int _max_sp, std::string _weaknesses, std::string _resistances, ActionList _actionList)
    : name(_name), hp(_hp), max_hp(_max_hp), sp(_sp), max_sp(_max_sp), weakness(_weaknesses), resistance(_resistances), isDowned(false), actionList(_actionList) {}

    int hp;
    int sp; // Spell points
    int max_hp;
    int max_sp;
    std::string name;
    std::string weakness;
    std::string resistance;
    bool isDowned;
    int initiative;

    void rollInitiative() {
        initiative = rand() % 20 + 1; // Roll a d20 for initiative
    }


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
                int damage = rand() % 5 + 21;
                // Check if the target is weak to fire
                if (target.weakness == "strike") {
                    // If the target is weak to fire, increase damage
                    damage *= 2;
                    std::cout << target.name << " is weak! \n";
                } else if (target.resistance == "strike"){
                    // If the target is resistant to fire, decrease damage
                    damage /= 2;
                    std::cout << target.name << " resists! \n";
                }
                std::cout << target.name << " takes " << damage << " damage \n";
                target.takeDamage(damage);
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
                int damage = rand() % 5 + 21;
                // Check if the target is weak to fire
                if (target.weakness == "slash") {
                    // If the target is weak to fire, increase damage
                    damage *= 2;
                    std::cout << target.name << " is weak! \n";
                } else if (target.resistance == "slash"){
                    // If the target is resistant to fire, decrease damage
                    damage /= 2;
                    std::cout << target.name << " resists! \n";
                }
                std::cout << target.name << " takes " << damage << " damage \n";
                target.takeDamage(damage);
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
                int damage = rand() % 5 + 21;
                // Check if the target is weak to fire
                if (target.weakness == "pierce") {
                    // If the target is weak to fire, increase damage
                    damage *= 2;
                    std::cout << target.name << " is weak! \n";
                } else if (target.resistance == "pierce"){
                    // If the target is resistant to fire, decrease damage
                    damage /= 2;
                    std::cout << target.name << " resists! \n";
                }
                std::cout << target.name << " takes " << damage << " damage \n";
                target.takeDamage(damage);
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
                    int damage = rand() % 5 + 21;
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
                    int damage = rand() % 5 + 21;
                    // Check if the target is weak to fire
                    if (target.weakness == "wind") {
                        // If the target is weak to fire, increase damage
                        damage *= 2;
                        std::cout << target.name << " is weak! \n";
                    } else if (target.resistance == "wind"){
                        // If the target is resistant to fire, decrease damage
                        damage /= 2;
                        std::cout << target.name << " resists! \n";
                    }
                    std::cout << target.name << " takes " << damage << " damage \n";
                    target.takeDamage(damage);
                    caster.reduceSP(10);
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
                    int damage = rand() % 5 + 21;
                    // Check if the target is weak to fire
                    if (target.weakness == "electric") {
                        // If the target is weak to fire, increase damage
                        damage *= 2;
                        std::cout << target.name << " is weak! \n";
                    } else if (target.resistance == "electric"){
                        // If the target is resistant to fire, decrease damage
                        damage /= 2;
                        std::cout << target.name << " resists! \n";
                    }
                    std::cout << target.name << " takes " << damage << " damage \n";
                    target.takeDamage(damage);
                    caster.reduceSP(10);
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
                    int damage = rand() % 5 + 21;
                    // Check if the target is weak to fire
                    if (target.weakness == "ice") {
                        // If the target is weak to fire, increase damage
                        damage *= 2;
                        std::cout << target.name << " is weak! \n";
                    } else if (target.resistance == "ice"){
                        // If the target is resistant to fire, decrease damage
                        damage /= 2;
                        std::cout << target.name << " resists! \n";
                    }
                    std::cout << target.name << " takes " << damage << " damage \n";
                    target.takeDamage(damage);
                    caster.reduceSP(10);
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

    void almighty(Character& caster, Character characters[]) {
        if (!caster.isDowned) {
            if (caster.sp >= 10) {
                std::cout << caster.name << " casts almighty!\n";
                srand(time(NULL));
                for (int i = 0; i < 4; ++i) {
                    if (!characters[i].isDowned) {
                        int damage = rand() % 5 + 21;
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
                int action;
                bool validAction = false;
                while (!validAction){
                    std::cout << "What should JOKER do?\n";
                    std::cout << "1. Blizzard\n";
                    std::cout << "2. Cleave\n";
                    std::cout << "3. Cure\n";
                    std::cin >> action;
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
                            int target;
                            bool validTarget = false;
                            while (!validTarget) {
                                std::cout << "Select a target:\n";
                                std::cout << "1. MONA\n";
                                std::cout << "2. PANTHER\n";
                                std::cout << "3. SKULL\n";
                                std::cin >> target;
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
                int action;
                bool validAction = false;
                while (!validAction){
                    std::cout << "What should MONA do?\n";
                    std::cout << "1. Tornado\n";
                    std::cout << "2. Cure\n";
                    std::cout << "3. Revive\n";
                    std::cin >> action;
                    switch (action) {
                        case 1: {
                            characters[1].tornado(characters[1], boss[0]);
                            validAction = true;
                            break;
                        }
                        case 2: {
                            int target;
                            bool validTarget = false;
                            while (!validTarget) {
                                std::cout << "Select a target:\n";
                                std::cout << "1. JOKER\n";
                                std::cout << "2. PANTHER\n";
                                std::cout << "3. SKULL\n";
                                std::cin >> target;
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
                            int target;
                            bool validTarget = false;
                            while (!validTarget) {
                                std::cout << "Select a target:\n";
                                std::cout << "1. JOKER\n";
                                std::cout << "2. PANTHER\n";
                                std::cout << "3. SKULL\n";
                                std::cin >> target;
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
                int action;
                bool validAction = false;
                while (!validAction){
                    std::cout << "What should PANTHER do?\n";
                    std::cout << "1. SOLAR\n";
                    std::cout << "2. SHOT\n";
                    std::cout << "3. Cure\n";
                    std::cin >> action;
                    switch (action) {
                        case 1: {
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
                            int target;
                            bool validTarget = false;
                            while (!validTarget) {
                                std::cout << "Select a target:\n";
                                std::cout << "1. JOKER\n";
                                std::cout << "2. MONA\n";
                                std::cout << "3. SKULL\n";
                                std::cin >> target;
                                switch (target) {
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
                int action;
                bool validAction = false;
                while (!validAction){
                    std::cout << "What should SKULL do?\n";
                    std::cout << "1. THUNDER\n";
                    std::cout << "2. CLEAVE\n";
                    std::cout << "3. BASH\n";
                    std::cin >> action;
                    switch (action) {
                        case 1: {
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
            // KAMOSHIDA's Turns
            case KAMOSHIDA_ACTIONS:
                break;
        }
    }
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

        Character characters[4] = {
            Character("JOKER", JOKER_HP, JOKER_HP, JOKER_SP, JOKER_SP, "ice", "fire", Character::JOKER_ACTIONS),
            Character("MONA", MONA_HP, MONA_HP, MONA_SP, MONA_SP, "electric", "wind", Character::MONA_ACTIONS),
            Character("PANTHER", PANTHER_HP, PANTHER_HP, PANTHER_SP, PANTHER_SP, "wind", "fire", Character::PANTHER_ACTIONS),
            Character("SKULL", SKULL_HP, SKULL_HP, SKULL_SP, SKULL_SP, "fire", "electric", Character::SKULL_ACTIONS),
        };

        Character boss[1] = {
            Character("KAMOSHIDA", MAX_BOSS_HP, MAX_BOSS_HP, MAX_BOSS_SP, MAX_BOSS_SP, "none", "none", Character::KAMOSHIDA_ACTIONS)
        };


        Global() {
            //xres=1024, yres=1024;
            xres=2040, yres=2040;
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

void init_opengl(void);
void check_mouse(XEvent *e);
int check_keys(XEvent *e);
void check_keys_intro(XEvent *e);
void physics(void);
void render(void);
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
bool compareInitiative(const Character& a, const Character& b);

//===========================================================================
//===========================================================================
int main()
{
    init_opengl();
    display_startup();
    // Access and manipulate individual characters
    for (int i = 0; i < 4; ++i) {
        std::cout << "Character " << i + 1 << " name: " << g.characters[i].name << std::endl;
        std::cout << "Character " << i + 1 << " HP: " << g.characters[i].hp << std::endl;
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
    // BACKGROUND GENERATION
    //create opengl texture elements
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
    //unsigned char *data2 = new unsigned char[h * w * 4];
    g.tex.backImage = &img[0];
    glGenTextures(1, &g.tex.backTexture);
    int w = g.tex.backImage->width;
    int h = g.tex.backImage->height;
    glBindTexture(GL_TEXTURE_2D, g.tex.backTexture);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, 4, w, h, 0,
            GL_RGBA, GL_UNSIGNED_BYTE, data0);
    //g.tex.xc[0] = 1.0;
    //g.tex.xc[1] = 0.0;
    //g.tex.yc[0] = 1.0;
    //g.tex.yc[1] = 0.0;
    /*-----------------------------------------*/

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

    // city
    g.city.backImage = &img[3];
    glGenTextures(1, &g.city.backTexture);
    w = g.city.backImage->width;
    h = g.city.backImage->height;
    glBindTexture(GL_TEXTURE_2D, g.city.backTexture);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, 3, w, h, 0,
            GL_RGB, GL_UNSIGNED_BYTE, g.city.backImage->data);
    g.city.xc[0] = 0.0;
    g.city.xc[1] = 1.0;
    g.city.yc[0] = 0.0;
    g.city.yc[1] = 1.0;





}

// Function to sort characters based on initiative (descending order)
bool compareInitiative(const Character& a, const Character& b) {
    return a.initiative > b.initiative;
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
                g.characters[0].selectAction(g.characters, g.boss, 0);
                break;
            case XK_2:
                g.characters[1].selectAction(g.characters, g.boss, 1);
                break;
            case XK_3:
                g.characters[2].selectAction(g.characters, g.boss, 2);
                break;
            case XK_4:
                g.characters[3].selectAction(g.characters, g.boss, 3);
                break;
            case XK_5:
                g.boss[0].almighty(g.boss[0], g.characters);
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

//Added Play Game - Nicklas Chiang
void play_game()
{
    srand(time(NULL));
    int done = 0;
    while (!done) {
        while (x11.getXPending()) {
            XEvent e = x11.getXNextEvent();
            x11.check_resize(&e);
            check_mouse(&e);
            done = check_keys(&e);
        }
        // Combat sequence
            // Simulate combat rounds
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

//Function to display hp for characters array
void display_hp()
{

    Rect r;
    unsigned int c = 0xFFFFFF;

    // Set the position for displaying HP in the top right corner
    r.bot = g.yres - 20;
    r.left = g.xres - 150;
    r.center = 0;
    // Display HP and SP for each character in the global array
    for (int i = 0; i < 4; ++i) {
        int current_hp = g.characters[i].hp;
        int current_sp = g.characters[i].sp;
        int max_hp = g.characters[i].max_hp;
        // Calculate the percentage of health remaining
        if (current_hp == 0) {
            // If HP is zero, display "down" instead of percentage
            ggprint8b(&r, 16, c, "%s HP: Downed", g.characters[i].name.c_str());
        } else {
            // Calculate the percentage of health remaining
            int percentage = (int)(((float)current_hp / max_hp) * 100);
            // Display HP using ggprint8b function
            ggprint8b(&r, 16, c, "%s HP: %d/%d (%d%%)", g.characters[i].name.c_str(), current_hp, max_hp, percentage);
        }
        r.bot -= 20;
        // Display SP
        ggprint8b(&r, 16, c, "%s SP: %d/%d", g.characters[i].name.c_str(), current_sp, g.characters[i].max_sp);
        // Move to next line
        r.bot -= 20;
    }

    // Display HP and SP for the enemy character in the global array

    if (g.boss[0].hp == 0) {
        ggprint8b(&r, 16, 0xFF0000, "%s HP: Downed", g.boss[0].name.c_str());
    } else {
        int bossPercentage = (int)(((float)g.boss[0].hp / g.boss[0].max_hp) * 100);
        ggprint8b(&r, 16, 0xFF0000, "%s HP: %d/%d (%d%%)", g.boss[0].name.c_str(), g.boss[0].hp, g.boss[0].max_hp, bossPercentage);
    }
    r.bot -= 20;
    // Display SP
    ggprint8b(&r, 16, 0xFF0000, "%s SP: %d/%d", g.boss[0].name.c_str(), g.boss[0].sp, g.boss[0].max_sp);
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
    // glClear(GL_COLOR_BUFFER_BIT);
    // glColor3f(1.0, 1.0, 1.0);
    // ----- Red Box ----- //
    // draw in counter-clockwise order
    // botleft -> botright -> topright -> topleft
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // main container border
    glColor4f(1.0, 0.0, 0.0, 1.0); // red
    glBegin(GL_QUADS); 
    glVertex2i(0, g.yres/4); // topleft: (x,y)
    glVertex2i(g.xres, g.yres/4); // topright: (x,y)
    glVertex2i(g.xres, 0); // botright: (x,y)
    glVertex2i(0, 0); // botleft: (x,y)
    glEnd();
    // main container inner
    //  255, 87, 51 
    //glColor3ub(255,87,51);
    glColor3ub(255,255,255);
    glBegin(GL_QUADS); 
    glVertex2i(10, (g.yres/4) - 10); // topleft: (x,y)
    glVertex2i(g.xres - 10, (g.yres/4) - 10); // topright: (x,y)
    glVertex2i(g.xres - 10, 10); // botright: (x,y)
    glVertex2i(10,10); // botleft: (x,y)
    glEnd();



    // glXSwapBuffers(x11.dpy, x11.win);
    // usleep(1000);
    // std::cout << "Battle Menu init" << std::endl;
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
        unsigned int color = WHITE;

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
    glColor3f(1.0, 1.0, 1.0);

    glBindTexture(GL_TEXTURE_2D, g.tex.backTexture);
    glBegin(GL_QUADS);
    // 0,0 = top left
    // 1,1 = bottom right
    glTexCoord2f(0.0, 0.0); glVertex2i(0,g.yres);
    glTexCoord2f(1.0, 0.0); glVertex2i(g.xres, g.yres);
    glTexCoord2f(1.0, 1.0); glVertex2i(g.xres, 0);
    glTexCoord2f(0.0, 1.0); glVertex2i(0, 0);
    glEnd();

    /*
    //draw bg
    glBindTexture(GL_TEXTURE_2D, g.tex.backTexture);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0, 0.0); glVertex2i(0,0); // bottom left
    glTexCoord2f(1.0, 0.0); glVertex2i(g.xres, 0); // bottom right
    glTexCoord2f(1.0, 1.0); glVertex2i(g.xres, g.yres); // top right
    glTexCoord2f(0.0, 1.0); glVertex2i(0, g.yres); // top left
    glEnd();
    */
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
    glClearColor(0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT);
    glColor3f(1.0, 1.0, 1.0);

    display_battleMenu();
    display_hp();

    Rect r;

    // Set the position for displaying HP in the top left corner
    r.bot = g.yres - 20;
    r.left = 20;
    r.center = 0;

    ggprint8b(&r, 16, 0xFFFFFF, "press 1 for joker actions");
    r.bot -= 20;
    ggprint8b(&r, 16, 0xFFFFFF, "press 2 for mona actions");
    r.bot -= 20;
    ggprint8b(&r, 16, 0xFFFFFF, "press 3 for panther actions");
    r.bot -= 20;
    ggprint8b(&r, 16, 0xFFFFFF, "press 4 for skull actions");
    r.bot -= 20;
    ggprint8b(&r, 16, 0xFFFFFF, "press 5 for kamoshida to attack all");
    r.bot -= 20;


}













