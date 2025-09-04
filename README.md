
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

Compilation on windows is a little tricky, here are the [SDL3 Docs for getting set up on windows](https://wiki.libsdl.org/SDL3/README-windows).

Essentially, you'll need to:
- Download MinGW-w64
- update it
- install the x86_64 toolchain
- update again
- build SDL
- install SDL
- build project

I plan to add a script that will make Windows compilation easier.
