# anitaGSE
`
vi unpack/utilityFunctions.h # add #include <cstring>
declare -x CPPFLAGS=-I$ANITA_FLIGHT_SOFT_DIR/common
declare -x FLIGHTSOFT_LIBDIR=$ANITA_FLIGHT_SOFT_DIR/lib
cd ~/anitagse
./configure
cd wvreader
vi Makefile
remove -all static from LDFLAGS
make
`
