
# Goblin Colosseum

## Download Instructions

Make sure you have git installed

```{sh}
# Clone the repo, including the emsdk module
git clone --recurse-submodules git@github.com:collinsjacob127/GoblinColosseum.git

# Enter the project's directory
cd GoblinColosseum/
```

## Build

### Linux

Prerequisites:
- cmake
- git

(Instructions based loosely on those given by [SDL3 docs](https://github.com/libsdl-org/SDL/blob/main/docs/INTRO-cmake.md))

```{sh}
# Move into the game subdirectory (GoblinColosseum/game)
cd game

# Download external packages for SDLs text library
./libs/SDL_ttf/external/download.sh

# Build SDL library files
cmake -S . -B build

# Build the game (-j4 builds in parallel)
cmake --build build -j4

# Run the game
./build/src/GOBLIN
```

### Windows

Here's how to setup for windows: [MinGW SDL3 Instructions](https://github.com/libsdl-org/SDL/blob/main/docs/INTRO-mingw.md)

Essentially, you'll need to:
- Download MinGW-w64
- update it
- install the tools
- build SDL
