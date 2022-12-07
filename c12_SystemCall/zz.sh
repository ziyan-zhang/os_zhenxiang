sudo qemu-system-i386 \
	-m 1024 -s -S\
	-serial mon:stdio\
	-hda hd3M.img
#	-drive file=/opt/bochs/hd80M.img,index=1,media=disk,format=raw,if=ide
