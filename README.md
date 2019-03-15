[![Build Status](http://bytebit.info.tm/jenkins/buildStatus/icon?job=BetterSpades)](http://bytebit.info.tm/jenkins/job/BetterSpades/)
[![](https://img.shields.io/github/downloads/xtreme8000/BetterSpades/total.svg)](https://github.com/xtreme8000/BetterSpades/releases)
[![GPL](https://img.shields.io/badge/license-GPL--3.0-red.svg)](https://github.com/xtreme8000/BetterSpades/blob/standalone/LICENSE)
[![DonateBTC](https://img.shields.io/badge/bitcoin-donate-yellow.svg)](https://github.com/xtreme8000/BetterSpades#donate)

[![Google Play](/docs/android.png)](https://play.google.com/store/apps/details?id=party.aos.betterspades)
![GPL v3](https://www.gnu.org/graphics/gplv3-127x51.png)

## ![](docs/icon_small.png) BetterSpades

* Replicate of the great game *Ace of Spades* (classic voxlap)
* runs on very old systems back to OpenGL 1.1 (OpenGL ES support too)
* shares similar if not even better performance to voxlap
* can run on *"embedded"* systems like a [Steam Link](https://store.steampowered.com/app/353380/Steam_Link/)

#### Why should I use this instead of ...?

* free of any Jagex code, they can't shut it down
* open for future expansion
* easy to use
* no hidden bugs

### Quick usage guide

**As of right now, you can download the newest stable version from the [releases page](https://github.com/xtreme8000/BetterSpades/releases).**

**You can get [nightly builds here](http://bytebit.info.tm/jenkins/job/BetterSpades/).**

You can either:
* use the client temporarily by extracting the downloaded zip into a new directory.
* extract all contents to your current Ace of Spades installation directory (normally found at `C:/Ace of Spades/`), effectively replacing the old voxlap version

## System requirements

| Type    | min. requirement                                     |
| ------- | ---------------------------------------------------- |
| OS      | Windows 98 or Linux                                  |
| CPU     | 1 GHz single core processor                          |
| GPU     | 64MB VRAM, Mobile Intel 945GM or equivalent          |
| RAM     | 256MB                                                |
| Display | 800x600px                                            |
| Others  | Keyboard and mouse<br />Dial up network connection   |


## Build requirements

These libraries and files are needed:

| Name         | License         | Usage                  | GitHub                                            |
| ------------ | --------------- | ---------------------- | :-----------------------------------------------: |
| GLFW3        | *ZLib*          | OpenGL context         | [Link](https://github.com/glfw/glfw)              |
| OpenAL soft  | *LGPL-2.1*      | 3D Sound env           | [Link](https://github.com/kcat/openal-soft)       |
| inih         | *BSD-3.Clause*  | .INI file parser       | [Link](https://github.com/benhoyt/inih)           |
| stb_truetype | *Public domain* | TrueType font renderer | [Link](https://github.com/nothings/stb)           |
| dr_wav       | *Public domain* | wav support            | [Link](https://github.com/mackron/dr_libs/)       |
| http         | *Public domain* | http client library    | [Link](https://github.com/mattiasgustavsson/libs) |
| LodePNG      | *MIT*           | png support            | [Link](https://github.com/lvandeve/lodepng)       |
| libdeflate   | *MIT*           | decompression of maps  | [Link](https://github.com/ebiggers/libdeflate)    |
| enet         | *MIT*           | networking library     | [Link](https://github.com/lsalzman/enet)          |
| parson       | *MIT*           | JSON parser            | [Link](https://github.com/kgabis/parson)          |
| log.c        | *MIT*           | logger                 | [Link](https://github.com/xtreme8000/log.c)       |
| GLEW         | *MIT*           | OpenGL extensions      | [Link](https://github.com/nigels-com/glew)        |

You will need to compile the following by yourself, or get hold of precompiled binaries:

* GLFW3
* GLEW
* OpenAL soft *(only needed on Windows)*
* libdeflate
* enet

Follow the instructions on their project page, then place produced static libraries in `libs/`.

Some files need to be copied to the `src/` directory.

This means:

| source                        | &rightarrow; | destination                                                                              |
| ----------------------------- | ------------ | ---------------------------------------------------------------------------------------- |
| `dr_wav.h`                    | &rightarrow; | `src/dr_wav.c` <br /> Make sure to add `#define DR_WAV_IMPLEMENTATION` on the first line |
| `lodepng.h` and `lodepng.cpp` | &rightarrow; | `src/lodepng/lodepng.h` and `src/lodepng/lodepng.c`                                      |
| `libdeflate.h`                | &rightarrow; | `src/libdeflate.h`                                                                       |
| `ini.c` and `ini.h`           | &rightarrow; | `src/ini.c` and `src/ini.h`                                                              |
| `parson.c` and `parson.h`     | &rightarrow; | `src/parson.c` and `src/parson.h`                                                        |
| `http.h`                      | &rightarrow; | `src/http.h`                                                                             |
| `stb_truetype.h`              | &rightarrow; | `src/stb_truetype.h`                                                                     |
| `log.h` and `log.c`           | &rightarrow; | `src/log.h` and `src/log.c`                                                              |


Because state of copyright of 0.75 assets is unknown, you will need to get them *[here](http://aos.party/bsresources.zip)*. Unzip the file and extract all contents to `resources/` manually.

#### Windows

This project uses CMake to generate all Makefiles automatically. It's best to use MinGW for GCC on Windows. You can generate the required files by opening `cmd.exe` in the `build/` directory and typing:
```
cmake -G "MinGW Makefiles" ..
make
```
You might need to replace `make` by `mingw32-make` if cmd tells you it could not find the first.

If everything went well, the client should be in the `build/bin/` subfolder.

When starting `client.exe`, you will be greeted by a server list. Select a server on it to start playing!
You can also start the client the same way as you did with the voxlap version by opening cmd and putting an `aos://` link in as the first argument:

```
client.exe -aos://16777343:32887 //Connects to a local server
```

#### Linux

Compilation now works the same on Linux. Just change the build system to `Unix Makefiles` or leaving it as default will probably work too (`cmake ..`).

You can build each library yourself, or install them with your distro's package manager:
```
sudo apt-get install libgl1-mesa libgl1-mesa-dev libopenal1 libopenal-dev libglfw-dev libenet-dev libglew-dev
```
(this does not include libdeflate or lodepng which are a requirement too, see [_Wiki/Building_](https://github.com/xtreme8000/BetterSpades/wiki/Building) for more details)

Start the client e.g. with the following inside the `build/bin/` directory:
```
./client -aos://16777343:32887
```

#### macOS

The same instructions for Linux work on macOS aside from some minor differences. First, use Homebrew or MacPorts to grab dependencies:
```
brew install glfw enet
```
The development headers for OpenAL and OpenGL don't have to be installed since they come with macOS by default. libdeflate and lodepng should be installed and placed manually in a way similar to Linux. See [_Wiki/Building_](https://github.com/xtreme8000/BetterSpades/wiki/Building) for more details.

## Gallery

| <img src="/docs/pic01.png" width="250px"><br />*quite old* | <img src="/docs/pic02.png" width="250px"><br />*quite old* | <img src="/docs/pic03.png" width="250px"> |
| :-: | :-: | :-: |
| <img src="/docs/pic04.png" width="250px"><br />*grenade fun* | <img src="/docs/pic05.png" width="250px"><br />*falling block animation* | <img src="/docs/pic06.png" width="250px"><br />*sniping on normandie* |
