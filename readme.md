# A Gameboy Emulator
Author: Don Freiday

![screenshot](https://github.com/donfreiday/gb/blob/master/screenshot.png)

My very first emulator.

Implemented so far: enough to play Tetris!!!

Not implemented: window layer of graphics, sound, other stuff

#Usage
gb rom.gb

#Dependencies 
SDL2, ncurses, make

# Build Instructions

Ubuntu:
I spun up a 18.04 minimal VM and this worked:
~~~~
sudo apt update && sudo apt upgrade
sudo apt install git make g++ libsdl2-dev libncurses5-dev libncursesw5-dev
git clone https://github.com/donfreiday/gb.git && cd gb && make
~~~~

Arch: Todo, I already installed dependencies etc on my machine. Plus if you're running Arch you can probably figure it out ;-)

Windows: TBD, haven't put much effort in here


#Resources:

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
