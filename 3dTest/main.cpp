#define SDL_MAIN_HANDLED
#include <iostream>
#include <fstream>
#include <SDL.h>
using namespace std;


class Screen {
public:
    SDL_Surface* surf;
    SDL_Window* window;
    unsigned char flag;
    unsigned char screen[64][32];

    Screen(SDL_Surface* surface, SDL_Window* win) {
        surf = surface;
        window = win;
        flag = 0; // if it is one then we overdrew
    }

    void clear(){
        SDL_LockSurface(surf); // ensure we can use surf
        SDL_memset(surf->pixels, 0, surf->h * surf->pitch);
        SDL_UnlockSurface(surf); // unlock
        SDL_UpdateWindowSurface(window);
    }

    void update() {
        SDL_LockSurface(surf); // ensure we can use surf
        unsigned int* pixels = (unsigned int*)surf->pixels;
        for (int i = 0; i < 640; i++) {
            for (int j = 0; j < 320; j++){
                pixels[i + j * 640] = 0xffffffff;
            }
        }
        SDL_UnlockSurface(surf); // unlock
    }

    void blitPixel(int x, int y, int scale, int color) {
        SDL_LockSurface(surf); // ensure we can use surf
        unsigned int* pixels = (unsigned int*)surf->pixels;
        for (int i = 0; i < scale; i++) {
            for (int j = 0; j < scale; j++) {
               pixels[(x + i) + (y + j) * surf->w] ^= color;
            }
        }
        SDL_UnlockSurface(surf); // unlock
    }

    void drawSprite(unsigned short x, unsigned short y, unsigned char h, unsigned char spr[]) {
        SDL_LockSurface(surf); // ensure we can use surf
        unsigned int* pixels = (unsigned int*)surf->pixels;
        //cout << endl;
        for (int i = 0; i < h; i++) { // rows
            for (int j = 0; j <= 7; j++) { // cols
                if (x + j >= 64) {
                    x = 0;
                }
                if (y + i >= 32) {
                    y = 0;
                }
                if ((unsigned int)(spr[i] & 1 << 7 - j) > 0) {
                    if (pixels[((x + j) * 10) + ((y + i) * 10) * surf->w] > 0) {
                        flag = 1; // if there is a  pixel overiten
                    }
                    blitPixel((x + j) * 10, (y + i) * 10,10, 0xFFFFFF);
                }
            }
        }
        SDL_UnlockSurface(surf); // unlock
        SDL_UpdateWindowSurface(window);
    }
};
class CPU {
public:
    unsigned char regs[16] = {0}; // a list of all the regs can be addressed like this: regs[0xF] or reg[0x0] ect... WHILE it is only 8 bits use 16 for overflow reasons
    unsigned char soundTimer; // when not zero plays a beep
    unsigned char delayTimer; // can be set and read
    unsigned char sp; //5 sp is a reference to an item on the 8 bit stack
    unsigned short stack[16]; // stores the last proccsess recursion depth of 1
    unsigned short pc; // just a program counter
    unsigned int i; // a pointer to a memory address
    unsigned char memory[4096] = {0};
    unsigned char selectedSpr[16] = { 0 };
    Screen* screen; // a screen
    CPU (Screen* s){
        pc = 512;
        delayTimer = 0;
        soundTimer = 0;
        screen = s;
        i = 0;
        sp = 0;
        loadFonts();
    }

    void loadFonts() {
        unsigned char font[80] = { 
            0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
            0x20, 0x60, 0x20, 0x20, 0x70, // 1
            0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
            0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
            0x90, 0x90, 0xF0, 0x10, 0x10, // 4
            0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
            0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
            0xF0, 0x10, 0x20, 0x40, 0x40, // 7
            0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
            0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
            0xF0, 0x90, 0xF0, 0x90, 0x90, // A
            0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
            0xF0, 0x80, 0x80, 0x80, 0xF0, // C
            0xE0, 0x90, 0x90, 0x90, 0xE0, // D
            0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
            0xF0, 0x80, 0xF0, 0x80, 0x80, // F
        }; // standard chip 8 font
        for (int i = 0; i < 80; i++) {
            memory[i] = font[i];
        }
    }

    bool isKeyDown() { // is a key down
        SDL_Event e;
        SDL_PumpEvents();
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.scancode == SDL_SCANCODE_1 || e.key.keysym.scancode == SDL_SCANCODE_2 || e.key.keysym.scancode == SDL_SCANCODE_3 || e.key.keysym.scancode == SDL_SCANCODE_4 || e.key.keysym.scancode == SDL_SCANCODE_Q
                    || e.key.keysym.scancode == SDL_SCANCODE_W || e.key.keysym.scancode == SDL_SCANCODE_E || e.key.keysym.scancode == SDL_SCANCODE_R || e.key.keysym.scancode == SDL_SCANCODE_A || e.key.keysym.scancode == SDL_SCANCODE_S
                    || e.key.keysym.scancode == SDL_SCANCODE_D || e.key.keysym.scancode == SDL_SCANCODE_F || e.key.keysym.scancode == SDL_SCANCODE_Z || e.key.keysym.scancode == SDL_SCANCODE_X || e.key.keysym.scancode == SDL_SCANCODE_C
                    || e.key.keysym.scancode == SDL_SCANCODE_V) {
                    return true;
                }
            }
        }
        return false;
    }

    char currentKeyDown() { // what key is down
        SDL_Event e;
        SDL_PumpEvents();
        while (1) {
            if (SDL_PollEvent(&e)) {
                if (e.type == SDL_KEYDOWN) {
                    switch (e.key.keysym.scancode)
                    {
                    case SDL_SCANCODE_1: // 1 or 0 in ch8
                        return 0x0;
                        break;
                    case SDL_SCANCODE_2: // 2 or 1 in ch8
                        return 0x1;
                        break;
                    case SDL_SCANCODE_3: // 3 or 2 in ch8
                        return 0x2;
                        break;
                    case SDL_SCANCODE_4: // 4 or 3 in ch8
                        return 0x3;
                        break;
                    case SDL_SCANCODE_Q: // q or 4 in ch8
                        return 0x4;
                        break;
                    case SDL_SCANCODE_W: // w or 5 in ch8
                        return 0x5;
                        break;
                    case SDL_SCANCODE_E: // e or 6 in ch8
                        return 0x6;
                        break;
                    case SDL_SCANCODE_R: // r or 7 in ch8
                        return 0x7;
                        break;
                    case SDL_SCANCODE_A: // a or 8 in ch8
                        return 0x8;
                        break;
                    case SDL_SCANCODE_S: // s or 9 in ch8
                        return 0x9;
                        break;
                    case SDL_SCANCODE_D: // d or A
                        return 0xA;
                        break;
                    case SDL_SCANCODE_F: // f or B in ch8
                        return 0xB;
                        break;
                    case SDL_SCANCODE_Z: // z or C in ch8
                        return 0xC;
                        break;
                    case SDL_SCANCODE_X: // x or D in ch8
                        return 0xD;
                        break;
                    case SDL_SCANCODE_C: // c or E in ch8
                        return 0xE;
                        break;
                    case SDL_SCANCODE_V: // v or F in ch8
                        return 0xF;
                        break;
                    default:
                        break;
                    }
                }
            }
        }
        
        return -1;
    }

    bool getKeyDown(unsigned char keyId) { // is THIS key down
        SDL_Event e;
        SDL_PumpEvents();
        const Uint8* keystates = SDL_GetKeyboardState(NULL);
        switch ((short)keyId)
        {
        case 0x0:
            return keystates[SDL_SCANCODE_1];
        case 0x1:
            return keystates[SDL_SCANCODE_2];
        case 0x2:
            return keystates[SDL_SCANCODE_3];
        case 0x3:
            return keystates[SDL_SCANCODE_4];
        case 0x4:
            return keystates[SDL_SCANCODE_Q];
        case 0x5:
            return keystates[SDL_SCANCODE_W];
        case 0x6:
            return keystates[SDL_SCANCODE_E];
        case 0x7:
            return keystates[SDL_SCANCODE_R];
        case 0x8:
            return keystates[SDL_SCANCODE_A];
        case 0x9:
            return keystates[SDL_SCANCODE_S];
        case 0xA:
            return keystates[SDL_SCANCODE_D];
        case 0xB:
            return keystates[SDL_SCANCODE_F];
        case 0xC:
            return keystates[SDL_SCANCODE_Z];
        case 0xD:
            return keystates[SDL_SCANCODE_X];
        case 0xE:
            return keystates[SDL_SCANCODE_C];
        case 0xF:
            return keystates[SDL_SCANCODE_V];
        default:
            break;
        }
        return false;
    }

    void getSubArr(unsigned char* arr, unsigned char* res, int start, int end) {
        for (int i = 0; i < end; i++) {
            res[i] = arr[i + start];
        }
    }
    // "C:/Users/kings/Desktop/testRom.ch8"
    void loadProgram(const char path[]) {
        ifstream rom(path, fstream::in | ios::binary);
        int ref = 0;
        char ch;
        if (rom.is_open()) {
            while (rom >> noskipws >> ch) {
                memory[512 + ref] = ch; // write first 8 bits into memory
                //cout << "LOADING value: " << ch << "AT ADDR" << ref / 2 << endl;
                ref += 1;
            }
        }
        rom.close();
        /*
        unsigned short CHIP_8_OPCODES[20] = { 0xC11F, 0xC20F, 0xA20C, 0xD128, 0xD128, 0x1208, 0x7070, 0x2070, 0xA820,0x5050, 0x0, 0x0, 0x70, 0x70, 0x20, 0x70, 0xA8, 0x20, 0x50, 0x50, };

        for (int i = 0; i < 20; i ++) {
            memory[512 + i] = CHIP_8_OPCODES[i];
          
          cout << (int)CHIP_8_OPCODES[i] << " ";
        }
        */
    }


    void eval(unsigned short opcode) {
        unsigned char nibble = opcode >> 12;
        unsigned short params = opcode & 0xFFF;
        //cout << "EXECUTING CODE: "<<  opcode << endl;
        switch (nibble)
        {
        case 0:
            if (params == 224) {
                //cout << "IMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM CLEARING: " << endl;
                screen->clear();
            }
            if (params == 238) {
                pc = stack[sp]; // get are old position
                sp -= 1; // point to the previous position
            }
            break;
        case 1: // jmp
            //cout << "JUMPING Yay";
            pc = params - 2; // se the program counter to our parameters
            for (int i = 0; i < 10; i++) {
                cout << (unsigned short)memory[pc + i] << " ";
            }
            //cout << endl << pc << "YAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAY" << endl;
            break;
        case 2: // call
            sp += 1;
            stack[sp] = pc;
            pc = params - 2;
            break;
        case 3: // (3xkk) skip instruction if Vx == kk
            if ((regs[params >> 8]) == (params & 0xFF)) {
                pc += 2;
            }
            break;
        case 4: // (4xkk) skip instruction if Vx != kk
            if ((regs[params >> 8]) != (params & 0xFF)) {
                pc += 2;
            }
            break;
        case 5: // (5xy0) skips next instruction if reg[x] == reg[y]
            if ( regs[(params >> 8)] == regs[((params >> 4) & 0xF)]) {
                pc += 2;
            }
            break;
        case 6: // (6xkk) load a positive int into the register vx
            regs[(params >> 8)] = (params & 0xFF);
            break;

        case 7: // (7xkk) set vx to vx + kk
            regs[(params >> 8)] += (params & 0xFF);
            break;
        case 8: // 8xyo where o is the operation preformed on the registers the result is always in vx
            // switch on the last param
            switch (params & 0xF)
            {
            case 0: // vx = vy
                regs[params >> 8] = regs[(params >> 4) & 0xF];
                break;
            case 1: // vx | vy
                regs[params >> 8] = regs[params >> 8] | regs[(params >> 4) & 0xF];
                break;
            case 2: // vx & vy
                regs[params >> 8] = regs[params >> 8] & regs[(params >> 4) & 0xF];
                break;
            case 3: // vx xot vy
                regs[params >> 8] = regs[params >> 8] ^ regs[(params >> 4) & 0xF];
                break;
            case 4: // vx + vy
                regs[(params >> 8)] += regs[((params >> 4) & 0xF)];
                if (regs[(params >> 8)] > 255) {
                    regs[0xF] = 1;
                }
                else {
                    regs[0xF] = 0;
                }
                break;
            case 5: // vx - vy
                if (regs[params >> 8] < regs[params >> 4 & 0xF]) {
                    regs[0xF] = 1;
                }
                else {
                    regs[0xF] = 0;
                }
                regs[params >> 8] -= regs[params >> 4 & 0xF];
                break;
            case 6: // vx >> vy bit
                regs[0xF] = params % 2; // get the least significant bit of the number (what we will lose when we shift)
                regs[params >> 8] = regs[params >> 8] >> 1;
                break;
            case 7: // vy - vx 
                if (regs[params >> 8] > regs[params >> 4 & 0xF]) {
                    regs[0xF] = 1;
                }
                else {
                    regs[0xF] = 0;
                }
                regs[params >> 8] = regs[params >> 4 & 0xF] - regs[params >> 8];
                break;
            case 14:
                regs[0xF] = (params & 0x8000); // get the most significant bit of the number (what we will lose when we shift)
                regs[params >> 8] = regs[params >> 8] << 1;
            default:
                break;
            }
            break;
        case 9: // 9xy0 skip next insturction if vx and vy are not equal
            if (regs[params >> 8] !=  regs[params >> 4 & 0xF]) {
                sp += 2;
            }
            break;
        case 0xA: // set i to params (i is a location in the memory used to draw sprites)
            i = params;
            break;
        case 0xB: // Bnnn jump to  v0 + nnn
            pc = regs[0x0] + params;
            break;
        case 0xC: // Cxkk random number anded with kk stored in x;
            srand(time(0));
            regs[params >> 8] = (rand()%256) & (params & 0xFF);
            break;
        case 0xD: // Dxyn DRAW A SPRITE AT (vx, vy) the sprite is loaded from memory at i and n is the height it is always 8 wide
            getSubArr(memory, selectedSpr, i, params & 0xF); // five rows of 8
            screen->drawSprite(regs[params >> 8], regs[(params &0xFF) >> 4], params & 0xF, selectedSpr);
            regs[0xF] = screen->flag; // flag will be 1 if we overwrite a pixel
            screen->flag = 0; // set flag to zero for future
            break;
        case 0xE: //WIP (no keyboard) keyboard
            if ((params & 0xFF) == 0x9E) { // skip instruction if key is down
                if (getKeyDown(regs[(params >> 8)])) {
                    pc += 2;
                }
            }// key x down
            else if ((params & 0xFF) == 0xA1) { // skip if key is not down
                if ( !getKeyDown(regs[(params >> 8)]) ) {
                    pc += 2;
                }
            }
            break;
        case 0xF: // random instructions wich use 1 variable
            switch (params & 0xFF) {
            case 0x07: // vx = the delay timer
                regs[params >> 8] = delayTimer;
                break;
            case 0x0A: // wait until key pressed
                regs[params >> 8] = currentKeyDown(); // instruction waits for valid key press then returns it
                break;
            case 0x15: // set delay timer to vx
                delayTimer = regs[params >> 8];
                break;
            case 0x18: // WIP (no sound yet) set sound timer
                soundTimer = regs[params >> 8];
                break;
            case 0x1E: // i += vx
                i = i + regs[params >> 8];
                break;
            case 0x29: //  select a diget with the value of vx stored in the program memory
                i = regs[(params >> 8)] * 5; // nth digit times digit size (5)
                break;
            case 0x33: // set bcd version of register x to i, i + 1, and i + 2 
                memory[i] = regs[params >> 8]/100;
                memory[i + 1] = (regs[params >> 8] / 10)%10;
                memory[i + 2] = (regs[params >> 8] % 100) % 10;
                break;
            case 0x55: // stores from v0 to vx in memory then sets i to i + x + 1
                for (int r = 0; r <= params >> 8; r++) {
                    memory[i] = regs[r];
                    i++;
                }
                i += 1; // i is already i + x so just add 1
                break;
            case 0x65: // sets values v0 through vx to the values starting at memory address y. then set i to i + x + 1 
                for (int r = 0; r <= params >> 8; r++) {
                    regs[r] = memory[i];
                    i += 1;
                }
                i += 1;
                break;
            }
            break;

        default:
            break;
        }
    }
};

int main(int argc, char* args[])
{
    int r, g, b = 0;
    int frameTime = 0;
    int frameDelay = 100;;
    int frameStart = 0;
    SDL_Renderer* renderer;
    int SCREEN_WIDTH = 640;
    int SCREEN_HEIGHT = 320;


    //The window we'll be rendering to
    SDL_Window* window = NULL;

    //The surface contained by the window
    SDL_Surface* screenSurface = NULL;
    //Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
    }
    else
    {
        //Create window
        window = SDL_CreateWindow("SDL Tutorial", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
        renderer = SDL_CreateRenderer(window, -1, 0);
        SDL_RenderSetLogicalSize(renderer, 64, 32);
        if (window == NULL)
        {
            printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        }
        else
        {
            //Get window surface
            screenSurface = SDL_GetWindowSurface(window);
            Screen screen(screenSurface, window);
            screen.clear();
            CPU myCPU(&screen);
            myCPU.loadProgram("C:/Users/kings/OneDrive/Desktop/chipRoms/tank.ch8");
            for (int i = 0; i < 4096; i++) {
                //cout << hex << (short)myCPU.memory[i] << " ";
            }
            SDL_Event event;
            double now;
            double secs;
            double lastTime = SDL_GetPerformanceCounter();
            while (1) {
                double lastTime = SDL_GetPerformanceCounter();
                /*stepper code
                while (true)
                {
                    SDL_PollEvent(&event);
                    if (event.type == SDL_KEYDOWN) {
                        break;
                    }
                }*/


                unsigned short instruction = (myCPU.memory[myCPU.pc] << 8) + ((myCPU.memory[myCPU.pc + 1]));
                myCPU.eval(instruction);
                myCPU.pc += 2;
                if (myCPU.delayTimer > 0) {
                    myCPU.delayTimer -= 1;
                }
                if (myCPU.soundTimer > 0) {
                    myCPU.soundTimer -= 1;
                }
                float freq = SDL_GetPerformanceFrequency();

                now = SDL_GetPerformanceCounter();
                secs = (now - lastTime) / freq;
                SDL_Delay(abs(1 - secs * 100));
            }
            myCPU.eval(0xF029);
            myCPU.eval(0xD99A);
            myCPU.eval(0x00E0);
            bool quit = false;
            while (!quit) {
                while (SDL_PollEvent) {}
            }

        }
    }

    //Destroy window
    SDL_DestroyWindow(window);

    //Quit SDL subsystems
    SDL_Quit();

    return 0;
}