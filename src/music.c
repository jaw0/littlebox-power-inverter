/*
  Copyright (c) 2013
  Author: Jeff Weisberg <jaw @ tcp4me.com>
  Created: 2013-Sep-30 17:28 (EDT)
  Function: music
*/


const char * const songnames[] = {
    "raygun",
    "menu",
    "ahead",
    "unsafe",
    "up/dn",
    "emergency",

    "chariots",
    "cucaracha",
    "star wars",
    "jeopardy",
    "sw march",
    "birthday",
    "revilie",
    "taps",
    "menuet",
    "nyan cat",
    "b danube",
    "ss banner",
    "cantina",

    0,
};

const char * const songs[] = {
    "[32 c>>7 b>>7] z0",	// ray gun
    "A3A3D-3",			// menu
    "A3D-3D+3",			// go
    "g4e4f4f-3",		// unsafe
    "G-4G-4G-4A",		// upside down
    "[4 D+>D->]",		// emergency

    "T66 c3 g3~a3~b3~ g e e3 c3 g3~a3~b3~ g g g3 c3 g3~a3~b3~ g e e3 e3 f3~e3~d3~ c1 c c>3~b3~a3~ b3 b4 g4 a3 a4 f4 g3 c>3~b3~a3~ b c>3 c>3~b3~a3~ b3 b4 g4 a3 a4 f4 g3 g4 e4 f3~e3~d3~ c1",	// chariots of fire

    // la cucaracha
    "T160 | c c3 f3 f3 a3 a3 S c+ a S c+ c+3 d+3 c+3 b3 a3 S b g S "
    "| c c3 e3 e3 g3 g3 S b g S c+ d+3 c+3 b3 a3 g3 S [2 a f z3 ] f S "
    "| f3>>f3z3 a3 z3 c3 c3 c3 S f3>>f3z3 a3 S f3 f3 f3 e3 e3 d3 d3 S c1 z3 c3 c3 c3 S "
    "| e3>>e3z3 g3 z3 c3 c3 c3 S e3>>e3z3 g3 S c+ d+3 c+3 b3 a3 g3 S [2 f z3>>z3z3 ] S ",

    "T66 c3~c3~c3~g>1d>+1c>+3~b>3~a3~g>+1d>+2c>+3~b>3~a3~g>+1d>+2c>+3~b>3~c+3~a>1d>3~d3~g>1d>+1c>+3~b>3~a3~g>+1d>+2c>+3~b>3~a3~g>+1d>+2c>+3~b>3~c+3~a>1d2~d3~e2>z3e>3c>+3b>3a>3g3g>3~a>3~b>3~a2~e>3~f#2d2~d3~e2>z3e>3c>+3b>3a>3g3d>+3>z4a>4a>1d2~d3~e2>z3e>3c>+3b>3a>3g3g>3~a>3~b>3~a2~e>3~f#2d+2~d+3~g>+2~f+3~e>+2~d+3~c>2~b_3~a>2~g3~d>+1>z2c3~c3~c3~g>1d>+1c>+3~b>3~a3~g>+1d>+2c>+3~b>3~a3~g>+1d>+2c>+3~b>3~c+3~a>1d2~d3~g>1d>+1c>+3~b>3~a3~g>+1d>+2c>+3~b>3~a3~g>+1d>+2c>+3~b>3~c+3~a>1d+2g>+0>>z1z2g+3~g+3~g+3~g>+3",	// star wars

    "T132 c>+f>+c>+f>c>+f>+c>+1c>+f>+c>+f>+a>+a>+3g>+3f>+3e>+3d>+3d_+3c>+f>+c>+f>c>+f>+c>+1f>+f+3d>+3c+b_az>gz>f",	// jeopardy

    "T132 ggge3b_3 ge3b_3 g1",	// star wars imperial march
    "T150 c3c3dcfe1 c3c3dcgf1   c3c3c+afed1  b3b3aefe1",	// happy birthday
    "T180 c-3.c-4f-3c-3f-3a-3f-2f-3.f-4a-3f-3a-3c3a-2f-3.a-4c2a-3.f-4c-2c-3.c-4f-2f-3.f-4f-2",		// revilie
    "T120 c-3.c-4f-1.c-3.f-4a-1.c-3.f-4a-2c-3f-4a-2c-3f-4a-1.f-3.a-4c>1a-2f-2c-1.c-3.c-4f>>-0f-2",	// taps
    "T180 dg>-3a>-3b>-3c>3d>g-g>-ec>3d>3e>3f>3#g>g-g>-cd>3c>3b>-3a>-3b-c>3b>-3a>-3g>-3a-b>-3a>-3g>-3f>-3#g>-1. S", // bach - Menuet

    // nyan cat
    "c+4 d+4 e+4 z4 a+4 d+4 c+4 d+4 | e+4 a+4 c++4 d++4 c++4 g+4 a+4 z4 | e+4 z4 c+4 d+4 e+4 z4 a+3 | b+4 g+4 a+4 b+4 d++4 c++4 d++4 b+4 | e+3 f+3 b#4 c+3 b4 | c+4 b4 a3 a3 b3 | c+3 c+4 b4 a4 b4 c#+4 e+4 | f+4 c+4 e+4 b4 c+4 a4 b4 a4 | c+3 e+3 f+4 c+4 e+4 b4 | c+4 a4 b#4 c+4 c+4 b4 a4 b4 | c+3 a4 b4 c#+4 e+4 b4 c+4 [2 b4 a4 b3 a3 b3 ] b4 a4 b3 a3 a3 | a3 e4 f4 a3 e4 f4 | a4 b4 c+4 a4 d+4 c+4 d+4 e+4 | a3 a3 e4 f4 a4 e4 | d+4 c+4 b4 a4 d4 c4 d4 e4 | a3 e4 f4 a3 e4 f4 | a4 a4 b4 c+4 a4 e4 f4 e4 | a3 a4 g4 a4 e4 f4 a4 [2 d+4 c+4 d+4 e+4 a3 g3 ] d+4 c+4 d+4 e+4 a3 b3",

    //blue danube
    "c3 c3 e3 g3 | g g+3 g+ e+3 | e+ c3 c3 e3 g3 | g g+3 g+ f+3 | f+ b-3 b-3 d3 a3 | a a+3 a+ f+3 | f+ b-3 b-3 d3 a3 | a a+3 a+ e+3 | e+ c3 c3 e3 g3 | c+ c++3 c++ g+3 | g+ c3 c3 e3 g3 | c+ c++3 c++ a+3 | a+ d3 d3 f3 a3 | a3>>a3z3 a3 f#3 g3 | e+3>>e+3z3 e+3 c+3 e3 | e d3 a g3 | c3 c3 c3 c3 | c+3 b3 | b3 a3 a3 z3 a3 g#3 | g#3 a3 a3 z3 d3 d3 | e d3 z3 d3 d3 | a g3 z3 c+3 b3 | b3 a3 a3 z3 a3 b3 | d+3 c+3 c+3 z3 f3 a3 | a g3 f4>>f4z4 e4 c4 a-4 | e4 e4 e3 d3 g3 S",

    // star spangled banner
    "g3 e3 | c e g | c+1 e+3 d+3 | c+ e f# | g1 g3 g3 | e+3>>e+3z3 d+3 c+ | b1 a3 b3 | c+ c+ g | e c g3 e3 | c e g | c+1 e+3 d+3 | c+ e f# | g1 g3 g3 | e+3>>e+3z3 d+3 c+ | b1 a3 b3 | c+ c+ g | e c | e+3 e+3 | e+ f+ g+ | g+1 f+3 e+3 | d+ e+ f+ | f+1 f+ | e+3>>e+3z3 d+3 c+ | b1 a3 b3 | c+ e f# | g1 | g | c+ c+ c+3 b3 | a a a | d+ f+3 e+3 d+3 c+3 | c+ b g3 g3 | c+3>>c+3z3 d+3 e+3 f+3 | g+1 c+3 d+3 | e+3>>e+3z3 f+3 d+ | c+1",


    "T132 a2d+2a2d+2a3d+2a3z3g#3a2a3g#3a3g3z3f#3g3g_3f2>z3d1>z3a2d+2a2d+2a3d+2a3z3g#3a2g3z3g2>z3f#3g2c+3b_2a2g2>z3a2d+2a2d+2a3d+2a3z3g#3a2c+3z3c+2>z3a3g2f2>z3d1>z3d1f1a1c+1e_+2d+2g#3a2f3",	// cantina band from star wars


    0,
};

