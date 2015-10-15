#DTN2在Pandaboard平台上的交叉编译与移植  
李超  
2015年9月  
##一、代码编译  
###（1）交叉编译器
操作系统：Ubuntu 12.04.5 64位。  
使用如下版本的交叉编译器进行代码编译。  
> arm-none-linux-gnueabi  
> gcc version 4.4.1 (Sourcery G++ lite 2010q1-202)
####配置交叉编译器：
进入到交叉编译器的bin目录下，执行命令  
> export PATH=`pwd`:$PATH

配置成功的标志为，离开当前目录，在其他任意位置执行
> arm-none-linux-gnueabi-gcc

显示结果  
`arm-none-linux-gnueabi-gcc: no input files`

###（2）TCL的交叉编译
####进入编译目录
>cd tcl8.5.18/unix
####执行命令
> export tcl`_`cv`_`strtod_buggy=1  
export ac`_`cv`_`func`_`strtod=yes

否则会出现错误

fixstrtod.o: In function \`fixstrtod':
fixstrtod.c:(.text+0x0): multiple definition of` \`fixstrtod'  
strtod.o:strtod.c:(.text+0x0): first defined here  
collect2: ld returned 1 exit status
####执行配置
> CC=arm-none-linux-gnueabi-gcc ./configure --prefix=/opt/linuxtcl --build='uname–m' --host=arm-none-linux-gnueabi --cache-file=cache`_`file`_`o
####执行编译与安装
>make  
make install

###（3）Berkeley DB的交叉编译
####进入编译目录
>cd db-5.3.28/build_unix
####执行配置
> CC=arm-none-linux-gnueabi-gcc ../dist/configure --with-mutex=ARM/gcc-assembly --prefix=/opt/linuxdb --build=x86_64 --host=arm-none-linux-gnueabi -enable-cxx --enable-stl --enable-tcl --with-tcl=/opt/linuxtcl/lib
####执行编译与安装
>make  
make install

###（4）oasys的交叉编译
####进入编译目录
>cd oasys
####执行配置
>CC=arm-none-linux-gnueabi-gcc ./configure --build=x86_64 --host=arm-none-linux-gnueabi  --with-db=/opt/linuxdb --with-tcl=/opt/linuxtcl
####执行编译
>make
###（5）Openssl的交叉编译
####下载并交叉编译openssl
下载openssl源代码  
`https://www.openssl.org/source/`
####配置环境
>export TARGETMACH=arm-none-linux-gnueabi  
export BUILDMACH=x86`_`64  
export CROSS=arm-none-linux-gnueabi  
export CC=${CROSS}-gcc  
export LD=${CROSS}-ld  
export AS=${CROSS}-as  
export AR=${CROSS}-ar  
./Configure -DOPENSSL_NO_HEARTBEATS --openssldir=/home/openssl/ shared os/compiler:arm-none-linux-gnueabi-

####执行编译与安装
>make  
make install

###（6）DTN2的交叉编译
####进入编译目录
>cd DTN2
####生成配置文件
>./build-configure.sh

有可能需要安装autoconf工具
####执行配置
>CC=arm-none-linux-gnueabi-gcc ./configure --disable-ecl --disable-edp --build='uname -m' --host=arm-none-linux-gnueabi
####修改Rules.make文件
在`INCFLAGS`后加入
>-I/home/openssl/include/
####修改apps/dtntunnel/TCPTunnel.cc文件
在头部加入  
>\#ifndef IP`_`TRANSPARENT  
\#   warning Using hardcoded value for IP_TRANSPARENT as libc headers do not define it.  
\#   define IP`_`TRANSPARENT 19  
\#endif  

####修改apps/dtntunnel/UDPTunnel.cc文件
在头部加入
>\#ifndef IP`_`TRANSPARENT  
\#   warning Using hardcoded value for IP`_`TRANSPARENT as libc headers do not define it.  
\#   define IP`_`TRANSPARENT 19  
\#endif  
\#ifndef IP`_`ORIGDSTADDR
\#   warning Using hardcoded value for IP`_`ORIGDSTADDR as libc headers do not define it.
\#   define IP`_`ORIGDSTADDR 20  
\#endif  
\#ifndef IP`_`RECVORIGDSTADDR  
\#   warning Using hardcoded value for IP_RECVORIGDSTADDR as libc headers do not define it.  
\#   define IP`_`RECVORIGDSTADDR IP`_`ORIGDSTADDR  
\#endif

####执行编译
>make

##二、代码移植
###（1）依赖库准备
`libdb-5.3.so`（在Berkeley DB的交叉编译安装目录中）  
`libtcl8.5.so`（在TCL的交叉编译安装目录中）  
`libdl-2.11.1.so`（在交叉编译器目录的/arm-none-linux-gnueabi/libc/lib目录下）  
`libpthread2.11.1.so`（在交叉编译器目录的/arm-none-linux-gnueabi/libc/lib目录下）  
`libstdc++.so.6.0.12`（在交叉编译器目录的/arm-none-linux-gnueabi/libc/usr/lib/目录下）  
`libc-2.11.1.so`（在交叉编译器目录的/arm-none-linux-gnueabi/libc/lib/目录下）  
`libm-2.11.1.so`（在交叉编译器目录的/arm-none-linux-gnueabi/libc/lib/目录下）  
`libgcc_s.so.1`（在交叉编译器目录的/arm-none-linux-gnueabi/libc/lib/目录下）  
`ld-2.11.1.so`（在交叉编译器目录的/arm-none-linux-gnueabi/libc/lib/目录下）
###（2）依赖库配置
####查看连接器所处的位置
使用命令
>readelf –l dtnd

查看`Requesting program interpreter`后的地址  
dtnd位于DTN2目录中的daemon目录下

####按照上述指示地址，首先将ld-2.11.1.so复制到相应目录下
建立连接
>ln -s ld-2.11.1.so ld-linux.so.3

赋予权限
>chmod 777 ld-linux.so.3

####将libgcc_s.so.1复制到/system/lib目录下

####将libtcl8.5.so复制到/system/lib目录下

####将libdb-5.3.so复制到/system/lib目录下

####将libm-2.11.1.so复制到/system/lib目录下
建立连接
>ln -s libm-2.11.1.so libm.so.6

####将libdl-2.11.1.so复制到/system/lib目录下
建立连接
>ln -s libdl-2.11.1.so libdl.so.2

####将libpthread-2.11.1.so复制到/system/lib目录下
建立连接
>ln -s libpthread-2.11.1.so libpthread.so.0

####将libstdc++.so.6.0.12复制到/system/lib目录下
建立连接
>ln -s libstdc++.so.6.0.12 libstdc++.so.6

####将libc-2.11.1.so复制到/system/lib目录下
建立连接
>ln -s libc-2.11.1.so libc.so.6

###（3）DTN2相关配置
####将daemon目录下的dtnd以及apps目录下需要用到的程序复制到开发板上  
####将dtn.conf文件放置在/etc/目录下  
####添加默认网关地址  
命令形式：
>route add default gw 192.168.1.1 dev adhoc0

####修改系统时间
命令形式：
>date -s “20150922.220000”

小数点前为日期，小数点后为时间
