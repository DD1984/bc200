test fw based on zephyr os

#### build:
```
west update
ZEPHYR_TOOLCHAIN_VARIANT=gnuarmemb GNUARMEMB_TOOLCHAIN_PATH=/usr/ west build -b nrf52840dk_nrf52840 -d fw/build fw/
```
