
# Goblin Colosseum

## Installation Instructions

Built on Ubuntu 24.04, but should work on any platform with emscripten support and a modern browser.

### Clone

Make sure you have 

```{sh}
# Clone the repo, including the emsdk module
git clone --recurse-submodules git@github.com:collinsjacob127/GoblinColosseum.git

# Enter the project's directory
cd GoblinColosseum/
```

### Build

(Instructions based loosely on those given by [SDL3 docs](https://github.com/libsdl-org/SDL/blob/main/docs/INTRO-cmake.md))

```{sh}
# Move into the game subdirectory (GoblinColosseum/game)
cd game

# Build SDL library files
cmake -S . -B build

# Build the game (-j4 builds in parallel)
cmake --build build -j4

# Run the game
./build/src/GOBLIN
```
