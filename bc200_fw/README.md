#### build:
```
git clone https://github.com/DD1984/bc200
cd bc200/bc200_fw/
west update
ZEPHYR_TOOLCHAIN_VARIANT=gnuarmemb GNUARMEMB_TOOLCHAIN_PATH=/usr/ west build -b nrf52840dk_nrf52840 -d fw/build fw/
```
fw for usb upgrade will be placed here: _fw/build/zephyr/BC200_Upgrade_zephyr.bin_
