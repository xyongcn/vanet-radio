编译
====
执行make即可完成编译，如果是交叉编译，则替换相应的编译器。若是编译到Android，利用ndk-build即可完成。

dtn2和bytewalla部分的差异
====
 参数：   
 Option:   
 -t:dtn2=0|bytewalla=1(type of DTN,default is dtn2)   
 -p:payload dir    
 -d:destination eid    
 -m:source eid    
 -s:shared dir    
 -l:dtn2|bytewalla received bundle log dir   
 -r:this program's running log file path    
 
 根据参数中-t指定的是0或者1从而却分是DTN2还是bytewalla,总的来说没有区别，只是在执行dtnsend脚本的时候有差异，在file_watch.c文件的dtn2_send函数中，修改cmd字符串即可达到修改相应的dtnsend命令。   
