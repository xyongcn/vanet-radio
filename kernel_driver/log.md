# VANET射频模块开发日志
本文件描述在射频模块开发过程中遇到的问题和解决过程。所有日志按时间逆序排列。

## 201510112 Edison板子的spi模块工作不稳定问题和解决方法

1. source poky/oe-init-build-env
2. bitbake virtual/kernel -c menuconfig 会开始下载内核
3. config文件位于edison-src/device-software/meta-edison/recipes-kernel/linux/files
4. kernel代码位于edison-src/build/tmp/work/edison-poky-linux/linux-yocto/3.10.17+gitAUTOINC+6ad20f049a_c03195ed6e-r0/linux-edison-standard-build
	注意source指向../linux



系统崩溃的处理方式：
https://communities.intel.com/message/277202

yocto系统修改内核代码：
http://www.yoctoproject.org/docs/1.6.1/dev-manual/dev-manual.html#patching-the-kernel
	理解：在build/tmp中的代码每次都是从git中checkout出来的。
	跟着教程走好像不太好使，直接在kernel的bbappend文件中加入patch好使。
	bitbake -c cleansstate linux-yocto

需要加入CONFIG_SPI_SI4463
../device-software/utils/flash/postBuild.sh



问题：把spidev作为可加载模块后，需要arduino应用程序跑一遍才能激活spi，不然运行spidev_test测试程序没有反应。（不掉电重启不需要重复）
	解决方式：完全写好后变成内置模块！（或是改写probe函数？？）
	【可能已解决：直接用si4463模块测试getCTS可以输出（重新刷机后测试）~更新：发现还是没有解决，并没有规律】

gpio获取的问题：
1、试试spi没有激活时获取，再激活（测试成功）
2、试试不同的号码

CS的问题：（arduino测试时,SPI.transfer(wordb)）因为write方法是一个字一个字的调用，所以CS没有效果！！
1、尝试一次只传输一个字
	成功了，但是出现问题：开机后没有打开arduino程序时partinfo可以输出，但后面的两个命令没有输出，芯片严重发热；反之，partinfo没有输出【尝试一下message方法、0617更新：测试spidev原模块也是输出有问题，所以跟方法无关。】
		猜测可能有两个原因：1、处理1字节的命令有问题；2、在驱动中直接进行2次重置
2、尝试使用message方法

3、测试直接使用111号（IO10）的片选。
	失败，想借用逻辑分析仪试试

1、加入了SDN的控制
2、尝试使用spi自带的api


20150624：
成功调通了收发，包括irq的检测。但是还有如下问题：
	1、bug: sleep！
		在中断函数中调用clearirq函数报错
		有可能是wait_for_completion引起睡眠的问题！
		在async方法中，自定义的片选信号不好弄！！
		如果在spiwrite函数中阻塞等待done信号，系统会被卡死！
		尝试加上一个异步处理队列！
	2、异步处理队列中，太多的写操作导致读操作饿死！
		尝试让读操作每次都插到第一位。（可能存在问题！）
	
	3、出现了初始化收发包正常，用程序调用就出问题！
		这是由于使用iot eclipse编译出来的程序会默认将pin脚复用器进行初始化！





0629,看Arduino代码的线索:
SPI.cpp: i586-uclibc\libraries\spi\src\
edison_fab_b/variant.h

发现初始化依然存在问题：中断不工作了。尝试直接移植arduinoIDE的相关代码
	发现问题出在IO6的设置没搞对
发现sleep 1秒后，spi输出为0
	需要关闭spi 的PM（power management）。
	echo on > /sys/devices/pci0000\:00/0000\:00\:07.1/power/control


0716, 修改了中断处理，加入一个中断处理线程；把中断处理的自旋锁改成了信号量（防止忙等死锁）
