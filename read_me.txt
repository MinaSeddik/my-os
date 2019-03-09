http://wiki.osdev.org/Bare_Bones

http://wiki.osdev.org/CMOS

http://wiki.osdev.org/RTC

http://wiki.osdev.org/Detecting_Memory_(x86)

http://forum.osdev.org/viewtopic.php?f=1&t=9721

1) read minix 3 parts 6. 3,4,5 and document important header and data structure
2) implimient RTL clock sleep -> ex ---> sleep(1000);	sleep(milli_sec);
3) handle control keys in keyboard device driver
4) read the above URLs
5) read about LDT

http://www.brokenthorn.com/Resources/OSDevIndex.html
http://wiki.osdev.org/CPUID
http://code.google.com/p/solleros/source/browse/trunk/source/drivers/network/rtl8169.asm?r=284
http://brainbox.cc/repository/trunk/sixty-four/multiboot/include/
http://www.adayinthelifeof.nl/2011/10/11/creating-partitioned-virtual-disk-images/
http://forum.osdev.org/viewtopic.php?f=1&t=15975
http://www.ibm.com/developerworks/linux/library/l-bochs/index.html?ca=drs-
http://forum.osdev.org/viewtopic.php?f=1&t=16705
http://micron-sysv3.googlecode.com/svn-history/r200/trunk/sysv3/kernel/arch/i386/cpu/irq.c
http://wiki.osdev.org/Loopback_Device#Hard_Disk_Images
objdump -D kernel.bin > kernel_assembly.txt
http://www.linux-usb.org/gadget/file_storage.html
sudo dd if=/dev/zero of=./harddisk.img bs=1M count=2048
http://superuser.com/questions/367196/linux-how-to-format-multiple-file-systems-within-one-file

losetup -d /dev/loop0
losetup -o65536 /dev/loop0 ./hard.img.vhd 
mount -o rw /dev/loop0 ./mount_point/

losetup -o65536 /dev/loop3 ./hard.img.vhd
mkfs -t ext2 /dev/loop3
then, format it mkfs ext2
then, use fdisk utility with -t option to change the partition type


sudo /sbin/lspci | grep Ethernet
OS : RTL8139 is supported
company : 02:00.0 Ethernet controller: Realtek Semiconductor Co., Ltd. RTL8111/8168B PCI Express Gigabit Ethernet controller (rev 02)
ne2000 there is a tool
http://en.wikibooks.org/wiki/QEMU/Networking

http://serverfault.com/questions/365409/create-and-bridge-virtual-network-interfaces-in-linux
http://brezular.wordpress.com/2011/06/19/bridging-qemu-image-to-the-real-network-using-tap-interface/


qemu -m 512 -hda hard.img.vhd -cdrom bootable.iso -boot d -net nic,vlan=1,macaddr=00:aa:00:60:00:01,model=e1000 -net tap,vlan=1,ifname=tap0,script=no
qemu -m 512 -hda hard.img.vhd -cdrom bootable.iso -boot d -net tap,vlan=1,ifname=tap0,script=no
Warning: vlan 1 with no nics
qemu -m 512 -hda hard.img.vhd -cdrom bootable.iso -boot d -net nic,vlan=1,macaddr=00:1D:09:AE:03:EB,model=e1000 -net tap,vlan=1,ifname=tap0,script=no
qemu -m 512 -hda hard.img.vhd -cdrom bootable.iso -boot d -net nic,vlan=1,macaddr=00:1D:09:AE:03:EB,model=rtl8139 -net tap,vlan=1,ifname=tap0,script=no


model=rtl8139 -net user
-netdev user,id=usernet -device rtl8139,netdev=usernet

http://diosix.googlecode.com/svn-history/r284/trunk/user/sbin/drivers/i386/rtl8139/main.c
http://www.h7.dion.ne.jp/~qemu-win/HowToNetwork-en.html
http://www.pcidatabase.com/
http://wiki.qemu.org/Documentation/Networking#Virtual_Network_Device
http://www.jbox.dk/sanos/source/sys/dev/rtl8139.c.html
http://minirighi.sourceforge.net/html/group__RTL8139Driver.html#a6


http://minirighi.sourceforge.net/html/group__FSext2.html




ifconfig eth0 192.168.0.99         # set bochs IP address
  dlx:~# route add -net 192.168.0.0         # first 3 numbers match IP
  dlx:~# route add default gw 192.168.0.1   # your gateway to the net
  dlx:~# _



