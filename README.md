OB One
======

URN's in house low latency outside broadcast receiver/transmitter for Linux and OSX. OB One uses the OPUS codec to reduce tranmission size at relative low cost to audio quality. 

## Compatibility

Tested on Debian and Arch based Linux Distributions. RX tested on Raspberry Pi models B+ and 2. For TX capabillities an additional soundcard such as Cirrus Logic Audio Card required.

Tested on OSX 10.10 (Yosemite).

## Dependencies
Opus Codec 			http://www.opus-codec.org
Portaudio 			http://www.portaudio.com
ALSA *(Linux only)*	http://www.alsa-project.org

For a quick installation of dependecies on Linux use your package manager, for example for Debian based distros:

`apt-get install libopus-dev libasound2-dev portaudio19-dev`

and for OSX users (who have macports installed)

`port install portaudio libopus`

## Installation 
Clone reposity via `git clone https://github.com/urn/ob_one.git` and browse into the operating system directory of choice. From the terminal run the command:

`make` 

and then `sudo make install`

## Usage 
`tx [options] [destination]`

`rx [options]`

### Options
Flags follow standard GNU convention.

`-b -B -c -d -f -p -q -r -v`

**b:** Bytes per frame per channel *(default 96)* Sets the desired size of bytes OPUS should convert a frame of audio.

**B:** Buffer size *(RX only, default 50)* Sets the size of the playback buffer in milliseconds.

**c:** No. Channels *(default 2)* sets between mono and stereo.

**d:** Device *(default 0)* integer value which selects the audio output to send audio to. Set this value to 0 use the OS default (which can be set in your OS sound settings).

**f:** Frame size *(default 480)* set in bytes the size of the audio frame to be passed onto OPUS for conversion. For example a 480 byte frame with a sample rate of 48000 Hz equates to 10 ms of audio. **Note:** Due to OPUS contraints the frame size can only be 120,240,480,960,1920 or 2880 bytes. 

**p:** Port No. *(default 1350)* Set the port number to send the tranmission/listen to the transmission on. Only one TX/RX per port.

**q:** Quiet mode.

**r:** Sample rate (default 48000) sets the record and playback sample rate of hardware in Hz.

**v:** Verbose mode. 

