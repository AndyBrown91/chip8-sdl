//
//  main.cpp
//  Chip8
//
//  Created by Andy on 20/07/2015.
//  Copyright (c) 2015 Andy. All rights reserved.
//

#include "Chip.h"

using namespace std;

int main(int argc, char* argv[])
{   
    if (argc != 2)
    {
        cerr << "Need 2 args (Chip8 ROMFILE) recieved " << argc << endl;
        exit(1);
    }
        
    Chip chip;
    chip.loadFile(argv[1]);
    chip.emulate();
    
    return 0;
}

