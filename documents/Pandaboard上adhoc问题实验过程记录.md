Pandaboard上adhoc问题实验过程记录


（一）尝试更换系统  
通过下载Android源代码编译的AOSP系统  
使用官方仓库提供的内核代码，地址  
https://android.googlesource.com/kernel/omap.git/+/android-omap-panda-3.0  
版本为3.0.31  
系统版本使用4.4.4，编译后可正常运行，但在打开adhoc模式时内核崩溃

（二）尝试更换内核  
（1）
Linaro提供了一个3.4版本的内核仓库  
https://git.linaro.org/landing-teams/working/ti/kernel.git  
但在2012年已经停止维护  
尝试了不同tag以及配置文件的多个组合，进行编译替换内核。有些启动时停止在“starting kernel”的位置，有一些在内核启动的过程之中崩溃
（2）  
在网上发现了一个为pandaboard提供的内核仓库  
https://eewiki.net/display/linuxonarm/PandaBoard  
包含有较新版本的内核以及Ubuntu文件系统  
选择了3.10.24版本（对应edison的3.10.17）  
仿照其他开发板的替换内核过程  
http://android.serverbox.ch/?p=1278  
使用页面上提供的U-boot，内核可以在pandaboard上启动  
然后将Android系统放到sd卡上，通过修改bootloader可以引导系统启动。通过串口可以看到到达命令提示符的位置。但是系统很不稳定，不断提示进程退出（untracked pid xxx exited），而且插上usb线后没有反应，无法打开wifi

（三）  
（1）尝试替换内核代码  
思路是将3.2版本内核中的/net/mac80211，/net/wireless文件夹以及include中的相关文件替换为3.10版本内核中对应的文件  
由于两个版本差异较大，而且include机制也有变化，在修改的过程中，所涉及的文件不断增加，错误也越来越多。在修改了一段时间后，感觉无法继续  
（2）尝试修改内核代码  
思路是通过内核官网www.kernel.org提供的补丁日志对内核代码进行逐一修改  
经过查询，初步将范围确定在2011年11月至2013年10月之间的有关IBSS的补丁  
但在修改的过程中发现，由于补丁是基于当时最新版本的文件进行增量修改，因此补丁之间存在着比较复杂的依赖关系。有时在按照一个补丁进行修改时，发现其上下文已在其他地方进行了改动，或者补丁中用到的一些结构或变量没有定义，就需要去查找与它相关的多个补丁进行先行修改。有一些定义首次出现在哪一个补丁中有时也很难确定。

因此只能先选取一些与其他代码部分关联较小的补丁，并根据补丁的功能描述进行筛选，选择一些相关性比较强的（去掉一些处理AP、mesh或者安全认证相关）补丁，对3.2版本内核进行修改  
修改过程的列表地址  
https://github.com/lchao-bit/linaro-kernel/commits/master  
但是经测试还是没有能解决问题
