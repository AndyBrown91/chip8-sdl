//
//  Chip.cpp
//  Chip8
//
//  Created by Andy on 02/08/2015.
//  Copyright (c) 2015 Andy. All rights reserved.
//

#include "Chip.h"

using namespace std;
using namespace SDL2pp;

#define disp(x, y) display[y * displayWidth + x]

Chip::Chip () : memoryStart(0x200), memorySize(0xfff), distribution(0, 255), displayHeight(32), displayWidth(32)
{
    memset(memory, 0, memorySize);
    memset(registers, 0, sizeof(registers));
    memset(stack, 0, sizeof(stack));
    memset(display, 0, sizeof(display));
    
    displayWidth = 64;
    displayHeight = 32;
    pixelSize = 10;
    drawFlag = true;
    
    I = 0;
    pc = memoryStart;
    sp = 0;
    
    st = 0;
    dt = 0;

    //Framerate in Hz
    frameRate =  60;
    
    //Read sprite data into interpreter memory (0x0 - 0x200)
    //Sprites start at position 0 and are 5 bytes each
    unsigned char hexChars [] = {
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
                    0xF0, 0x80, 0xF0, 0x80, 0x80 // F
    };
    
    //Lookup for converting between Chip8 keyboard and SDL
    keyLookup = {
            SDL_SCANCODE_1, SDL_SCANCODE_2, SDL_SCANCODE_3, SDL_SCANCODE_4,
            SDL_SCANCODE_Q, SDL_SCANCODE_W, SDL_SCANCODE_E, SDL_SCANCODE_R,
            SDL_SCANCODE_A, SDL_SCANCODE_S, SDL_SCANCODE_D, SDL_SCANCODE_F,
            SDL_SCANCODE_Z, SDL_SCANCODE_X, SDL_SCANCODE_C, SDL_SCANCODE_V
    };

    for (int i = 0; i < sizeof(hexChars); i++)
    {
        memory[i] = hexChars[i];
    }
    
    initSDL();

    fileLoaded = false;
}

Chip::~Chip()
{
    
}

void Chip::initSDL()
{
    sdl = make_unique<SDL>(SDL_INIT_VIDEO);
    window = make_unique<Window>("Chip 8", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, displayWidth * pixelSize, displayHeight * pixelSize, SDL_WINDOW_RESIZABLE);
    renderer = make_unique<Renderer>(*window.get(), -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);
    
    renderer->SetDrawBlendMode(SDL_BLENDMODE_BLEND);
    
    // Clear screen
    renderer->SetDrawColor(0, 0, 0);
    renderer->Clear();
}

void Chip::loadFile(char *location)
{
    ifstream file;
    file.open(location, ios::binary | ios::ate);
    
    if (file.is_open())
    {
        size_t fileSize = file.tellg();
        
        file.seekg(ios::beg);
        
        file.read((char*) &memory[memoryStart], fileSize);
        
        fileLoaded = true;
        
        other.loadApplication(location);
//        size_t position = memoryStart;
//        
//        while (position < fileSize)
//        {
//            position += disassembleChip8(&memory[position]);
//        }
    }
    else
    {
        cerr << "Error opening file" << endl;
        exit(1);
    }
}

void Chip::emulate()
{
    if (fileLoaded)
    {
        Uint32 secondCounter = 0;
        
        while (1)
        {
            //Get time
            Uint32 blockStartTime = SDL_GetTicks();
            
            if (secondCounter == 0)
                secondCounter = blockStartTime;
            else if (secondCounter + 1000 < blockStartTime)
            {
                //printf("FPS: %d\n", fps);
                fps = 0;
                secondCounter = blockStartTime;
            }
            
            // Process input
            SDL_Event event;
            //poll event also calls pumpevent which refreshes keyboardstate
            while (SDL_PollEvent(&event))
                if (event.type == SDL_QUIT ||
                    (event.type == SDL_KEYDOWN && (event.key.keysym.sym == SDLK_ESCAPE)))
                    return;
            
            
            for (int i = 0; i < 9; i++)
            {
                //emulate cycle
                emulateCycle();
            }
            
            renderDisplay();
            fps++;
            
            //Update timers
            if (dt > 0)
                dt--;
            
            if (st > 0)
                st--;
            
            //Get time
            //minus time to see how long this block took
            int timeElapsed = SDL_GetTicks() - blockStartTime;
            
            //Keeps a steady rendering framerate
            int delay = 1000/frameRate;
            
            if (delay - timeElapsed > 0)
            {
                // Frame limiter
                SDL_Delay((1000/frameRate) - timeElapsed);
            }
        }
    }
    else
    {
        cerr << "A game file must be loaded before emulation" << endl;
        exit(1);
    }
}

void Chip::renderDisplay()
{
    if (drawFlag)
    {
        // Clear screen
        renderer->SetDrawColor(0, 0, 0);
        renderer->Clear();
        
        renderer->SetDrawColor(255, 255, 255);
        for (int y = 0; y < displayHeight; y++)
        {
            for (int x = 0; x < displayWidth; x++)
            {
                if (display[(y * displayWidth) + x] == 1)
                {
                    int startX = x * pixelSize;
                    int startY = y * pixelSize;
                    renderer->FillRect(startX, startY, startX + pixelSize, startY + pixelSize);
                }
            }
        }
    
        renderer->Present();
    }
}

bool Chip::setPixel(int x, int y)
{
    //Wrap around
    if (x > displayWidth)
        x -= displayWidth;
    
    if (y > displayHeight)
        y -= displayHeight;
    
    //XOR-ed on, if this causes any pixels to be erased return 1
    disp(x, y) ^= 1;
    
    return !disp(x, y);
}

unsigned short Chip::getHex (unsigned short opcode, uint8_t position, uint8_t length)
{
    //Size of the opcode not including position 0
    const uint8_t opcodeSize = 4;
    
    assert(length > 0);
    //Position starts at 0, must be between 0-3
    assert(position <= opcodeSize);
    //Position + length must be less than size of the opcode
    assert((position + length) <= opcodeSize);
    
    //Returns the value at (position) in opcode, value will be (length) nibbles long
    int mask = 0x0;
    int addedCount = 0;
    
    for (int i = 0; i < opcodeSize; i++)
    {
        if (i >= position && addedCount < length)
        {
            mask |= (0xf << ((opcodeSize - 1) - i) * 4);
            addedCount++;
        }
        else
            mask |= (0x0 << ((opcodeSize - 1) - i) * 4);
    }
    
    return (opcode & mask) >> ((opcodeSize - (position + length)) * 4);
}

unsigned char Chip::random()
{
    return distribution(generator);
}

long Chip::lookupScancode(SDL_Scancode code)
{
    return find(keyLookup.begin(), keyLookup.end(), code) - keyLookup.begin();
}

void Chip::hex0 (unsigned short opcode)
{
    switch (opcode)
    {
        case 0x00E0:
            printf("Clear the display\n");
            memset(display, 0, sizeof(display));
            drawFlag = true;
            break;
        case 0x00EE:
            //printf("Return from subroutine\n");
            pc = stack[--sp];
            break;
        default:
            //0x0NNN
            //printf("Execute machine subroutine %x\n", getHex(opcode, 1, 3));
            break;
    }
}

void Chip::goToAddress (unsigned short opcode)
{
    //first nibble = 1 or 2
    //1 = jump, 2 = call
    if (getHex(opcode, 0, 1) == 0x2)
    {
        //printf("Execute subroutine at %3x\n", getHex(opcode, 1, 3));
        //CALL
        //Save program counter to stack
        stack[sp] = pc;
        sp++;
    }
    else
    {
        //printf("Jump to %3x\n", getHex(opcode, 1, 3));
    }
    //Goto
    unsigned short address = getHex(opcode, 1, 3);

    if (address < memoryStart)
    {
        printf("Accessing interpreter memory space\n");
    }

    pc = address;
    jumpFlag = true;
}

void Chip::skipNextInstruction (unsigned short opcode)
{
    //3 = skip if equal to literal, 4 = skip if not equal to literal, 5 = skip if equal to register, 9 = skip if not equal to register
    unsigned char x = getHex(opcode, 1, 1);
    
    switch (getHex(opcode, 0, 1))
    {
        case 0x3:
            //printf("Skip if %x is equal %x\n", getHex(opcode, 1, 1), getHex(opcode, 2, 2));
            if (registers[x] == getHex(opcode, 2, 2))
                pc += 2;
            break;
        case 0x4:
            //printf("Skip if %x is NOT equal to %x\n", getHex(opcode, 1, 1), getHex(opcode, 2, 2));
            if (registers[x] != getHex(opcode, 2, 2))
                pc += 2;
            break;
        case 0x5:
            //printf("Skip if %x is equal %x\n", getHex(opcode, 1, 1), getHex(opcode, 2, 1));
            if (registers[x] == registers[getHex(opcode, 2, 1)])
                pc += 2;
            break;
        case 0x9:
            //printf("Skip if %x is NOT equal %x\n", getHex(opcode, 1, 1), getHex(opcode, 2, 1));
            if (registers[x] != registers[getHex(opcode, 2, 1)])
                pc += 2;
            break;
        default:
            break;
    }
}

//Need to remove the 00 from Vx
void Chip::hex6 (unsigned short opcode)
{
    //printf("Store %d in V%x\n", getHex(opcode, 2, 2), getHex(opcode, 1, 1));
    registers[getHex(opcode, 1, 1)] = getHex(opcode, 2, 2);
}

void Chip::hex7 (unsigned short opcode)
{
    //printf("Add %d to  V%x\n", getHex(opcode, 2, 2), getHex(opcode, 1, 1));
    registers[getHex(opcode, 1, 1)] += getHex(opcode, 2, 2);
}

void Chip::hex8 (unsigned short opcode)
{
    unsigned short x = getHex(opcode, 1, 1);
    unsigned short y = getHex(opcode, 2, 1);
    
    switch (getHex(opcode, 3, 1))
    {
        case 0x0:
            //printf("Store V%x in V%x\n", getHex(opcode, 2, 1), getHex(opcode, 1, 1));
            registers[x] = registers[y];
            break;
        case 0x1:
            //printf("Set %x to %x OR %x\n", getHex(opcode, 1, 1), getHex(opcode, 1, 1), getHex(opcode, 2, 1));
            registers[x] |= registers[y];
            break;
        case 0x2:
            //printf("Set %x to %x AND %x\n", getHex(opcode, 1, 1), getHex(opcode, 1, 1), getHex(opcode, 2, 1));
            registers[x] &= registers[y];
            break;
        case 0x3:
            //printf("Set %x to %x XOR %x\n", getHex(opcode, 1, 1), getHex(opcode, 1, 1), getHex(opcode, 2, 1));
            registers[x] ^= registers[y];
            break;
        case 0x4:
        {
            //printf("Add %x to %x\n", getHex(opcode, 1, 1), getHex(opcode, 2, 1));
            unsigned short result = registers[x] + registers[y];
            registers[0xf] = result > 255;
            registers[x] = result;
            break;
        }
        case 0x5:
            //printf("Subtract %x from %x\n", getHex(opcode, 2, 1), getHex(opcode, 1, 1));
            registers[0xf] = registers[x] > registers[y];
            registers[x] -= registers[y];
            break;
        case 0x6:
            //printf("Shift %x right 1 position and save in %x\n", getHex(opcode, 2, 1), getHex(opcode, 1, 1));
            registers[0xf] = registers[x] & 0x1;
            registers[x] >>= 1;
            break;
        case 0x7:
            //printf("Set %x to %x minus %x\n", getHex(opcode, 1, 1), getHex(opcode, 2, 1), getHex(opcode, 1, 1));
            registers[0xf] = registers[y] > registers[x];
            registers[x] = registers[y] - registers[x];
            break;
        case 0xE:
            //printf("Shift %x left 1 position and save in %x\n", getHex(opcode, 2, 1), getHex(opcode, 1, 1));
            registers[0xf] = (registers[x] & 0x80) > 0;
            registers[x] <<= 1;
            break;
            
        default:
            printf("Unhandled %x\n", opcode);
            exit(1);
            break;
    }
}

void Chip::hexA (unsigned short opcode)
{
    //printf("Store %x in I\n", getHex(opcode, 1, 3));
    I = getHex(opcode, 1, 3);
}

void Chip::hexB (unsigned short opcode)
{
    //printf("Jump to %x + v0\n", getHex(opcode, 1, 3));
    pc = getHex(opcode, 1, 3) + registers[0];
    jumpFlag = true;
}

void Chip::hexC (unsigned short opcode)
{
    //printf("Set %x to a random number with mask %x\n", getHex(opcode, 1, 1), getHex(opcode, 2, 2));
    registers[getHex(opcode, 1, 1)] = random() & getHex(opcode, 2, 2);
}

void Chip::hexD (unsigned short opcode)
{
    //printf("Draw sprite at x=%x y=%x with %x\n", getHex(opcode, 1, 1), getHex(opcode, 2, 1), getHex(opcode, 3, 1));
    unsigned short x = registers[getHex(opcode, 1, 1)];
    unsigned short y = registers[getHex(opcode, 2, 1)];
    unsigned short height = getHex(opcode, 3, 1);
    unsigned short pixel;
    //printf("x %d y %d\n", x, y);
    
    //Set overflow register to 0
    registers[0xF] = 0;
    
    for (int yline = 0; yline < height; yline++)
    {
        pixel = memory[I + yline];
        
        for (int xline = 0; xline < 8; xline++)
        {
            if ((pixel & (0x80 >> xline)) != 0)
            {
                if (display[(x + xline + ((y + yline) * 64))] == 1)
                    registers[0xF] = 1;
                
                display[x + xline + ((y + yline) * 64)] ^= 1;
            }
        }
    }
    
    drawFlag = true;
    
}

void Chip::hexE (unsigned short opcode)
{
    //Get whole keyboard state
    int numKeys = 0;
    const uint8_t* keyboardState = SDL_GetKeyboardState(&numKeys);
    
    unsigned short x = getHex(opcode, 1, 1);
    //Check keyboard state for key pressed, register[x] contains key to check, look that value up against key lookup table to get SDL_Scancode
    bool keyState = keyboardState[keyLookup[registers[x]]];

    switch (getHex(opcode, 2, 2))
    {
        case 0x9E:
            //printf("Skip if key stored in %x is pressed\n", getHex(opcode, 1, 1));
            if (keyState)
                pc += 2;
            break;
        
        case 0xA1:
            //printf("Skip if key stored in %x is NOT pressed \n", getHex(opcode, 1, 1));
            if (!keyState)
                pc += 2;
            break;
        default:
            printf("Unhandled %x\n", opcode);
            exit(1);
            break;
    }
}

void Chip::hexF (unsigned short opcode)
{
    //Everything acts on the bit in position 1 (0x0f00)
    unsigned short x = getHex(opcode, 1, 1);
    
    switch (getHex(opcode, 2, 2))
    {
        case 0x07:
            //printf("Store value of delay timer in %x\n", x);
            registers[x] = dt;
            break;
        case 0x0A:
        {
            printf("Wait for a keypress and save in %x\n", x);
            //Cycles are emulated in batches, this stops processing until a key is pressed, therefore not rendering the
            //current display
            renderDisplay();
            
            SDL_Event event;
            
            while (SDL_WaitEvent(&event))
            {
                if (event.type == SDL_KEYDOWN)
                {
                    unsigned char chipKey = lookupScancode(event.key.keysym.scancode);
                    
                    if (chipKey < keyLookup.size())
                    {
                        registers[x] = chipKey;
                        break;
                    }
                }
            }
            
            break;
        }
        case 0x15:
            //printf("Set delay timer to value in %x\n", x);
            dt = registers[x];
            break;
        case 0x18:
            //printf("Set sound timer to value in %x\n", x);
            st = registers[x];
            break;
        case 0x1E:
            //printf("Add value in %x to register I\n", x);
            //Set VF for overflow
            registers[0xF] = (I + registers[x]) > 0xFFF;
            I += registers[x];
            break;
        case 0x29:
            //printf("Set I to address of sprite data in %x\n", x);
            I = (registers[x] * 5);
            break;
        case 0x33:
        {
            //printf("Store binary coded decimal of %x in I, I+1, I+2\n", x);
            int v = registers[x];
            
            for (int i = 2; i >= 0; i--)
            {
                memory[I + i] = v % 10;
                v /= 10;
            }
            
            break;
        }
        case 0x55:
            //printf("Store all registers v0 to v%x to memory starting at I, I becomes I + %x + 1\n", x, x);
            for (int i = 0; i <= x; i++)
            {
                memory[I + i] = registers[i];
            }
            break;
        case 0x65:
            //printf("Fill registers v0 to v%x from memory starting at I, I becomes I + %x + 1\n", x, x);
            for (int i = 0; i <= x; i++)
            {
                registers[i] = memory[I + i];
            }
            
            break;
        default:
            printf("Unhandled %x\n", opcode);
            exit(1);
            break;
    }
    
}



size_t Chip::disassembleChip8 (unsigned char* buffer)
{
    size_t opSize = 2;
    
    //Opcodes are 16-bit - put together buffer[0] + buffer[1]
    unsigned short opcode = (*buffer << 8) | buffer[1];
    printf("%04x    -    ", opcode);
    
    //Most opcodes are based on the first character - mask it
    
    switch (getHex(opcode, 0, 1))
    {
        case 0x0:
            hex0(opcode);
            break;
        case 0x1:
        case 0x2:
            goToAddress(opcode);
            break;
        case 0x3:
        case 0x4:
        case 0x5:
        case 0x9:
            skipNextInstruction(opcode);
            break;
        case 0x6:
            hex6(opcode);
            break;
        case 0x7:
            hex7(opcode);
            break;
        case 0x8:
            hex8(opcode);
            break;
        case 0xA:
            hexA(opcode);
            break;
        case 0xB:
            hexB(opcode);
            break;
        case 0xC:
            hexC(opcode);
            break;
        case 0xD:
            hexD(opcode);
            break;
        case 0xE:
            hexE(opcode);
            break;
        case 0xF:
            hexF(opcode);
            break;
            
        default:
            printf("Not implemented: %x", opcode);
            exit(1);
            break;
    }
    
    
    
    //printf("%x", opcode);
    return opSize;
}

void Chip::emulateCycle()
{
    unsigned short opcode = (memory[pc] << 8) | memory[pc+1];
    //printf("%x %d\n", opcode, pc);
    
    jumpFlag = false;
    
    //Most opcodes are based on the first character - mask it
    switch (getHex(opcode, 0, 1))
    {
        case 0x0:
            hex0(opcode);
            break;
        case 0x1:
        case 0x2:
            goToAddress(opcode);
            break;
        case 0x3:
        case 0x4:
        case 0x5:
        case 0x9:
            skipNextInstruction(opcode);
            break;
        case 0x6:
            hex6(opcode);
            break;
        case 0x7:
            hex7(opcode);
            break;
        case 0x8:
            hex8(opcode);
            break;
        case 0xA:
            hexA(opcode);
            break;
        case 0xB:
            hexB(opcode);
            break;
        case 0xC:
            hexC(opcode);
            break;
        case 0xD:
            hexD(opcode);
            break;
        case 0xE:
            hexE(opcode);
            break;
        case 0xF:
            hexF(opcode);
            break;
            
        default:
            printf("Not implemented: %x", opcode);
            exit(1);
            break;
    }
    
    if ( !jumpFlag)
        pc += 2;
    
}