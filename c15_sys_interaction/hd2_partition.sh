/opt/bochs/bin/bximage -hd -mode="flat" -size=80 -q hd80M.img
fdisk ./hd80M.img
# 以下好多连续输选项的，见《操作系统真象还原》569页，分完区可以重复格式化，没必要非写成脚本了