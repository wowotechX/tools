# flash sdcard
flash_sdcard.sh is used to flash spl.bin and uboot.bin to sdcard.
But you should check which /dev/sdX sdcard creat by "sudo fdisk -l".

example:
    ./flash_sdcard.sh tiny210

Excute "flash_sdcard tiny210" to flash bin in sdcard for tiny210,
       Then BL0 can copy the spl-bin in sdcard to RAM when chioce "SDCARD BOOT" mode for tiny210.
