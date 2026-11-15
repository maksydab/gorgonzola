# Gorgonzola
This is a cellular automaton, often called a `Falling sand cellular automaton`, based on predefined set of rules it performs various simulations. Normally you run those types of things on CPU, but in this project the entire thing from the UI to the simulation is in a compute shader. So why is there main.c? you may ask, it's there to init the shader, as shader on it's own cannot create the window
## How to run it?
To run it you just launch the binary provided in releases, `app`. It must be in same folder as `sim.comp` file, otherwise it won't turn on, your system also must support openGL for it to work.
## Features
This project is fairly interesting as it contains many types of materials that interact with each other:
- Sand - classic of the said `Falling sand cellular automaton` genre
- Water - a liquid that can spread in all directions that aren't above it's current level, it can be boiled
- Steam - a gas that originates from water being boiled
- Wood - a solid that can be easily ignited
- Steel - a solid that can only be damaged by acid
- Acid - dissolves steel, wood, sand but not glass
- Glass - comes from melting sand
- Plant - spreads in water
- Ice - comes from taking water below certain temperature as you may already know.
- Fire - the main source of heat at your disposal
Additional features:
- Switching the display buffers - just press keyboard key `s`. It cycles.
- Temperature simulation - comes with heat transfer
- Ereaser - the first button, it allows to get rid of any material

## Why is it interesting
In normal CPU based cellular automaton, each cell is simulated one by one deciding where it wants to end up, but on a GPU, each cell decides about itself at the same time, essentially meaning if you have a sand and it has an empty cell below it it doesn't clear itself then set the cell below it to sand. The sand cell sets itself to empty, and then when the time comes for the cell below it, the cell below it sees that it's empty adn above is a sand cell so it sets itself to sand cell, this means the simulation needs to be deterministic.