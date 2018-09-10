# A Gameboy Emulator #

![screenshot](https://github.com/donfreiday/gb/blob/master/screenshot.png)

An unfinished cross-platform Gameboy emulator written in C++ using SDL2. Can be compiled to Javascript and run in a browser by way of LLVM and Emscripten.

**What works**: 
Enough to play Tetris!!!

**What doesn't (yet):**: 
MBC support, window layer of graphics, sound, other stuff

## Usage ##

**Native:**
gb rom.gb

**Javascript:**
emrun gb.html

## Boot ROM ##

The boot ROM is responsible for scrolling the Nintendo logo when the DMG is powered on.
The emulator will look for ./roms/bios.gb and, if found, execute it prior to the game ROM.

## Dependencies ##

SDL2, ncurses, make, clang, emscripten and LLVM for javascript build

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

Dependencies TBA

- - - -

### Ubuntu ###

Native:

```shell
sudo apt update && sudo apt upgrade
sudo apt install git make clang++ libsdl2-dev libncurses5-dev libncursesw5-dev
git clone https://github.com/donfreiday/gb.git && cd gb && make
```

Javascript:
The version of emscripten in Ubuntu's repository is from 2014 (stability ftw).
You'll have to compile emscripten, as well as LLVM most likely.

- - - -

### Windows: ###

TBD


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
