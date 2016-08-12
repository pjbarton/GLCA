# GLCA - OpenGL Coded Aperture

Projects a color-indexed set of polygons (i.e. detectors), and calculates the image histograms (i.e. system response) for a number of model rotations (i.e. radiation source angles).  Produces UInt16 system response file with dimensions [source angles, detectors].

Developed on:
* Dell M3800 Laptop
* Nvidia Quadro K1100M
* Ubuntu 14.04

## Prereqs 
    sudo apt-get install libglew-dev freeglut3-dev libglm-dev libconfig-dev

## Compile 
    g++ main.cpp model_ply.cpp LoadShaders.cpp chealpix.c -o glca -lGL -lglut -lGLEW -lconfig

## Config 
Edit parameters in `config.txt`:

    nside: [2<sup>N</sup>] number of pixels wide on each of 12 patches (npix = 12 * nside<sup>2</sup>)
    mask: [48 char hex] 192-bit mask with leftmost bit at north pole for HealPix ring indexing
    winSize: [pixels] width / height of window (impacts amplitude of system response)
    showBuffers: [t/f] render buffer to window during computations
    bHealpix: [t/f] cycle through HealPix angles and output histogram
    bAnimate: [t/f] slowly spin model (if bHealpix = false)
    bDOI: (not completely implemented)
    bFPS: [t/f] reports 

## Profile
    time ./glca config.txt
2000 FPS with `winSize = 200` and `showBuffers = false`
    
