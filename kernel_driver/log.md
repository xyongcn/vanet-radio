# VANET射频模块开发日志
本文件描述在射频模块开发过程中遇到的问题和解决过程。所有日志按时间逆序排列。

## 20151104
发现一个关于gpio设置的问题（会导致发送数据包不产生中断）
	出错的设置：#define RF_GPIO_PIN_CFG 0x13, 0x1B, 0x63, 0x60, 0x61, 0x00, 0x00, 0x00
	正确的设置：#define RF_GPIO_PIN_CFG 0x13, 0x00, 0x00, 0x20, 0x21, 0x00, 0x00
	【新测试】gpio0的设置需要是1B，否则有的包收不到。

si4463：rf4463f30天线开关设置：GPIO2：RX_STATE； GPIO3：TX_STATE


500Kbps：6.25B/100us

【存在一个严重的bug：如果在等待输入数据前先产生了中断，就卡死了。】

实测若使用中断方式，速度太慢。发现有时while(!is_tx_fifo_almost_empty())会卡死。


GPIO1电平变高后会延迟下降。考虑这么修改：把正在发送的缓冲区与指针保存到全局变量中，在中断处理函数中往fifo写数据。

【问题已经解决，是由于mtu默认后，len长度为1500，超过了缓冲区】
	修改mtu有效。

试一下把tx函数全注释，只剩ringbuffer，会不会死机？【会死机】
	经过注释定位，发现问题出现在enqueue和dequeue上。
	经过注释定位，发现问题出在copy_cmd函数中的内存拷贝！
错误3：
root@edison:~# [   42.978988] BUG: sleeping function called from invalid context   at /home/wu/Desktop/edison/old-version/edison-src/build/tmp/work/edison-poky-li  nux/linux-yocto/3.10.17+gitAUTOINC+6ad20f049a_c03195ed6e-r0/linux/arch/x86/mm/fa  ult.c:1142
[   42.999630] in_atomic(): 0, irqs_disabled(): 0, pid: 232, name: mdnsd
[   43.000891] BUG: unable to handle kernel NULL pointer dereference at 00000290
[   43.000977] IP: [<c1765ae0>] __ip_dev_find+0x50/0x100
[   43.001045] *pdpt = 0000000035102001 *pde = 0000000000000000
[   43.001119] Oops: 0000 [#1] PREEMPT SMP
[   43.001182] Modules linked in: rf(O) usb_f_acm u_serial g_multi libcomposite   bcm_bt_lpm bcm4334x(O) [last unloaded: spidev]
[   43.001339] CPU: 1 PID: 232 Comm: mdnsd Tainted: G           O 3.10.17-yocto-  standard #1
[   43.001416] Hardware name: Intel Corporation Merrifield/BODEGA BAY, BIOS 466   2014.06.23:19.20.05
[   43.001499] task: f5f21640 ti: f5570000 task.ti: f5570000
[   43.001558] EIP: 0060:[<c1765ae0>] EFLAGS: 00010246 CPU: 1
[   43.001619] EIP is at __ip_dev_find+0x50/0x100
[   43.001670] EAX: c1bf2a00 EBX: f5ecbcc0 ECX: 00000000 EDX: f5eadc00
[   43.001734] ESI: 0f02a8c0 EDI: 00000000 EBP: f5571c88 ESP: f5571c3c
[   43.001798]  DS: 007b ES: 007b FS: 00d8 GS: 0033 SS: 0068
[   43.001856] CR0: 8005003b CR2: 00000290 CR3: 35110000 CR4: 001007f0
[   43.001919] DR0: 00000000 DR1: 00000000 DR2: 00000000 DR3: 00000000
[   43.001980] DR6: ffff0ff0 DR7: 00000400
[   43.002022] Stack:
[   43.002050]  c132b360 00000000 c1bf2a00 f51178c0 f5dac240 000000db 00000000 f  5571bd0
[   43.002180]  c132b360 00100100 00200200 f5117980 f5ef0e40 000000db 00000000 f  5571bd0
[   43.002309]  f5571d58 c1bf2a00 f506c2c0 f5571cd8 c1734bc3 000000db 00000000 f  5571bd0
[   43.002438] Call Trace:
[   43.002486]  [<c132b360>] ? poll_select_copy_remaining+0x100/0x100
[   43.002563]  [<c132b360>] ? poll_select_copy_remaining+0x100/0x100
[   43.002640]  [<c1734bc3>] __ip_route_output_key+0xb3/0x760
[   43.002709]  [<c132b360>] ? poll_select_copy_remaining+0x100/0x100
[   43.002783]  [<c14b47ba>] ? delay_tsc+0x7a/0xc0
[   43.002845]  [<c14b46de>] ? __const_udelay+0x1e/0x20
[   43.002910]  [<c1735289>] ip_route_output_flow+0x19/0x50
[   43.002978]  [<c15ccad7>] ? dwc3_ep0_start_trans+0x77/0xf0
[   43.003047]  [<c176150f>] udp_sendmsg+0x6df/0x870
[   43.003114]  [<c173b120>] ? ip_copy_metadata+0x140/0x140
[   43.003195]  [<c176a57b>] inet_sendmsg+0x6b/0x90
[   43.003258]  [<c16f99ab>] sock_sendmsg+0x7b/0xb0
[   43.003320]  [<c14b46de>] ? __const_udelay+0x1e/0x20
[   43.003385]  [<c15cca37>] ? dwc3_send_gadget_ep_cmd+0x87/0xb0
[   43.003468]  [<c16fa30f>] SYSC_sendto+0xdf/0x110
[   43.003539]  [<c12b04c1>] ? handle_irq_event_percpu+0xe1/0x210
[   43.003612]  [<c1206d7b>] ? native_sched_clock+0x2b/0xd0
[   43.003681]  [<c1270be5>] ? sched_clock_cpu+0xe5/0x140
[   43.003747]  [<c12b0600>] ? handle_irq_event+0x10/0x50
[   43.003814]  [<c16fa7ad>] SyS_sendto+0x2d/0x30
[   43.003875]  [<c16fb138>] SyS_socketcall+0x208/0x2f0
[   43.003943]  [<c1896ba8>] syscall_call+0x7/0xb
[   43.004009]  [<c1890000>] ? ext4_mb_discard_group_preallocations+0x228/0x2a3
[   43.004078] Code: 79 af ff 8b 45 bc c1 eb 18 8b 1c 9d e0 4f db c1 85 db 75 0d   eb 45 90 8d 74 26 00 8b 1b 85 db 74 3a 39 73 18 75 f5 8b 53 0c 8b 3a <3b> 87 90   02 00 00 75 e8 85 ff 74 24 0f b6 45 b8 84 c0 74 09 8b
[   43.004712] EIP: [<c1765ae0>] __ip_dev_find+0x50/0x100 SS:ESP 0068:f5571c3c
[   43.004799] CR2: 0000000000000290
[   43.287854] ---[ end trace a5c3e85e3c7071a0 ]---


报错：【尝试注释掉自旋锁！】【不行!】
错误2：[  107.953275] BUG: unable to handle kernel NULL pointer dereference at   (null)
[  107.953378] IP: [<c122b8a3>] pgd_free+0x13/0x60
[  107.953443] *pdpt = 0000000000000000 *pde = c000feb1c000feac
[  107.953505] Oops: 0000 [#1] PREEMPT SMP

错误1：

[  118.519035] BUG: spinlock bad magic on CPU#0, socket_test-bun/299
[  118.519114]  lock: 0xf57eaa5c, .magic: 00000000, .owner: <none>/-1, .owner_cpu: 0
[  118.519185] CPU: 0 PID: 299 Comm: socket_test-bun Tainted: G           O 3.10.17-yocto-standard #1
[  118.519254] Hardware name: Intel Corporation Merrifield/BODEGA BAY, BIOS 466 2014.06.23:19.20.05
[  118.519321]  00000000 00000000 f56ebb18 c18965a4 f56ebb44 c189663b c1acaa6c f57eaa5c
[  118.519436]  00000000 c1ac9ec2 ffffffff 00000000 f57eaa5c c1ac9ec9 00000000 f56ebb54
[  118.519545]  c189665e f57eaa5c f51b7000 f56ebb70 c14bc409 f5414100 f5414200 f57eaa5c
[  118.519655] Call Trace:
[  118.519707]  [<c18965a4>] dump_stack+0x16/0x18
[  118.519761]  [<c189663b>] spin_dump+0x95/0x9d
[  118.519816]  [<c189665e>] spin_bug+0x1b/0x1f
[  118.519871]  [<c14bc409>] do_raw_spin_lock+0x119/0x120
[  118.519930]  [<c189b7ec>] _raw_spin_lock+0x1c/0x20
[  118.519985]  [<c1727d6f>] sch_direct_xmit+0x3f/0x190
[  118.520042]  [<c170ed5f>] dev_queue_xmit+0x17f/0x400
[  118.520099]  [<c173ad14>] ? ip_finish_output2+0x64/0x330
[  118.520158]  [<c173ae51>] ip_finish_output2+0x1a1/0x330
[  118.520218]  [<c173c088>] ip_fragment+0x648/0x800
[  118.520271]  [<c146255a>] ? selinux_ipv4_postroute+0x1a/0x20
[  118.520330]  [<c173acb0>] ? ip_reply_glue_bits+0x80/0x80
[  118.520389]  [<c1730845>] ? nf_iterate+0x75/0x80
[  118.520445]  [<c173c3ea>] ip_finish_output+0x1aa/0x390
[  118.520522]  [<c173c240>] ? ip_fragment+0x800/0x800
[  118.520583]  [<c173cf77>] ip_output+0xb7/0xc0
[  118.520635]  [<c173c240>] ? ip_fragment+0x800/0x800
[  118.520691]  [<c173c6b0>] ip_local_out+0x20/0x30
[  118.520744]  [<c173d883>] ip_send_skb+0x13/0x70
[  118.520796]  [<c1760bb0>] udp_send_skb+0x110/0x330
[  118.520852]  [<c17610b8>] udp_sendmsg+0x288/0x870
[  118.520908]  [<c173b120>] ? ip_copy_metadata+0x140/0x140
[  118.520972]  [<c12751f0>] ? put_prev_task_fair+0xd0/0x4f0
[  118.521115]  [<c176a57b>] inet_sendmsg+0x6b/0x90
[  118.521175]  [<c16f99ab>] sock_sendmsg+0x7b/0xb0
[  118.521233]  [<c12e888e>] ? find_get_page+0x5e/0xb0
[  118.521300]  [<c12eafdc>] ? filemap_fault+0xac/0x3b0
[  118.521362]  [<c189f075>] ? sub_preempt_count+0x55/0xe0
[  118.521427]  [<c16fa30f>] SYSC_sendto+0xdf/0x110
[  118.521487]  [<c189b916>] ? _raw_spin_unlock_irqrestore+0x26/0x50
[  118.521554]  [<c126d44b>] ? task_sched_runtime+0x5b/0xc0
[  118.521615]  [<c12716b4>] ? thread_group_cputime+0xa4/0xb0
[  118.521679]  [<c124595b>] ? ns_to_timespec.part.0+0x1b/0x30
[  118.521740]  [<c16fa7ad>] SyS_sendto+0x2d/0x30
[  118.521795]  [<c16fb138>] SyS_socketcall+0x208/0x2f0
[  118.521855]  [<c189bf88>] syscall_call+0x7/0xb
[  118.756605] Virtual device rf0 asks to queue packet!

大幅缩小缓冲区，会不会死机？
NETIF_F_LLTX标记可以不加锁！见netdevice文档。【还是不行】
=================================================
一个SPI驱动的例子：mrf24j40.c at86rf230.c
尝试不要缓冲区，直接在xmit中发送。（去掉cmd线程与irq线程）
	出现问题：tx不许睡眠报错！

发现mrf24j40和at86rf230中的tx都没有free skb，不知道什么原因！
编译wpan-tools：https://linuxrelatedworld.wordpress.com/2015/03/12/wpan-tools-cross-compilation/
使用：http://wpan.cakelab.org/：这个不能用。
http://sourceforge.net/projects/linux-zigbee/?source=navbar
一个解释：http://blog.csdn.net/honour2sword/article/details/45190417
【应当是linux-wsn：https://code.google.com/p/linux-wsn/wiki/LinuxStart】
=================================================
1m速率测试中发现卡死的情况，暂不明原因（猜测是isHand。。加锁的问题）【报错卡死：】
[  632.718940] BUG: unable to handle kernel NULL pointer dereference at   (null)
[  632.719038] IP: [<c1506ea0>] check_tty_count+0x30/0xb0
[  632.719108] *pdpt = 0000000000000000 *pde = c000feb1c000feac
[  632.719171] Oops: 0000 [#1] PREEMPT SMP
[  632.719224] Modules linked in: rf(O) usb_f_acm u_serial g_multi libcomposite bcm_bt_lp      m bcm4334x(O) [last unloaded: spidev]
[  632.719370] CPU: 0 PID: 226 Comm: sh Tainted: G           O 3.10.17-yocto-standard #1
[  632.719432] Hardware name: Intel Corporation Merrifield/BODEGA BAY, BIOS 466 2014.06.2      3:19.20.05
[  632.719501] task: f504ef40 ti: f519e000 task.ti: f519e000
[  632.719550] EIP: 0060:[<c1506ea0>] EFLAGS: 00010203 CPU: 0
[  632.719601] EIP is at check_tty_count+0x30/0xb0
[  632.719644] EAX: f528c1e0 EBX: 00000001 ECX: 00000000 EDX: 00002b2b
[  632.719696] ESI: c18dfa96 EDI: f528c000 EBP: f519fe70 ESP: f519fe50
[  632.719749]  DS: 007b ES: 007b FS: 00d8 GS: 0000 SS: 0068
[  632.719797] CR0: 8005003b CR2: 00000000 CR3: 01cc9000 CR4: 001007f0
[  632.719848] DR0: 00000000 DR1: 00000000 DR2: 00000000 DR3: 00000000
[  632.719898] DR6: ffff0ff0 DR7: 00000400
[  632.719932] Stack:
[  632.719957]  f528c000 f519fe68 c18969d4 00000000 f55d0a20 f528c000 f552bd80 f682dea0
[  632.720066]  f519fef0 c15097bb 00000001 00000000 f576a270 c1899c95 00000001 00000001
[  632.720172]  f6c01e80 00000001 00000001 00000001 00000001 f5c16010 f5a10750 f519fec8
[  632.720278] Call Trace:
[  632.720327]  [<c18969d4>] ? tty_lock_nested.isra.0+0x34/0x80
[  632.720389]  [<c15097bb>] tty_release+0x4b/0x4c0
[  632.720443]  [<c1899c95>] ? sub_preempt_count+0x55/0xe0
[  632.720502]  [<c1899c95>] ? sub_preempt_count+0x55/0xe0
[  632.720560]  [<c1267af4>] ? lg_local_unlock+0x24/0x50
[  632.720620]  [<c131c295>] __fput+0xd5/0x200
[  632.720673]  [<c131c45d>] ____fput+0xd/0x10
[  632.720725]  [<c125d3c1>] task_work_run+0x81/0xb0
[  632.720779]  [<c1243271>] do_exit+0x291/0x9f0
[  632.720830]  [<c133537d>] ? mntput+0x1d/0x30
[  632.720883]  [<c12449c4>] do_group_exit+0x34/0xa0
[  632.720936]  [<c1244a46>] SyS_exit_group+0x16/0x20
[  632.720988]  [<c1896ba8>] syscall_call+0x7/0xb
[  632.721030] Code: 53 83 ec 14 3e 8d 74 26 00 31 db 89 c7 b8 04 5a bc c1 89 d6 e8 62 f5       38 00 8b 8f e0 01 00 00 8d 87 e0 01 00 00 39 c1 74 0b 66 90 <8b> 09 83 c3 01 39 c1 75 f7       b8 04 5a bc c1 e8 ed f5 38 00 8b 47
[  632.721545] EIP: [<c1506ea0>] check_tty_count+0x30/0xb0 SS:ESP 0068:f519fe50
[  632.721618] CR2: 0000000000000000
[  632.938639] ---[ end trace 775cdb9b02c6c898 ]---
[  632.938691] Fixing recursive fault but reboot is needed!
[  632.938743] BUG: scheduling while atomic: sh/226/0x00000002
[  632.938789] Modules linked in: rf(O) usb_f_acm u_serial g_multi libcomposite bcm_bt_lp      m bcm4334x(O) [last unloaded: spidev]
[  632.938937] CPU: 0 PID: 226 Comm: sh Tainted: G      D    O 3.10.17-yocto-standard #1
[  632.938999] Hardware name: Intel Corporation Merrifield/BODEGA BAY, BIOS 466 2014.06.2      3:19.20.05
[  632.939065]  f504ef40 f504ef40 f519fc00 c18911d4 f519fc1c c188d09b c1a886bc f504f240
[  632.939176]  000000e2 00000002 00000009 f519fc9c c1895627 00000000 00000000 00000a6e
[  632.939284]  c1cd650c 00000a6e c1cbd840 f519e000 c1cd5c62 f73ef840 f504ef40 00000000
[  632.939393] Call Trace:
[  632.939439]  [<c18911d4>] dump_stack+0x16/0x18
[  632.939494]  [<c188d09b>] __schedule_bug+0x5e/0x70
[  632.939552]  [<c1895627>] __schedule+0x677/0x7c0
[  632.939610]  [<c188cc09>] ? printk+0x3d/0x3f
[  632.939663]  [<c1895793>] schedule+0x23/0x60
[  632.939714]  [<c12438bb>] do_exit+0x8db/0x9f0
[  632.939765]  [<c188cc09>] ? printk+0x3d/0x3f
[  632.939816]  [<c124207b>] ? kmsg_dump+0xbb/0xd0
[  632.939869]  [<c189786b>] oops_end+0x8b/0xc0
[  632.939920]  [<c188c60b>] no_context+0x1b5/0x1bd
[  632.939975]  [<c188c751>] __bad_area_nosemaphore+0x13e/0x146
[  632.940036]  [<c14b3c33>] ? vsnprintf+0x193/0x390
[  632.940090]  [<c188c770>] bad_area_nosemaphore+0x17/0x19
[  632.940148]  [<c189957b>] __do_page_fault+0xab/0x510
[  632.940206]  [<c1899c95>] ? sub_preempt_count+0x55/0xe0
[  632.940264]  [<c1899c95>] ? sub_preempt_count+0x55/0xe0
[  632.940321]  [<c18999e0>] ? __do_page_fault+0x510/0x510
[  632.940377]  [<c18999ed>] do_page_fault+0xd/0x10
[  632.940429]  [<c18970eb>] error_code+0x5f/0x64
[  632.940483]  [<c18999e0>] ? __do_page_fault+0x510/0x510
[  632.940540]  [<c1506ea0>] ? check_tty_count+0x30/0xb0
[  632.940595]  [<c18969d4>] ? tty_lock_nested.isra.0+0x34/0x80
[  632.940655]  [<c15097bb>] tty_release+0x4b/0x4c0
[  632.940708]  [<c1899c95>] ? sub_preempt_count+0x55/0xe0
[  632.940766]  [<c1899c95>] ? sub_preempt_count+0x55/0xe0
[  632.940824]  [<c1267af4>] ? lg_local_unlock+0x24/0x50
[  632.940882]  [<c131c295>] __fput+0xd5/0x200
[  632.940935]  [<c131c45d>] ____fput+0xd/0x10
[  632.940986]  [<c125d3c1>] task_work_run+0x81/0xb0
[  632.941039]  [<c1243271>] do_exit+0x291/0x9f0
[  632.941090]  [<c133537d>] ? mntput+0x1d/0x30
[  632.941143]  [<c12449c4>] do_group_exit+0x34/0xa0
[  633.161628]  [<c1244a46>] SyS_exit_group+0x16/0x20
[  633.161692]  [<c1896ba8>] syscall_call+0x7/0xb

[  634.970980] BUG: using smp_processor_id() in preemptible [00000000] code: jbd2/mmcblk0      p10/227
[  634.971091] caller is __schedule+0x37d/0x7c0
[  634.971146] CPU: 0 PID: 227 Comm: jbd2/mmcblk0p10 Tainted: G      D W  O 3.10.17-yocto      -standard #1
[  634.971215] Hardware name: Intel Corporation Merrifield/BODEGA BAY, BIOS 466 2014.06.2      3:19.20.05
[  634.971282]  00000000 00000000 f51c7e70 c18911d4 f51c7e8c c14bc881 c1ac44ac 00000000
[  634.971397]  f5f4baa0 000000e3 c1cbd840 f51c7f0c c189532d f5a1db98 00000001 00000387
[  634.971508]  ccf07a66 0000008d c1cbd840 f51c6000 f6fc4850 f73ef840 c1b86120 91827364
[  634.971619] Call Trace:
[  634.971668]  [<c18911d4>] dump_stack+0x16/0x18
[  634.971725]  [<c14bc881>] debug_smp_processor_id+0xc1/0xe0
[  634.971786]  [<c189532d>] __schedule+0x37d/0x7c0
[  634.971844]  [<c1899c95>] ? sub_preempt_count+0x55/0xe0
[  634.971903]  [<c1899c95>] ? sub_preempt_count+0x55/0xe0
[  634.971960]  [<c1260b49>] ? prepare_to_wait+0x49/0x70
[  634.972017]  [<c1895793>] schedule+0x23/0x60
[  634.972072]  [<c13f2046>] kjournald2+0x1b6/0x220
[  634.972127]  [<c1260d30>] ? wake_up_bit+0x20/0x20
[  634.972182]  [<c13f1e90>] ? commit_timeout+0x10/0x10
[  635.076609]  [<c1260234>] kthread+0x94/0xa0
[  635.076668]  [<c1899c95>] ? sub_preempt_count+0x55/0xe0
[  635.076732]  [<c189baf7>] ret_from_kernel_thread+0x1b/0x28
[  635.076789]  [<c12601a0>] ? kthread_create_on_node+0xc0/0xc0








在1k速率下于Arduino中测试长包发送成功，int没有underflow的错误。
通过手动修改radio_config_si4463.h中的RF_PKT_LEN_12中的PKT_TX_THRESHOLD字段为TX_THRESHOLD，解决了ovderflow的问题
	实测当我在外面的函数设置TX_THRESHOLD实际没有起作用。
	进一步测试，当速度增加到500k时，依然出现overload情况，可用fifo为0x34，而TH_THRESHOLD为0x40

发送可变长包的长度时，2字节的情况下大小端的顺序是怎样的？
出现错误：

可变长包测试情况：
	发包时，若一个包长小于缓冲区长度，也会产生2次中断；
	数据包第一个字节需要是可变长field的长度（不包含自己），接收方会根据设置确定fifo中是否保留长度字节
	先是出现了CRC校验出错的问题，把crc配置全部停掉就好了
缓冲机制需要修改(只改了si4463）：
	struct cmd中重新加入len字段（skbdata在tx中一次性拷入队列中，这样在发送时就不需要再写入一次长度字段，省了一次spi调用的时间【确认数据包结构？】）
	出现了“skb_over_panic”并死机的问题【通过修改上述条目解决】
	
大量发包时卡死：【猜测是rbuf_print_status被中断，在isHan..的读取中spin_lock出错】【已确认是这个问题。解决】
BUG: spinlock recursion on CPU#1, cmd_queue_handl/287
0,785,81670448,-; lock: cmd_ringbuffer+0x10/0xffffdac0 [rf], .magic: dead4ead, .owner: cmd_queue_handl/287, .owner_cpu: 1
4,786,81670538,-;CPU: 1 PID: 287 Comm: cmd_queue_handl Tainted: G           O 3.10.17-yocto-standard #1
4,787,81670632,-;Hardware name: Intel Corporation Merrifield/BODEGA BAY, BIOS 466 2014.06.23:19.20.05
4,788,81670703,-; f5635370 f5635370 f6f8fe58 c18911d4 f6f8fe84 c189126b c1ac4448 f845c550
4,789,81670818,-; dead4ead f5635670 0000011f 00000001 f845c550 c1ac38b3 00000082 f6f8fe94
4,790,81670929,-; c189128e f845c550 f6f8fef8 f6f8feb0 c14bc3fa 00000412 00000400 f845c550
4,791,81671040,-;Call Trace:
4,792,81671092,-; [<c18911d4>] dump_stack+0x16/0x18
4,793,81671145,-; [<c189126b>] spin_dump+0x95/0x9d
4,794,81671199,-; [<c189128e>] spin_bug+0x1b/0x1f
4,795,81671254,-; [<c14bc3fa>] do_raw_spin_lock+0x10a/0x120
4,796,81671315,-; [<c189640c>] _raw_spin_lock+0x1c/0x20
4,797,81671382,-; [<f8458604>] rbuf_enqueue+0x24/0x100 [rf]
4,798,81671448,-; [<f8455bfd>] si4463_tx+0x5d/0xa0 [rf]
4,799,81671507,-; [<c170e966>] dev_hard_start_xmit+0x2b6/0x530
4,800,81671568,-; [<c1727dca>] sch_direct_xmit+0x9a/0x190
4,801,81671625,-; [<c1206d7b>] ? native_sched_clock+0x2b/0xd0
4,802,81671683,-; [<c1727f34>] __qdisc_run+0x74/0x110
4,803,81671739,-; [<c170c244>] net_tx_action+0xf4/0x1d0
4,804,81671796,-; [<c1246f39>] __do_softirq+0xd9/0x240
4,805,81671853,-; [<c1246e60>] ? tasklet_hi_action+0x120/0x120
4,806,81671900,-; <IRQ>  [<c1247225>] ? irq_exit+0xa5/0xb0
4,807,81671979,-; [<c189c195>] ? do_IRQ+0x45/0xb0
4,808,81672033,-; [<c189c071>] ? common_interrupt+0x31/0x38
4,809,81672091,-; [<c1240d7f>] ? vprintk_emit+0x1ff/0x5e0
4,810,81672151,-; [<c188cc09>] ? printk+0x3d/0x3f
4,811,81672213,-; [<f845899b>] ? rbuf_print_status+0x2b/0x40 [rf]
4,812,81672282,-; [<f8456eca>] ? si4463_cmd_queue_handler+0x28a/0x300 [rf]


si4463的双缓冲区可以合并为一个缓冲区“GLOBAL_CONFIG:FIFO_MODE”
	RF_GLOBAL_CONFIG_1:0x60==>0x70
	合并后，读fifo状态可以显示tx buffer为0x81=129

尝试修改出长包：
	PKT_CONFIG1：0x02==>0x82
	

优化si4463中的spi读写方式


=================================================================================
在ndo_start_xmit方法中加入ringbuffer full 的判断
	netq_restart_timer
在cmd_queue中加入rfbuf almost empty的判断

速度加快后出现不正常的丢包：
	有可能是cmd处理队列中只有一个缓冲区，被覆盖了。【否决】
在中断处理线程中尝试关闭抢占【否决】

https://communities.intel.com/thread/75511
https://communities.intel.com/thread/78149?start=15&tstart=0
解决：https://communities.intel.com/message/314820#314820
优化了读写帧的操作
出现了“***SPI TIMEOUT ERROR***”的错误，用dmesg看到内核报错：“;list_add corruption. prev->next should be next (f666ab64), but was   (null). (prev=f5fe7ef0)."错误
	这个错误出现在spi_queued_transfer ==> __list_add中。
	【用arduino测试没有发现问题，是否speed的锅？】


尝试在优化发送流程（减少memcpy）
	修改cmd结构。
convert_phyTransmitPower_to_reg_value函数临时修改为写死的值

读取后状态是否需要手动重置？：不需要。


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
