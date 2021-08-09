bc200 - bike computer from aliexpress

#### unlock debugging:
jlink connected to SDO and SCLK testpoints  
erase all data  (exclude qspi flash) and protection, and programm only full flash exclude protection
```
./nrfjprog --jdll <full path>/JLink_Linux_V752a_x86_64/libjlinkarm.so -f NRF52 --log --recover
./nrfjprog --jdll <full path>/JLink_Linux_V752a_x86_64/libjlinkarm.so -f NRF52 --log --program full_flash.bin
```

#### how to make jlink from bluepill:
https://mysensors-rus.github.io/Blue-pill-to-JLink/ https://github.com/GCY/JLINK-ARM-OB

#### how to get full flash:
https://github.com/atc1441/ESP32_nRF52_SWD https://limitedresults.com/2020/06/nrf52-debug-resurrection-approtect-bypass/

#### reading qspi flash content:
```
faketime -f "x0.1" ./nrfjprog --jdll <full path>/JLink_Linux_V752a_x86_64/libjlinkarm.so -f NRF52 --log --qspiini nrfjprog_qspi_gd25q127c.ini --readqspi qspi.bin
```
`faketime -f "x0.1"` - need because nrfjprog work with my defective jlink from bluepill continuously only 32sec and this time is not enough for reading qspi flash

#### usbfw_decode:
util for decrypt and encrypt firmware and font for usb upgrade

#### fw:
fw start from 0x31000  
spi flash contain font from 0 offset and fatfs disk from 0x300000 offset
