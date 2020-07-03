# CDC-flasher
A command line utility to upload user program using the CDC bootloader.
Currently only Windows is supported as platform.


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

### How to use CDC flasher with Arduino IDE:

- copy into the file `STM32F1/boards.txt` the [following lines](https://github.com/stevstrong/Arduino_STM32/blob/62a37fec36a3aa026d3873dc4fb08fb0481d56c1/STM32F1/boards.txt#L380-L384):

```
genericSTM32F103C.menu.upload_method.CDCUploadMethod=CDC bootloader 1.0
genericSTM32F103C.menu.upload_method.CDCUploadMethod.upload.tool=cdc_upload
genericSTM32F103C.menu.upload_method.CDCUploadMethod.build.upload_flags=-DSERIAL_USB -DGENERIC_BOOTLOADER
genericSTM32F103C.menu.upload_method.CDCUploadMethod.build.vect=VECT_TAB_ADDR=0x8001000
genericSTM32F103C.menu.upload_method.CDCUploadMethod.build.ldscript=ld/cdc_bootloader.ld
```
- copy into the file `STM32F1/platform.txt` [these lines](https://github.com/stevstrong/Arduino_STM32/blob/62a37fec36a3aa026d3873dc4fb08fb0481d56c1/STM32F1/platform.txt#L181-L190):
```
# CDC upload 1.0
tools.cdc_upload.cmd=cdc_upload
tools.cdc_upload.cmd.windows=cdc_upload.bat
tools.cdc_upload.path.windows={runtime.hardware.path}/tools/win
#tools.cdc_upload.path.macosx={runtime.hardware.path}/tools/macosx
#tools.cdc_upload.path.linux={runtime.hardware.path}/tools/linux
#tools.cdc_upload.path.linux64={runtime.hardware.path}/tools/linux64
tools.cdc_upload.upload.params.verbose=
tools.cdc_upload.upload.params.quiet=
tools.cdc_upload.upload.pattern="{path}/{cmd}" "{build.path}/{build.project_name}.bin" {serial.port.file} 1
```
- copy the file https://github.com/stevstrong/Arduino_STM32/blob/master/STM32F1/variants/generic_stm32f103c/ld/cdc_bootloader.ld to the folder `STM32F1/variants/generic_stm32f103c/ld/`

- restart Arduino IDE
- select upload method: `CDC bootloader 1.0`.
