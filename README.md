OB One
======

URN's in house low latency outside broadcast receiver/transmitter for Linux. OB One uses the OPUS codec to reduce tranmission size at relative low cost to audio quality. 

## Compatibility

Tested on Debian and Arch based Linux Distributions. RX tested on Raspberry Pi models B+ and 2. For TX capabillities an addition soundcard such as Cirrus Logic Audio Card required.

## Dependencies
- libopus-dev
- libasound2-dev

## Installation 
Clone reposity via `git clone https://github.com/urn/ob_one.git` and browse into the Linux directory. From the terminal run the command:

`make` 

and then copy binaries into `/usr/local/`

## Usage 
`tx [options] [destination]`

`rx [options]`

### Options
Flags follow standard GNU convention.
`-b -B -c -f -p -q -r -v'

