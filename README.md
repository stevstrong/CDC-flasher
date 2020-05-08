# CDC-flasher
A command line utility to upload user program using the CDC bootloader.


### Usage:
```
cdc_flasher.exe <binary_file> <serial_com_port> <page_size_in_kb>
```
wherein:

- <binary_file> = name of the binary file to flash including absolute path

- <serial_com_port> = USB serial port where the device can communicate with the host

- <page_size_in_kb> = indicates the size of a page to be flashed. 1 for F1, and 2 for F3.


### How does it work:

The flasher tries to generate a reset over the given serial com port.

In the same time it looks for the bootloader identificable with VID:PID of 1EAF:0002.

If the bootloader is found then the binary file will be uploaded.

If the serial com port is found then a magic sequence is sent over that port to reset the device. As a result of the reset, the bootloader will be active so that the upload will be possible as described above.
