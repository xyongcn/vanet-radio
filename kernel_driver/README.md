#DESCRIPTION
1. The SI4463 kernel driver module has been modified to fit the ieee802154 mac layer[1,2,3].
 * [1] https://code.google.com/p/linux-wsn/wiki/LinuxStart
 * [2] kernel\Documentation\networking\ieee802154.txt
 * [3] http://elinux.org/images/7/71/Wireless_Networking_with_IEEE_802.15.4_and_6LoWPAN.pdf
1. The configuring tool is the low-pan-tools[4].
 * [4] http://sourceforge.net/projects/linux-zigbee

#COMPILING

1. make

#CONFIGURING
1. patch the edison kernel with 0001-default-chan-for-802154.patch (for yocto release 2.0), rebuild the edison-image.
	* http://www.yoctoproject.org/docs/1.6.1/dev-manual/dev-manual.html#patching-the-kernel
1. Move .ko file and tools/* into edison. 
1. tar -xvzf tools/lowpan-tools-0.3.1.tar.gz
2. chmod -R 777 *
3. ./start-up.sh
4. A test example is tools/test-input-count.c
