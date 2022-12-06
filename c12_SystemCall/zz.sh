sudo qemu-system-i386 \
	-m 1024 -s -S\
	-serial mon:stdio\
	-hda /opt/bochs/hd3M.img

	#-hdb /opt/bochs/hd80M.img

#	-drive file=/opt/bochs/hd3M.img,index=0,media=disk,format=raw,if=ide
#	-drive file=/opt/bochs/hd80M.img,index=1,media=disk,format=raw,if=ide
