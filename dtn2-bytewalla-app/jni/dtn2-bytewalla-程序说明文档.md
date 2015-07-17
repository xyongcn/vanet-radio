# dtn2和bytewalla上层应用
## 软件说明：   
>每个节点建立一个共享目录，当把需要发送给目标节点的文件放到这个目录的时候，应用将调用dtn2来发送这个文件。应用在被启动的时候需要指定相应的共享文件目录、dtn2或者bytewalla的payload目录、DTN类型、收到bundle的日志目录等。

## 程序编译己运行
>参考readme.md

## 代码结构
* 主要代码都在file\_watch.c中,queue.c、queue.h、file_queue.c、file_queue.h都是相应的队列的代码。
* file_watch.c中，main函数中会对输入的参数作处理分析。对输入的参数作错误的判断，然后启动两个线程，一个监视共享文件夹目录，另一个监视dtn收到的bundle的id目录。

* watchFile函数则是监控文件事件。   
>1. 如果共享目录里面添加了新文件则会先将文件名和文件保存到一个临时文件里面，然后调用DTN程序来发送这个文件；   
>2. 如果是DTN收到了bundle，那么它会在日志文件夹里面新建一个文件，文件名为这个bundle的id，当程序监控到这个文件的关闭事件后，就从DTN的payload文件夹里面读取这个bundle的内容，并还原到共享文件夹中。   
>3. 此外还会将收到的bundle的id加入到队列中，这样就不会触发对收到还原的文件进行发送的事件。   

* print_usage函数中显示程序的帮助信息

* exec_shell则是调用shell命令,由于发送的bundle时调用dtnsend命令，所以该命令一定要在系统的环境变量中

* dtn2_send和bytewalla_send函数则是两种不同的dtn程序下调用dtnsend命令的区别。具体的命令差异和参数差异可以在这个函数里面修改。如调用dtn发送的命令改成别的命令，参数也不一样了，只需要在函数里面修改即可完成。