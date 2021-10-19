#### packages that need to be installed before build:
```
sudo apt install git make cmake ninja-build gcc gcc-arm-none-eabi python3-pip
pip3 install --user -U west pyelftools
```
#### build:
```
git clone https://github.com/DD1984/bc200
cd bc200/bc200_fw/
~/.local/bin/west update
ZEPHYR_TOOLCHAIN_VARIANT=gnuarmemb GNUARMEMB_TOOLCHAIN_PATH=/usr/ ~/.local/bin/west build -b nrf52840dk_nrf52840 -d fw/build fw/
```
fw for usb upgrade will be placed here: [fw/build/zephyr/BC200_Upgrade_zephyr.bin](https://github.com/DD1984/bc200/tree/master/bc200_fw/fw/build/zephyr)

TODO: need to filter downloading zephyr os packages to decrease build time
