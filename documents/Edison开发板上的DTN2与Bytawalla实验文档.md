## DTN2在Edison开发板上的环境搭建、移植与使用 ##

李超

2015/5/11

### 一、重新安装官方系统镜像（已有的数据会被清除） ###
（1）从官方网站下载系统镜像Yocto complete image
https://software.intel.com/en-us/iot/hardware/edison/downloads

（2）将压缩包解压到Edison驱动器的根目录

（3）连接开发板，执行reboot重新启动

（4）在提示“Hit any key to stop autoboot”时，按任意键进入”boot >”提示符
 
（5）运行run do_ota


### 二、调整根目录大小并制作镜像 ###
**编译环境Ubuntu 12.04.5 x64**

（1）安装系统工具

sudo apt-get install build-essential git diffstat gawk chrpath texinfo libtool gcc-multilib

（2）下载镜像源代码Linux Source Files

https://software.intel.com/en-us/iot/hardware/edison/downloads

（3）解压后进入到源代码目录

**※注：以下操作不要使用root账户**

（4）在源代码根目录位置，确定edison.env文件位置

find * -name edison.env

（5）找到edison.env文件的partition部分，将其中的rootfs后面的数字改为1536MiB

（6）在源代码根目录位置，确定edison-image.bb位置

find * -name edison-image.bb

（7）修改edison-image.bb文件中的IMAGE_ROOTFS\_SIZE字段值为1572864

（8）在源代码根目录位置，确定wpa-supplicant_2.1.bbappend文件位置

find * -name wpa-supplicant\_2.1.bbappend

（9）将wpa-supplicant_2.1.bbappend文件中的

SRC\_URI = "${BASE\_SRC\_URI} \ git://android.googlesource.com/platform/external/wpa\_supplicant\_8;protocol=https;tag=android-4.4.4\_r2.0.1"

改为

SRC\_URI = "${BASE\_SRC\_URI} \ git://github.com/wujingbang/wpa\_supplicant\_8.git;protocol=https;tag=android-4.4.4\_r2.0.1"

（8）在源代码根目录执行
make setup

（9）执行
make image

（10）在源代码根目录下执行

cd out/current/build/toFlash，编译完成的镜像在此处

（11）安装驱动工具

apt-get install dfu-util

（12）安装远程控制工具
apt-get install screen

（13）连接开发板，打开新的终端，使用screen登陆

sudo screen /dev/ttyUSB0 115200

（14）在镜像目录执行

./flashall.sh

（15）看到要求开发板重启的提示后，在screen终端中执行
reboot

（16）查看分区状态

df -h

### 三、开发板与PC之间的文件传输 ###

（1）开发板访问usb分区

rmmod g_multi

mkdir /update （只有首次需要，可以指定任意的空目录）

losetup -o 8192 /dev/loop0 /dev/disk/by-partlabel/update

mount /dev/loop0 /update

（2）计算机访问usb分区

umount /update

modprobe g_multi

### 四、开发板使用wifi ###

configure_edison --wifi

按照提示进行连接

### 五、Tcl的编译 ###

（1）下载Tcl8.5版本

http://www.tcl.tk/software/tcltk/download.html

（2）进入目录

cd tcl8.5.18

cd unix

（3）进行编译

./configure --prefix=/opt/edisontcl

make

make install

### 六、Berkeley DB的编译 ###


（1）下载Berkeley DB 5.3版本

http://www.oracle.com/technetwork/database/database-technologies/berkeleydb/downloads/index-082944.html

（2）进入目录

cd db-5.3.28

cd build_unix

（3）进行编译

../dist/configure --prefix=/opt/edisondb --enable-cxx --enable-stl --enable-tcl --with-tcl=/opt/edisontcl/lib

make

make install

### 七、oasys的编译 ###
（1）使用mercurial工具下载最新版本的oasys

hg clone http://hg.code.sf.net/p/dtn/oasys dtn-oasys

（2）将oasys的目录重命名为“oasys”

（3）进入oasys文件夹

（4）生成编译文件

./configure --with-tcl=/opt/edisontcl --with-db=/opt/edisondb

（5）修改Rules.make

在加入EXTRA_CFLAGS后加上-fpermissive

（6）修改tclcmd/tclreadline.c
将

rl\_attempted\_completion\_function = (CPPFunction *) TclReadlineCompletion;

改为

rl_attempted_completion_function = (rl\_completion\_func\_t *) TclReadlineCompletion;

（来源：http://www.ietf.org/mail-archive/web/dtn-users/current/msg01804.html）

（7）开始编译

make

### 八、DTN2的编译 ###
（1）使用mercurial工具下载最新版本的oasys

hg clone http://hg.code.sf.net/p/dtn/DTN2 dtn-DTN2

（2）将DTN2文件夹放置在与oasys同样的位置

（3）修改开发板系统中的/etc/opkg/base-feeds.conf文件

加入

src all http://iotdk.intel.com/repos/1.1/iotdk/all

src x86 http://iotdk.intel.com/repos/1.1/iotdk/x86

src i586 http://iotdk.intel.com/repos/1.1/iotdk/i586

（4）连接网络，执行

opkg update

（5）安装以下工具包

opkg install perl-module-extutils-makemaker

opkg install perl-module-extutils-mm-unix

opkg install perl-module-utf8

opkg install perl-module-config-git

opkg install perl-dev

opkg install perl-module-extutils-command

opkg install perl-module-extutils-install

opkg install perl-module-extutils-mkbootstrap

opkg install libssp-dev

（6）生成编译文件

./configure --disable-ecl --disable-edp

（7）修改Rules.make

在加入EXTRA_CFLAGS后加上-fpermissive

（8）开始编译

make

（9）创建程序运行过程中需要的临时目录

mkdir /var/dtn

mkdir /var/dtn/db

mkdir /var/dtn/bundles

（10）将dtn.conf放置于/etc/文件夹下

（11）复制Tcl的库文件到系统目录

cd /opt/edisontcl/lib

cp -rf * /usr/lib/

（12）首次执行程序，需要初始化数据库

./dtnd --init

### 九、配置adhoc模式 ###

（1）建立如下内容的脚本，命名为wpacli\_ibss\_open.sh
<pre><code>if [ $# != 1 ] ; then
 echo "$0 <SSID>"
 exit
fi
$rmt wpa_cli -iwlan0 disconnect
$rmt wpa_cli -iwlan0 remove_network all
$rmt wpa_cli -iwlan0 add_network
$rmt wpa_cli -iwlan0 set_network 0 frequency 2412
$rmt wpa_cli -iwlan0 set_network 0 mode 1
if [ ! -n "$rmt" ] ; then
$rmt wpa_cli -iwlan0 set_network 0 ssid \"$1\"
else
$rmt wpa_cli -iwlan0 set_network 0 ssid '\"'$1'\"'
fi
$rmt wpa_cli -iwlan0 set_network 0 auth_alg OPEN
$rmt wpa_cli -iwlan0 set_network 0 key_mgmt NONE
$rmt wpa_cli -iwlan0 set_network 0 scan_ssid 1
$rmt wpa_cli -iwlan0 select_network 0
$rmt wpa_cli -iwlan0 enable_network 0
$rmt wpa_cli -iwlan0 status
</code></pre>

（2）搜索当前存在的网络名称

wpa\_cli scan\_results

（3）连接到某个网络

sh wpacli\_ibss\_open.sh 网络id

（4）配置IP地址	

ifconfig wlan0 （IP地址）

（5）配置网关地址

route add default gw （网关地址）

配置不正确会导致DTN2发现机制不正常工作，报Network unreachable的错误

（6）查看状态

wpa_cli status

### 十、SSH多终端窗口 ###
在ubuntu环境下

（1）停止network manager

stop network-manager

（2）启动usb0网络接口

ifconfig usb0 up

（3）配置usb0接口的网络地址

ifconfig usb0 192.168.2.2

注意IP地址不要设置为192.168.2.15，此地址为开发板保留

（4）查看与开发板的连接

ping 192.168.2.15

（5）使用ssh连接到终端

ssh root@192.168.2.15

### 十一、DTN2简单使用方法 ###
（1）首先启动DTN后台主服务

执行daemon文件夹下的dtnd

一些参数：

-d 后台运行

-o 记录日志文件

-l 在日志中输出信息的级别

（2）主服务启动后，会显示如“XXX-dtn%”的提示符

在此提示符下可以查看当前DTN节点的状态

如查看连接状态：link dump

查看bundle状态：bundle list，bundle info等

具体信息对应的命令可以参考命令的help输出信息

（3）在发送bundle时，在另外的终端中找到apps文件夹下的dtnsend工具

示例如下：

dtnsend -s localhost -d dtn://edison1.dtn -t m -p “hello”

-s 后面是本节点的eid

-d 后面是目标节点的eid

-t 为负载类型

-p 为实际负载

-D 要求目的节点收到bundle之后发送应答bundle至源节点

### 十二、DTN2配置文件 ###

DTN2的配置文件dtn.conf需要放在/etc文件夹下

需要注意以下几个部分：

设置本节点的eid名称

route local_eid “dtn://edison1.dtn”

设置路由算法

route set type flood

添加网络端口

interface add tcp0 tcp

添加连接

link add link1 dtn://edison2.dtn ONDEMAND tcp

注：如果配置文件中的连接在主进程中报不一致添加失败的错误，需要删除
之前的bundle并重新初始化程序

rm -rf /var/dtn

mkdir /var/dtn

重启之后

dtnd --init

添加路由表项

route add dtn://edison2.dtn/* tcp0

发现机制设置

discovery add disc0 ip port=9556

discovery announce tcp0 disc0 tcp interval=5

注：如果设置发现机制则route与link部分不需要填写，程序会自动添加

### 十三、Edison开发板启动运行脚本 ###
（1）在/etc/init.d目录之下建立需要运行的脚本

示例如下：
<pre><code>#!/bin/sh
sleep 30
sh /home/wpacli_ibss_open.sh Edison
sleep 5
ifconfig wlan0 192.168.5.15
sleep 1
route add default gw 192.168.5.1
sleep 1
rm -rf /var/dtn
sleep 1
mkdir /var/dtn
/home/DTN2/daemon/dtnd -d -o test.log
</code></pre>

（2）为脚本增加可执行权限

chmod +x /etc/init.d/bootup_edison.sh

（3）将脚本添加到启动序列中

cd /etc/init.d

update-rc.d bootup_edison.sh defaults

（4）如有需要可以将脚本从启动序列中移除

cd /etc/init.d

update-rc.d -f bootup_edison.sh remove


### 十四、DTN2与bytewalla的static之间的相互通信注意事项 ###
（1）首先需要配置adhoc网络，至于是否运行aodv则看需要

（2）需要将配置文件（可以在代码中修改，也可以在bytewalla的config按钮中修改）中，RouterSetting部分的router_type改为static。

（3）RouterSetting 部分添加static路由中必要的下一跳路由，并且在LinksSetting中加入相应的配置

（4）启动bytewalla中的service，如果要发送bundle则点击send按钮输入相应的目标节点eid进行发送，发送的信息是sdcard根目录下的test_0.5M.mp3文件。如果需要修改相应的内容既可以修改该文件内容，也可以在代码里面修改android.geosvr.dtn.apps.DTNSend.java 中的SendMessage()函数的内容。

（5）Bytewalla发送完成bundle后会有BundleTrasmitted的通知栏提示，如果bytewalla收到的一个bundle会有BundlRecived通知栏提醒。

### 十五、DTN2与bytewalla的epidemic之间的相互通信注意事项 ###
（1）首先需要配置adhoc网络，至于是否运行aodv则看需要

（2）其次，需要将配置文件（可以在代码中修改，也可以在bytewalla的config按钮中修改）中，RouterSetting部分的router_type改为epidemic

（3）启动bytewalla中的service，epidemic会自动发现在adhoc网络中的其他DTN节点，在截面的linkSection部分可以看到已经建立的配置。

（4）如果要发送bundle则点击send按钮输入相应的目标节点eid进行发送，发送的信息是sdcard根目录下的test_0.5M.mp3文件。如果需要修改相应的内容既可以修改该文件内容，也可以在代码里面修改android.geosvr.dtn.apps.DTNSend.java 中的SendMessage()函数的内容

（5）Bytewalla发送完成bundle后会有BundleTrasmitted的通知栏提示，如果bytewalla收到的一个bundle会有BundlRecived通知栏提醒。

**Bytewalla代码: https://github.com/thedevilking/bytewalla-epidemic**

### 十六、Edison修改时间 ###
（1）停止自动对时服务

systemctl disable systemd-timesyncd

（2）修改时间

timedatectl set-time "2015-06-06 12:00:00"

### 十七、Edison上蓝牙的使用 ###
（1）程序地址

https://github.com/xyongcn/bt-adb-shell/tree/master/edison

（2）蓝牙模块开机默认可发现和可配对

修改/etc/bluetooth/main.conf中的

DiscoverableTimeout = 0

PairableTimeout = 0

（3）在脚本中添加启动程序

rfkill unblock bluetooth

/home/bt-shell-edison &

（4）控制端确认蓝牙连接

l2ping 蓝牙地址

（5）在出现“input：”提示后，输入

bleconnect 蓝牙地址


### 十八、tcpdump中出现 unreachable - need to frag 问题的解决 ###
使用如下命令进行分包
 iptables -t filter -I FORWARD -p tcp --tcp-flags SYN,RST,ACK SYN -j TCPMSS --set-mss 1400
 
 iptables -t filter -I FORWARD -p tcp --tcp-flags SYN,RST,ACK SYN,ACK -j TCPMSS --set-mss 1400

