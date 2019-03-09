
CC	 = gcc
CFLAGS	 = -Wall -O -fstrength-reduce -fomit-frame-pointer -finline-functions -nostdinc -fno-builtin -nostartfiles -nodefaultlibs -I./include -m32 -c 

LDSCRIPT = ./link.ld
LDFLAGS	 = -melf_i386 -T $(LDSCRIPT)
LD	 = ld
 
all : rm_kernal kernel.bin clean copy_kenral generate_iso_bootable_cd copy_iso
 

rm_kernal : 
	rm -f kernel.bin

start.o : k386.asm
	nasm -f elf -o start.o k386.asm
 
kernel.o : main.c
	$(CC) $(CFLAGS) -o kernel.o main.c

string.o : string.c
	$(CC) $(CFLAGS) -o string.o string.c

bios.o : bios.c
	$(CC) $(CFLAGS) -o bios.o bios.c

system.o : system.c
	$(CC) $(CFLAGS) -o system.o system.c
 
gdt.o : gdt.c
	$(CC) $(CFLAGS) -o gdt.o gdt.c

idt.o : idt.c
	$(CC) $(CFLAGS) -o idt.o idt.c

stdlib.o : stdlib.c
	$(CC) $(CFLAGS) -o stdlib.o stdlib.c

stdio.o : stdio.c
	$(CC) $(CFLAGS) -o stdio.o stdio.c

math.o : math.c
	$(CC) $(CFLAGS) -o math.o math.c

datetime.o : datetime.c
	$(CC) $(CFLAGS) -o datetime.o datetime.c

vga.o : vga.c
	$(CC) $(CFLAGS) -o vga.o vga.c

clock.o : clock.c
	$(CC) $(CFLAGS) -o clock.o clock.c

keyboard.o : keyboard.c
	$(CC) $(CFLAGS) -o keyboard.o keyboard.c

mutex.o : mutex.c
	$(CC) $(CFLAGS) -o mutex.o mutex.c

memory.o : memory.c
	$(CC) $(CFLAGS) -o memory.o memory.c

scheduler.o : scheduler.c
	$(CC) $(CFLAGS) -o scheduler.o scheduler.c

thread.o : thread.c
	$(CC) $(CFLAGS) -o thread.o thread.c

ata.o : ata.c
	$(CC) $(CFLAGS) -o ata.o ata.c

rtl8139.o : rtl8139.c
	$(CC) $(CFLAGS) -o rtl8139.o rtl8139.c

ne2000.o : ne2000.c
	$(CC) $(CFLAGS) -o ne2000.o ne2000.c

filesystem.o : filesystem.c
	$(CC) $(CFLAGS) -o filesystem.o filesystem.c

fat32.o : fat32.c
	$(CC) $(CFLAGS) -o fat32.o fat32.c

ext2.o : ext2.c
	$(CC) $(CFLAGS) -o ext2.o ext2.c

iso9660.o : iso9660.c
	$(CC) $(CFLAGS) -o iso9660.o iso9660.c

pci.o : pci.c
	$(CC) $(CFLAGS) -o pci.o pci.c

debug.o : debug.c
	$(CC) $(CFLAGS) -o debug.o debug.c

shell.o : shell.c
	$(CC) $(CFLAGS) -o shell.o shell.c

blockqueue.o : blockqueue.c
	$(CC) $(CFLAGS) -o blockqueue.o blockqueue.c

inet.o : inet.c
	$(CC) $(CFLAGS) -o inet.o inet.c

netif.o : netif.c
	$(CC) $(CFLAGS) -o netif.o netif.c

ether.o : ether.c
	$(CC) $(CFLAGS) -o ether.o ether.c

arp.o : arp.c
	$(CC) $(CFLAGS) -o arp.o arp.c

ip.o : ip.c
	$(CC) $(CFLAGS) -o ip.o ip.c

icmp.o : icmp.c
	$(CC) $(CFLAGS) -o icmp.o icmp.c

udp.o : udp.c
	$(CC) $(CFLAGS) -o udp.o udp.c

tcp.o : tcp.c
	$(CC) $(CFLAGS) -o tcp.o tcp.c

tcp_input.o : tcp_input.c
	$(CC) $(CFLAGS) -o tcp_input.o tcp_input.c

tcp_output.o : tcp_output.c
	$(CC) $(CFLAGS) -o tcp_output.o tcp_output.c

stats.o : stats.c
	$(CC) $(CFLAGS) -o stats.o stats.c

mem.o : mem.c
	$(CC) $(CFLAGS) -o mem.o mem.c

pbuf.o : pbuf.c
	$(CC) $(CFLAGS) -o pbuf.o pbuf.c

net_sys.o : net_sys.c
	$(CC) $(CFLAGS) -o net_sys.o net_sys.c


date.o : util/date.c
	$(CC) $(CFLAGS) -o util/date.o util/date.c

reboot.o : util/reboot.c
	$(CC) $(CFLAGS) -o util/reboot.o util/reboot.c

cat.o : util/cat.c
	$(CC) $(CFLAGS) -o util/cat.o util/cat.c

touch.o : util/touch.c
	$(CC) $(CFLAGS) -o util/touch.o util/touch.c

mkdir.o : util/mkdir.c
	$(CC) $(CFLAGS) -o util/mkdir.o util/mkdir.c

rm.o : util/rm.c
	$(CC) $(CFLAGS) -o util/rm.o util/rm.c

cp.o : util/cp.c
	$(CC) $(CFLAGS) -o util/cp.o util/cp.c

kernel.bin : start.o kernel.o string.o bios.o system.o gdt.o idt.o stdlib.o stdio.o math.o datetime.o vga.o clock.o keyboard.o mutex.o memory.o scheduler.o thread.o ata.o rtl8139.o ne2000.o filesystem.o fat32.o ext2.o iso9660.o pci.o debug.o shell.o blockqueue.o inet.o netif.o ether.o arp.o ip.o icmp.o udp.o tcp.o tcp_input.o tcp_output.o stats.o mem.o pbuf.o net_sys.o util/date.o util/reboot.o util/cat.o util/touch.o util/mkdir.o util/rm.o util/cp.o
	$(LD) $(LDFLAGS) -o kernel.bin start.o kernel.o string.o bios.o system.o gdt.o idt.o stdlib.o stdio.o math.o datetime.o vga.o clock.o keyboard.o mutex.o memory.o scheduler.o thread.o ata.o rtl8139.o ne2000.o filesystem.o fat32.o ext2.o iso9660.o pci.o debug.o shell.o blockqueue.o inet.o netif.o ether.o arp.o ip.o icmp.o udp.o tcp.o tcp_input.o tcp_output.o stats.o mem.o pbuf.o net_sys.o util/date.o util/reboot.o util/cat.o util/touch.o util/mkdir.o util/rm.o util/cp.o



clean : 
	rm -f *.o util/*.o

copy_kenral : 
	cp kernel.bin isofiles/boot/kernal.bin
	objdump -D kernel.bin > kernel_assembly.txt

generate_iso_bootable_cd : 
	mkisofs -R -b boot/grub/stage2_eltorito -no-emul-boot -boot-load-size 4 -boot-info-table -o bootable.iso isofiles/ 

copy_iso : 
	rm -f /home/mina/Desktop/booting/bootable.iso
	cp bootable.iso /home/mina/Desktop/booting/bootable.iso





