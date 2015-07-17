Bytewalla和DTN2之上的应用
===
 * 控制DTN2和bytewalla的程序在文件夹dtn2-bytewalla-app文件夹中。   
 * 编译到Edison板子上只需要make就行，如果编译到pandaboard上需要先配置好android ndk-build，然后执行ndk-build即可。   
 * 执行该应用前，需要保证DTN服务都开启，dtnsend命令可以被执行，且DTN程序收到发给自己的bundle后会在特定的日志目录下记录一个文件，文件名就该bundle的ID。
 * 应用参数如下：   
 -t :dtn2=0|bytewalla=1(type of DTN,default is dtn2)        
 -p :(payload dir)     
 -d :destination eid      
 -m :source eid      
 -s :shared dir      
 -l :dtn2|bytewalla received bundle log dir     
 -r :this program's running log file path    
 * 如果需要修改调用的dtnsend命令的参数，可以再dtn2-bytewalla-app/jni/file_watch.c文件的dtn2_send()或者bytewa_send()函数中修改
  
 
Bytewalla上的控制应用
===
 * bytewalla上层的控制应用其实就是一个dtnsend命令，该命令用来将发送bundle的执行传递给bytewalla，然后进行相应的操作。源码在bytewalla-controler/jni/中
 * 编译过程只需要利用ndk-build即可完成。
