# QuantumPong
A quantum version of the classic Pong game. Each user controls a paddle and bounces the electron wavefunction around until it diffracts away! The top and bottom edges absorb the wavefunction. A player loses when more than 50% of the original wavefunction has been absorbed on their side. Youtube video showing how it looks like: https://www.youtube.com/shorts/oA7VtZ_YcGI

## Controls
The top player controls the paddle using the ASDW keys
The bottom player controls the paddle using the JKLI keys
The colormap can be changed by pressing the number 1 to make it brighter and 2 to makeit darker.
Pressing R resets the game.
Clicking with the mouse anywhere on the screen creates a potential barrier in that spot (invisible)

## Installation
This is a preliminary version of the game and requires OpenCL GPU acceleration to run. For now, it can only run on Linux with X11 (no Wayland support yet).

## Technical details
This game is simulated on a discretized version of the Schrodinger equation in 2D space. The equation is solved by computing the time evolution operator with a Chebyshev decomposition.

## Wish list
Support for more platforms
OpenCL/OpenGL integration
Make it event driven
Make the window dimensions independent of the simulation dimensions
Better visualization of the objects on the screen
Implement magnetic fields, electric fields, optical fields


