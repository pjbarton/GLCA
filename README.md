# GLCA
OpenGL Coded Aperture

Projects a color-indexed set of polygons (i.e. detectors), and calculates the image histograms (i.e. system response) for a number of model rotations (i.e. radiation source angles).  Produces a UInt16 file of size source angles-by-detectors.

Developed on:
Dell M3800 Laptop
Nvidia Quadro K1100M
Ubuntu 14.04

## Prereqs 
    sudo apt-get install libglew-dev freeglut3-dev libglm-dev libconfig-dev

## Compile 
    g++ main.cpp model_ply.cpp LoadShaders.cpp chealpix.c -o glca -lGL -lglut -lGLEW -lconfig

## Config 
Edit parameters in `config.txt`

## Profile
    time ./glca config.txt
2000 FPS with `winSize = 200` and `showBuffers = false`
    
