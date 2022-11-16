nasm -I include/ -o mbr.bin mbr.S
nasm -I include/ -o loader.bin loader.S
sudo dd if=./mbr.bin of=/opt/bochs/hd60M.img bs=512 count=1 conv=notrunc
sudo dd if=./loader.bin of=/opt/bochs/hd60M.img bs=512 count=4 seek=2 conv=notrunc

