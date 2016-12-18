# flash sdcard
flash_sdcard.sh is used to flash spl.bin and uboot.bin to sdcard.
But you should check which /dev/sdX sdcard creat by "sudo fdisk -l".

example:
    ./flash_sdcard.sh uboot  
			==========>it will flash uboot-spl and uboot to sdcard
						Then BL0 can copy the spl-bin in sdcard to RAM when chioce "SDCARD BOOT" mode for tiny210.
    ./flash_sdcard.sh fit_image   
			===========>it will flash fit_image to sdcard

