## ![](resources/icon.png) BetterSpades

* Replicate of the great game *Ace of Spades* (classic voxlap)
* runs on very old systems back to OpenGL 1.1
* shares similiar if not even better performance to voxlap

#### Why should I use this instead of ...?

* free of any Jagex code, they can't shut it down
* open for future expansion
* no hidden bugs

### Quick usage guide

If you just got this from the releases page or even the [official website](https://aos.party/) this might be relevant to you.

* You can either use it temporarily by extracting the downloaded zip to a new directory and start the client by the command line as described on the last steps of the build guide for Windows or Linux
* or you extract all contents to your Ace of Spades installation (normally found at `C:/Ace of Spades/`), effectively replacing the old voxlap version


## Build requirements

These libraries and files are needed:

| Name        | License         | Usage                 | Github                                            |
| ----------- | --------------- | --------------------- | :-----------------------------------------------: |
| GLFW3       | *ZLib*          | OpenGL context        | [Link](https://github.com/glfw/glfw)              |
| OpenAL soft | *LGPL-2.1*      | 3D Sound env          | [Link](https://github.com/kcat/openal-soft)       |
| dr_wav      | *Public domain* | wav support           | [Link](https://github.com/mackron/dr_libs/)       |
| LodePNG     | *MIT*           | png support           | [Link](https://github.com/lvandeve/lodepng)       |
| libdeflate  | *MIT*           | decompression of maps | [Link](https://github.com/ebiggers/libdeflate)    |
| enet        | *MIT*           | networking library    | [Link](https://github.com/lsalzman/enet)          |
| inih        | *BSD-3.Clause*  | .INI file parser      | [Link](https://github.com/benhoyt/inih)           |
| http        | *Public domain* | http client library   | [Link](https://github.com/mattiasgustavsson/libs) |
| parson      | *MIT*           | JSON parser           | [Link](https://github.com/kgabis/parson)          |

You will need to compile

* GLFW3
* OpenAL soft
* libdeflate
* and enet

by yourself or get hold of precompiled binaries. Place produced static libaries in `lib/`.

Some files need to be copied to the `src/` directory.

This means:

| source                    | &rightarrow; | destination                                                                   |
| ------------------------- | ------------ | ---------------------------                                                   |
| `dr_wav.h`                | &rightarrow; | `src/dr_wav.c` <br /> Make sure to define `DR_WAV_IMPLEMENTATION` in *Line 9* |
| `lodepng.h`               | &rightarrow; | `src/lodepng/lodepng.h`                                                       |
| `lodepng.cpp`             | &rightarrow; | `src/lodepng/lodepng.c`                                                       |
| `libdeflate.h`            | &rightarrow; | `src/libdeflate.h`                                                            |
| `ini.c` and `ini.h`       | &rightarrow; | `src/ini.c` and `src/ini.h`                                                   |
| `parson.c` and `parson.h` | &rightarrow; | `src/parson.c` and `src/parson.h`                                             |
| `http.h`                  | &rightarrow; | `src/http.h`                                                                  |

Because state of copyright of 0.75 assets is unknown, you will need to get them *[here](http://aos.party/bsresources.zip)*. Unzip the file and extract all contents to `resources/` manually.

#### Windows

This project uses CMake to generate all Makefiles automatically. It's best to use MinGW for gcc on windows. You can generate the required files by opening `cmd.exe` in the `build/` directory and typing:
```
cmake -G "MinGW Makefiles" ..
make
```
You might need to replace `make` by `mingw32-make` if cmd tells you it could not find the first.

If everything went well, the client should be in the `build/bin/` subfolder.

You can start the client the same way as you did with voxlap version.

See this example:
```
client.exe -aos://16777343:32887
```

#### Linux

Compilation now works the same on Linux. Just change the build system to `Unix Makefiles` or leaving it as default will probably work too (`cmake ..`).

You can build each library yourself, or install them with your distro's package manager:
```
sudo apt-get install libgl1-mesa libgl1-mesa-dev libopenal1 libopenal-dev libglfw-dev libenet-dev
```
(this does not include libdeflate or lodepng which are a requirement too, see _Wiki/Building_ for more details)

Start the client with the following inside the `build/bin/` directory:
```
./client -aos://16777343:32887
```

#### macOS

The same instructions for Linux work on macOS aside from some minor differences. First, use Homebrew or MacPorts to grab dependencies:
```
brew install glfw enet
```
The development headers for OpenAL and OpenGL don't have to be installed since they come with macOS by default. libdeflate and lodepng should be installed and placed manually in a way similar to Linux. See _Wiki/Building_ for more details.

## Gallery

| <img src="/docs/pic01.png" width="250px"> | <img src="/docs/pic02.png" width="250px"> | <img src="/docs/pic03.png" width="250px"> |
| :-: | :-: | :-: |

##

>*Donations are greatly appreciated* :+1:
>
><img src="https://bitaps.com/static/img/bitcoin.svg" height="30px"> `1AeSfdVmbEX6VCqxCgk9WkzSA8XJkKr4FM`

<center>![GPLv3](https://www.gnu.org/graphics/gplv3-127x51.png)</center>
