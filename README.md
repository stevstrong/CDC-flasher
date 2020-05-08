# CDC-flasher
A command line utility to upload user program using the CDC bootloader.


The core was developed as a PowerShell script, from which an EXE file is generated using [PS2EXE Gui](https://gallery.technet.microsoft.com/scriptcenter/PS2EXE-GUI-Convert-e7cb69d5).


Features:
- input parameter is the STM32F1 binary file to be flashed as absolute path
- the flasher looks for a COM port having [1eaf:0004] as vid:pid to generate a software reset on the device
- then another COM port with [1eaf:0002] is used for program upload.

Usage:

cdc_flasher <binary_file> <serial_com_prt> <page_size_in_kb>

wherein:

<binary_file> = name of the binary file to flash including absolute path

<serial_com_port> = USB serial port where the device can communicate with the host

<page_size_in_kb> = indicates the size of a page to be flashed. 1 for F1, and 2 for F3.

The flasher tries to generate a reset over the given serial com port.
In the same time it looks for the bootloader identificable with VID:PID of 1EAF:0002.
If the bootloader is found then the binary file will be uploaded.
If the serial com port is found then a magic sequence is sent over that port to reset the device. As a result of the reset, the bootloader will be active so that the upload will be possible as described above.
