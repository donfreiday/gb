# A Gameboy Emulator #

![screenshot](https://github.com/donfreiday/gb/blob/master/screencap.gif)

A cross-platform Gameboy emulator written in C++ using SDL2. Can be compiled to Javascript and run in a browser via Emscripten.

**What works**: 
Enough to play Tetris, Super Mario Land, and probably other games too.
Controller support.

**What doesn't (yet):**: 
window layer of graphics, sound, other stuff

## Usage ##

**Native:**
gb rom.gb

**Javascript:**
emrun emscripten/gb.html

## Dependencies ##

SDL2, make, clang.
Emscripten and LLVM for Javascript build target.

## Building ##

### Arch ###
Native:

```shell
git clone https://github.com/donfreiday/gb.git && cd gb && make
```

Javascript:

```shell
git clone https://github.com/donfreiday/gb.git && cd gb && make js
```
- - - -

### Ubuntu ###

Native:

```shell
sudo apt update && sudo apt upgrade
sudo apt install git make clang++ libsdl2-dev
git clone https://github.com/donfreiday/gb.git && cd gb && make
```

Javascript:

The version of emscripten in Ubuntu's repository is from 2014 (stability ftw).
You'll have to find a PPA or compile Emscripten (and LLVM probably).

- - - -

### Windows: TODO: finish this###

Install Microsoft Visual Studio.
Install [git](https://git-scm.com/downloads)
Download and extract the [SDL2 development libraries](https://www.libsdl.org/download-2.0.php) for Visual C++.
Note: if the Visual Studio CLI is not in your PATH, you'll need to specify the full path to cl.exe.
Note: sdl2path refers to the directory where you extracted the SDL2 development libs.



# Resources:

<http://gbdev.gg8.se/wiki/articles/Pan_Docs>

<http://marc.rawer.de/Gameboy/Docs/GBCPUman.pdf>

<https://realboyemulator.wordpress.com/>

<http://gameboy.mongenel.com/dmg/opcodes.html>

<http://problemkaputt.de/pandocs.htm#cpuinstructionset>

<https://github.com/shonumi/gbe-plus/>

<http://imrannazar.com/GameBoy-Emulation-in-JavaScript:-The-CPU>

<http://bgb.bircd.org/#downloads>

<https://github.com/CTurt/Cinoop/tree/master/source>

<http://www.codeslinger.co.uk/pages/projects/gameboy.html>

<https://github.com/zanders3/gb>

<https://github.com/ocornut/imgui>

<https://github.com/ocornut/imgui/pull/336/commits/a592303b70789ffb53a768201f83e1d97fc8cd41>

