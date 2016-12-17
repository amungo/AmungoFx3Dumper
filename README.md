# AmungoFx3Dumper
Stream dumper for Amungo's NTLab's nt1065 board

## Usage:
AmungoFx3Dumper FX3_IMAGE NT1065_CFG  OUT_FILE|stdout  SECONDS|inf DRV_TYPE

_FX3_IMAGE_  - name of img file with firmware for cypress FX3 chip

_NT1065_CFG_ - name of hex file with nt1065 configuration

_OUT_FILE_   - output file name, use 'stdout' as a file name to direct signal to standart output stream

_SECONDS_    - number of seconds to dump, use 'inf' to dump until Ctrl-C is pressed

_DRV_TYPE_   - driver installed in your system. Use 'cypress' or 'libusb'

## Examples:
* dumping one minute of signal and use cypress driver:
 * AmungoFx3Dumper AmungoItsFx3Firmware.img  nt1065.hex  dump4ch.bin  60  cypress
* dumping signal non-stop to stdout:
 * AmungoFx3Dumper AmungoItsFx3Firmware.img  nt1065.hex  stdout  inf  libusb

## File format
The program dumps all data from a FX3 board without any changes. Usually the format is:

* 4 channels
* 2 bits per sample per channel
* Overall 8 bits (1 byte) per one sample for all channels
* Decode table is
 * 00b ->  1
 * 01b ->  3
 * 10b -> -1
 * 11b -> -3

You can use AmgExtract to extract several channels from output file and converting
data to usual _signed char_ format. AmgExtract is located here: https://github.com/amungo/AmgExtract
