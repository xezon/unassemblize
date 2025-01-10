# Building Unassemblize

## Quick Start

### Windows
- Install Visual Studio 2022 with C++ desktop development workload
- Open the project folder in VS 2022
- Select your build configuration (Debug/Release)
- Build Solution (F7)

### macOS
```sh
# Install dependencies
brew install cmake ccache clang-format doxygen graphviz glfw

# Clone and build
git clone https://github.com/OmniBlade/unassemblize.git
cd unassemblize
mkdir build && cd build
cmake ..
make -j$(sysctl -n hw.ncpu)
```

### Ubuntu 22.04 or higher
```sh
# Install dependencies
sudo apt-get update && sudo apt-get install -y \
  ccache \
  clang \
  clang-format \
  cmake \
  doxygen \
  git \
  graphviz \
  libglfw3-dev \
  libgl1-mesa-dev \
  xorg-dev

# Clone and build
git clone https://github.com/OmniBlade/unassemblize.git
cd unassemblize
mkdir build && cd build
cmake ..
make -j$(nproc)
```

## Docker (CLI Only)
For command-line usage only, you can use our Docker container:
```sh
docker build -t unassemblize .
docker run -v $(pwd):/work unassemblize [OPTIONS] [INPUT]
```

## Usage

For a graphical interface, launch with:
```sh
./unassemblize --gui
```

To see all available options and commands, run:
```sh
./unassemblize --help
```
