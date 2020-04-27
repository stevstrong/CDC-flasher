# CDC-flasher
A command line utility to upload user program using the CDC bootloader.


The core was developed as a PowerShell script, from which an EXE file is generated using [PS2EXE Gui](https://gallery.technet.microsoft.com/scriptcenter/PS2EXE-GUI-Convert-e7cb69d5).


Features:
- input parameter is the STM32F1 binary file to be flashed as absolute path
- the flasher looks for a COM port having [1eaf:0004] as vid:pid to generate a software reset on the device
- then another COM port with [1eaf:0002] is used for program upload.
