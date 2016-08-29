# flash sdcard
flash_sdcard.sh is used to flash image to sdcard.
But you should check which /dev/sdX sdcard creat by "sudo fdisk -l".

example:
    ./flash_sdcard.sh uboot
	./flash_sdcard.sh kernel
	./flash_sdcard.sh rootfs

Excute "flash_sdcard uboot" to flash spl.bin and uboot.bin to sdcard,
       Then BL0 can copy the spl-bin in sdcard to RAM when chioce "SDCARD BOOT" mode for tiny210.
Excute "flash_sdcard.sh kernel" to flash uImage and tiny210.dtb to sdcard.
Excute "flash_sdcard.sh rootfs" to flash ramdisk.img to sdcard.

IN SDCARD, those bin use unrelated area like that:
					section_range		total_sections		total_size
uboot-spl.bin		1-48				48					24KB
uboot.bin			49-1072				1024				512KB
uImage				1073-5168			4096				2MB
dtb					5169-5296			128					64KB
ramdisk.img			5297-				8192				4MB
