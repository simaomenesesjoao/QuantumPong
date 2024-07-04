# QuantumPong
A quantum version of the classic Pong game. Each user controls a paddle and bounces the electron wavefunction around until it diffracts away! The top and bottom edges absorb the wavefunction. A player loses when more than 50% of the original wavefunction has been absorbed on their side. Youtube video showing how it looks like: https://www.youtube.com/shorts/oA7VtZ_YcGI

## Controls
Lobby:
- Press N to set/unset ready. The game will start once both players are ready

Ingame:
- Use the WASD keys to move the paddle around
- Clicking with the mouse anywhere on the screen creates a potential barrier circle in that spot
- Pressing Space pauses the game. To resume, set/unset ready by pressing space again

End screen:
- Press Return to return to the lobby

## Installation
This is a preliminary version of the game and requires OpenCL GPU acceleration to run. SDL2 is used for the client-side graphics.

## Technical details
This game is simulated on a discretized version of the Schrodinger equation in 2D space. The equation is solved by computing the time evolution operator with a Chebyshev decomposition.

## Wish list
Support for more platforms<br>
More physical systems (spin effects, sublattice effects)<br>
Make the window dimensions independent of the simulation dimensions<br>
