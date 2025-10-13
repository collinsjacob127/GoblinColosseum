
# Goblin Colosseum

## Download Instructions

Make sure you have git installed

```{sh}
# Clone the repo, including the emsdk module
git clone --recurse-submodules git@github.com:collinsjacob127/GoblinColosseum.git

# Enter the project's directory
cd GoblinColosseum/
```

## Build Instructions

### Building on Linux

Prerequisites:
- cmake
- git
- git-lfs (Make sure you run `git lfs install`)

(Instructions based loosely on those given by [SDL3 docs](https://github.com/libsdl-org/SDL/blob/main/docs/INTRO-cmake.md))

(**Optional**) You *may* need to [install the pre-requisite dev tools](https://github.com/libsdl-org/SDL/blob/main/docs/README-linux.md#build-dependencies) for window management:
```
sudo apt-get install build-essential git make \
pkg-config cmake ninja-build gnome-desktop-testing libasound2-dev libpulse-dev \
libaudio-dev libfribidi-dev libjack-dev libsndio-dev libx11-dev libxext-dev \
libxrandr-dev libxcursor-dev libxfixes-dev libxi-dev libxss-dev libxtst-dev \
libxkbcommon-dev libdrm-dev libgbm-dev libgl1-mesa-dev libgles2-mesa-dev \
libegl1-mesa-dev libdbus-1-dev libibus-1.0-dev libudev-dev \
libpipewire-0.3-dev libwayland-dev libdecor-0-dev liburing-dev
```

Build the game:

```{sh}
# Move into the game subdirectory (GoblinColosseum/game)
cd game

# Build SDL library files
cmake -S . -B build

# Build the game
cmake --build build --parallel

# Run the game
./build/src/GOBLIN
```

### Building on Windows

1. Download [MSYS2](https://www.msys2.org/)
2. Open MSYS2 UCRT64.
3. Install working toolchain
```{sh}
pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-sdl3
```
4. Build (while in GoblinColosseum/game directory)
```{sh}
cmake -S . -B build
cmake --build build --parallel
```
5. Run
```{sh}
./build/src/GOBLIN.exe
```

If you run into issues, here is what I referenced to figure it out: [MinGW SDL3 Instructions](https://github.com/libsdl-org/SDL/blob/main/docs/INTRO-mingw.md)
