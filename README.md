coospo bc200 - bike computer from [aliexpress](https://aliexpress.ru/item/4001156919018.html)

#### spec:
PCB revision: v5.4
 - cpu: nrf52840 - cortex-m4 (integrated BLE, USB); 256kb ram; 1mb flash
 - external flash: GD25Q127C - qspi 16mb nor flash
 - display: BTG-160240D - monochrome, sunreadeble lcd with backlight have 160x240 resolution and based on ST75320 controller
 - 4 buttons
 - 1300mah lipo battery + charger
 - 12mm piezo buzzer
 - gps + glonass + beidou module
 - hx3203 - light sensor
 - qmp6988 - digital barometric pressure sensor
 
#### bc200_fw:
test fw based on zephyr os ([build instructions](https://github.com/DD1984/bc200/blob/master/bc200_fw/README.md))  
works:
- [x] lcd display + contrast + backlight + lvgl graphic lib
- [x] buttons
- [x] qspi flash + fatfs + usb mass storage
- [x] fw upgrade via usb usign original image format (**pls downgrade fw to 1.5.3 or late first, with 1.5.9 something go wrong**). upgrade possible in both direction orig fw <-> zephyr os fw ([video of process](https://youtu.be/bRdI50IbiIM))
- [x] qspi flash speed fixed by updating nrf sdk

#### unlock debugging:
jlink connected to SDO and SCLK testpoints  
erase all data  (exclude qspi flash) and protection, and programm only full flash exclude protection
```
nrfjprog -f NRF52 --log --recover
nrfjprog -f NRF52 --log --program full_flash.bin
```
switch NFC antenna pins to GPIOs - used as buttons (first cmd enable write possibility to UICR,second - write needed value)
```
nrfjprog -f NRF52 --ramwr 0x4001E504 --val 0x00000001
nrfjprog -f NRF52 --ramwr 0x1000120C --val 0xfffffffe
```

#### how to make jlink from bluepill:
https://mysensors-rus.github.io/Blue-pill-to-JLink/  
https://github.com/GCY/JLINK-ARM-OB  
http://forum.easyelectronics.ru/viewtopic.php?p=651817#p651817

#### how to get full flash:
https://github.com/atc1441/ESP32_nRF52_SWD  
https://limitedresults.com/2020/06/nrf52-debug-resurrection-approtect-bypass/

#### reading qspi flash content:
```
faketime -f "x0.1" ./nrfjprog --jdll <full path>/JLink_Linux_V752a_x86_64/libjlinkarm.so -f NRF52 --log --qspiini nrfjprog_qspi_gd25q127c.ini --readqspi qspi.bin
```
`faketime -f "x0.1"` - need because nrfjprog work with my defective jlink from bluepill continuously only 32sec and this time is not enough for reading qspi flash

#### usbfw_decode:
util for decrypt and encrypt firmware and font for usb upgrade

#### original fw:
fw start from 0x31000  
spi flash contain font from 0 offset and fatfs disk from 0x380000 offset
