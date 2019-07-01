#!/bin/sh
if ! test -f README.md; then
	echo "This must be run from the BetterSpades directory"
	exit 1
fi

wget -O src/ini.h https://raw.githubusercontent.com/benhoyt/inih/master/ini.h
wget -O src/ini.c https://raw.githubusercontent.com/benhoyt/inih/master/ini.c
wget -O src/stb_truetype.h https://raw.githubusercontent.com/nothings/stb/master/stb_truetype.h
wget -O src/libdeflate.h https://raw.githubusercontent.com/ebiggers/libdeflate/master/libdeflate.h
wget -O src/http.h https://raw.githubusercontent.com/mattiasgustavsson/libs/master/http.h
wget -O src/parson.h https://raw.githubusercontent.com/kgabis/parson/master/parson.h
wget -O src/parson.c https://raw.githubusercontent.com/kgabis/parson/master/parson.c
wget -O src/log.h https://raw.githubusercontent.com/xtreme8000/log.c/master/src/log.h
wget -O src/log.c https://raw.githubusercontent.com/xtreme8000/log.c/master/src/log.c

mkdir -p src/lodepng
wget -O src/lodepng/lodepng.h https://raw.githubusercontent.com/lvandeve/lodepng/master/lodepng.h
wget -O src/lodepng/lodepng.c https://raw.githubusercontent.com/lvandeve/lodepng/master/lodepng.cpp

wget -O src/dr_wav.c https://raw.githubusercontent.com/mackron/dr_libs/master/dr_wav.h
patch -p0 --forward < dr_wav.patch

echo "Done."
