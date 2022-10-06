//
//  Chip.h
//  Chip8
//
//  Created by Andy on 02/08/2015.
//  Copyright (c) 2015 Andy. All rights reserved.
//

#ifndef __Chip8__Chip__
#define __Chip8__Chip__

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <memory>
#include <random>
#include <vector>

#include <assert.h>

#include <SDL2/SDL.h>

#include <SDL2pp/SDL.hh>
#include <SDL2pp/Window.hh>
#include <SDL2pp/Renderer.hh>

#include "chip8.h"

class Chip
{
public:
    Chip();
    ~Chip();
    
    void initSDL ();
    void loadFile (char* location);
    
    void emulate ();

private:
    chip8 other;
    
    unsigned short getHex (unsigned short opcode, uint8_t position, uint8_t length);
    void hex0 (unsigned short opcode);
    
    void goToAddress (unsigned short opcode);
    void skipNextInstruction (unsigned short opcode);
    
    void hex6 (unsigned short opcode);
    
    void hex7 (unsigned short opcode);
    
    void hex8 (unsigned short opcode);
    
    void hexA (unsigned short opcode);
    void hexB (unsigned short opcode);
    
    void hexC (unsigned short opcode);
    
    void hexD (unsigned short opcode);
    void hexE (unsigned short opcode);
    void hexF (unsigned short opcode);
    size_t disassembleChip8 (unsigned char* buffer);
    
    unsigned char random();
    long lookupScancode (SDL_Scancode code);
    
    void renderDisplay ();
    bool setPixel (int x, int y);
    void emulateCycle ();
    
    ////////////////////////
    //      Variables     //
    ////////////////////////
    bool fileLoaded;
    
    //4k
    unsigned char memory [0xfff];
    //0x0 - 0x200 is used for the intepreter, so actual memory starts at 0x200
    const int memoryStart;
    const size_t memorySize;
    
    //General registers - VF is a special flag
    unsigned char registers [16];
    //Special register used to store memory addresses
    unsigned short I;
    
    //Program counter - currently executing address
    unsigned short pc;
    bool jumpFlag;
    //Stack pointer - Topmost level of the stack
    unsigned char sp;
    //Stack - used to store return address when leaving subroutines
    unsigned short stack [16];
    
    //Sound and delay - decrement at a rate of 60hz
    //Sound timer - sounds as long as the value is greater than 0 - single tone, frequency = whatever
    unsigned char st;
    //Delay timer - just decrements
    unsigned char dt;
    
    //Display
    int displayWidth;
    int displayHeight;
    int pixelSize;
    unsigned char display [64*32];
    bool drawFlag;
    
    int fps;
    int frameRate;
    
    //Random numbers
    std::default_random_engine generator;
    std::uniform_int_distribution<unsigned char> distribution;
    
    //Keyboard events
    std::vector<SDL_Scancode> keyLookup;
    
    //SDL
    std::unique_ptr<SDL2pp::SDL> sdl;
    std::unique_ptr<SDL2pp::Window> window;
    std::unique_ptr<SDL2pp::Renderer> renderer;
};

#endif /* defined(__Chip8__Chip__) */
