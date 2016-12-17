# AmungoFx3Dumper
Stream dumper for Amungo's NTLab's nt1065 board

Usage: AmungoFx3Dumper FX3_IMAGE NT1065_CFG  OUT_FILE|stdout  SECONDS|inf DRV_TYPE

FX3_IMAGE  - name of img file with firmware for cypress FX3 chip
NT1065_CFG - name of hex file with nt1065 configuration
OUT_FILE   - output file name, use 'stdout' as a file name to direct signal to standart output stream
SECONDS    - number of seconds to dump, use 'inf' to dump until Ctrl-C is pressed
DRV_TYPE   - driver installed in your system. Use 'cypress' or 'libusb'


Example (dumping one minute of signal and use cypress driver):
AmungoFx3Dumper AmungoItsFx3Firmware.img  nt1065.hex  dump4ch.bin  60  cypress

Example (dumping signal non-stop to stdout):
AmungoFx3Dumper AmungoItsFx3Firmware.img  nt1065.hex  stdout  inf  libusb