#include "help.h"
#include <string>

const std::string& GetHelp() {
    const static std::string txt = R"(
atracdenc is a tool to encode to ATRAC1 and decode from ATRAC1 format

Usage:
atracdenc {-e | --encode | -d | --decode} -i <in> -o <out>

-e or --encode		encode file
-d or --decode		decode file
-i			path to input file
-o			path to output file
-h			print help and exit

Advanced options:
--bfuidxconst		Set constant amount of used BFU
--bfuidxfast		Enable fast search of BFU amount
--notransient[=mask]	Disable transient detection and use optional mask
			to set bands with forced short MDCT window

Examples:
Encode in to ATRAC1 (SP)
	atracdenc -e -i my_file.wav -o my_file.aea
Decode from ATRAC1 (SP)
	atracdenc -d -i my_file.aea -o my_file.wav
)";

    return txt;
}
