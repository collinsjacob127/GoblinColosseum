
# Goblin Colosseum

## Installation Instructions

Built on Ubuntu 24.04, but should work on any platform with emscripten support and a modern browser.

### Clone the repo

```{sh}
# Clone the repo, including the emsdk module
git clone --recurse-submodules git@github.com:collinsjacob127/GoblinColosseum.git

# Enter the project's directory
cd GoblinColosseum/
```

### Set-up Emscripten

Original instructions [here](https://emscripten.org/docs/getting_started/downloads.html).

```{sh}
# Enter the emsdk directory
cd emsdk

# Fetch latest version
git pull

# Install version 4.0.13 (latest at time of writing)
./emsdk install 4.0.13

# Activate the sdk
./emsdk activate 4.0.13

# Set PATH and environment variables
source ./emsdk_env.sh
```

Windows & Mac users, see the linked [emscripten docs](https://emscripten.org/docs/getting_started/downloads.html) for platform-specific instructions.


