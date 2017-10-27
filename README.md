## ![](/icon.png) BetterSpades

* Replicate of the great game *Ace of Spades* (classic voxlap)
* runs on very old systems back to OpenGL 1.1
* shares similiar if not even better performance to voxlap

## Build requirements

These libraries and files are needed:

| Name | License | Usage | Github |
| ---- | ------- | ----- | :----: |
| GLFW3 | *ZLib* | OpenGL context | [Link](https://github.com/glfw/glfw) |
| OpenAL soft | *LGPL-2.1* | 3D Sound env| [Link](https://github.com/kcat/openal-soft) |
| dr_wav | *Public domain* | wav support | [Link](https://github.com/mackron/dr_libs/blob/master/dr_wav.h) |
| LodePNG | *MIT* | png support | [Link](https://github.com/lvandeve/lodepng) |
| libdeflate | *MIT* | decompression of maps | [Link](https://github.com/ebiggers/libdeflate) |
| enet | *MIT* | networking library | [Link](https://github.com/lsalzman/enet) |

#### Windows

You can compile the client on windows by starting `compile.bat`.
This will also run the generated executable automaticly.

* `run_min_gfx.bat` will launch the client with the lowest and least demanding settings
* `run_max_gfx.bat` is the most you can get

See the client's source to understand what each value inside the script means.

#### Linux

Compilation should start after running `make`. Note that this is still experimental and the build script might not be up to date. This will be improved after a refractor of the entire build process, thus making it uniform on every platform.

## Gallery

| <img src="/docs/pic01.png" width="250px"> | <img src="/docs/pic02.png" width="250px"> |
| :-: | :-: |

##

>*Donations are greatly appreciated* :+1:
>
><img src="https://bitaps.com/static/img/bitcoin.svg" height="30px"> `1AeSfdVmbEX6VCqxCgk9WkzSA8XJkKr4FM`
