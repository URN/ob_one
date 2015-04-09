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

`-b -B -c -f -p -q -r -v`

**b:** Bytes per frame *(default 96)* Sets the desired size of bytes OPUS should convert a frame of audio.

**B:** Buffer size *(RX only, default 25)* Sets the size of the playback buffer in milliseconds. *(Recommended size 200ms).

**c:** No. Channels *(default 2)* sets between mono and stereo.

**f:** Frame size *(default 480)* set in bytes the size of the audio frame to be passed onto OPUS for conversion. **Note:** Due to OPUS contraints the frame size can only be 120,240,480,960,1920 or 2880 bytes. 

**p:** Port No. *(default 1350)* Set the port number to send the tranmission/listen to the transmission on. Only one tr/rx per port.

**q:** Quiet mode.

**r:** Sample rate (default 48000) sets the record and playback sample rate of hardware.

**v:** Verbose mode. 

