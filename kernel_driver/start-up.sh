rmmod spidev
insmod si4463_802154.ko
./lowpan-tools-0.3.1/src/iz add wpan-phy0
ifconfig wpan0 up