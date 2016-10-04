/*
 * adfu.c
 *
 * Device Firmware Upgrating for bubblegum.
 * Refer to https://github.com/96boards-bubblegum/linaro-adfu-tool/
 *
 * Copyright (C) 2016 wowotech
 * Subject to the GNU Public License, version 2.
 *
 * Author: wowo<www.wowotech.net>
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

#include <unistd.h>
#include <error.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <libusb.h>

#include "dfu_board.h"
#include "debug.h"

DEBUG_SET_LEVEL(DEBUG_LEVEL_INFO);

static libusb_context *libusb_ctx;
static libusb_device_handle *libusb_handler;

static int b96_init_usb(void)
{
	DEBUG("%s\n", __func__);

	libusb_init(&libusb_ctx);
	libusb_set_debug(libusb_ctx,3);

	return 0;
}

static int b96_uninit_usb(void)
{
	DEBUG("%s\n", __func__);

	libusb_exit(libusb_ctx);
	libusb_ctx = NULL;

	return 0;
}


/**
 * Open the device and init it
 * You should call b96_init_usb() first before using this function
 *
 * @return the handler of the device
 */
static libusb_device_handle *b96_init_device(void)
{
	int i;

	int r1;
	int c1;

	libusb_device_handle *handler = NULL;
	struct libusb_device_descriptor desc;

	DEBUG("%s\n", __func__);

	handler = libusb_open_device_with_vid_pid(libusb_ctx, 0x10d6, 0x10d6);
	if (handler == NULL) {
		ERR("Error: cannot open device 10d6:10d6\n");
		return handler;
	}

	r1 = libusb_reset_device(handler);
	if (r1 != 0)
		ERR("Error: cannot reset device\n");

	r1 = libusb_get_device_descriptor(libusb_get_device(handler), &desc);
	if (r1 != 0)
		ERR("Error: cannot get device descriptor\n");

	INFO("bDescriptorType: %d\n", desc.bDescriptorType);
	INFO("bNumConfigurations: %d\n", desc.bNumConfigurations);
	INFO("iManufacturer: %d\n", desc.iManufacturer);

	for (i = 1; i <= desc.bNumConfigurations; i++) {
		struct libusb_config_descriptor *config = NULL;
		int r2 = 0;
		int j;
		r2 = libusb_get_config_descriptor_by_value(
			libusb_get_device(handler), i, &config);
		if (r2 != 0) {
			ERR("Error: cannot get configuration %d descriptor\n", i);
			continue;
		}

		INFO("bNumInterfaces: %d\n", config->bNumInterfaces);
		for (j = 0; j < config->bNumInterfaces; j++) {
		}

		if (config != NULL) {
			libusb_free_config_descriptor(config);
			config = NULL;
		}
	}

	r1 = libusb_detach_kernel_driver(handler, 0);
	if (r1 != 0) {
		const char *errorStr = "unknown";
		if (r1 == LIBUSB_ERROR_NOT_FOUND)
			errorStr = "LIBUSB_ERROR_NOT_FOUND (no worry)";
		else if (r1 == LIBUSB_ERROR_INVALID_PARAM)
			errorStr = "LIBUSB_ERROR_INVALID_PARAM";
		else if (r1 == LIBUSB_ERROR_NO_DEVICE)
			errorStr = "LIBUSB_ERROR_NO_DEVICE";
		else if (r1 == LIBUSB_ERROR_NOT_SUPPORTED)
			errorStr = "LIBUSB_ERROR_NOT_SUPPORTED";

		ERR("Info: cannot detach kernel driver: %s\n", errorStr);
	}

	c1 = 0;
	r1 = libusb_get_configuration(handler, &c1);
	if (r1 != 0)
		ERR("Error: cannot get device configuration\n");

	INFO("Configuiration: %d\n", c1);
	r1 = libusb_set_configuration(handler, c1);
	if (r1 != 0)
		ERR("Error: cannot set device configuration\n");

	r1 = libusb_claim_interface(handler, 0);
	if (r1 != 0)
		ERR("Error: cannot claim device interface\n");

	return handler;
}

static void b96_uninit_device(libusb_device_handle *handler)
{
	int r1;

	DEBUG("%s\n", __func__);

	r1 = libusb_release_interface(handler, 0);
	if (r1 != 0)
		ERR("Error: cannot release device interface\n");

	libusb_close(handler);
}

void readCSW(libusb_device_handle *handler)
{
	unsigned char buf1[1024];
	int r1 = 0;
	int transferred = 0;
	int i;

	int state = 1;

	DEBUG("%s\n", __func__);

	while (state != 0) {
		switch(state) {
		case 1:
			r1 = libusb_bulk_transfer(handler, 0x82, buf1, 1024,
						  &transferred, 3000);
			if (r1 == 0) {
				INFO("CSW:");
				for (i = 0; i<transferred; i++)
					INFO("%02x ", buf1[i]);
				INFO("\n");
			}
			state = 2;
			break;

		case 2:
			if (r1 == 0)
				state = 3;
			else
				state = 5;
			break;

		case 3:
			if (transferred == 13 && buf1[0] == '\x55' &&
			    buf1[1]=='\x53' && buf1[2]=='\x42' && buf1[3]=='\x53')
				state = 4;
			else
				state = 8;
			break;

		case 4:
			if (transferred == 13 && buf1[12] == '\x02')
				state = 8;
			else
				state = 0;
			break;

		case 5:
			r1 = libusb_clear_halt(handler, 0x82);
			if (r1 != 0)
				ERR("Error: cannot clear halt for endpoints 0x82\n");

			state = 6;
			break;

		case 6:
			r1 = libusb_bulk_transfer(handler, 0x82, buf1, 1024,
						  &transferred, 3000);
			if (r1 == 0) {
				INFO("CSW:");
				for (i = 0; i < transferred; i++)
					INFO(" %02x", buf1[i]);
				INFO("\n");
			}
			state = 7;
			break;

		case 7:
			if (r1 == 0)
				state = 3;
			else
				state = 8;

			break;

		case 8:
			r1 = libusb_reset_device(handler);
			if (r1 != 0)
				ERR("Error: cannot reset device\n");

			state = 0;
			break;
		}
	}

	return;
}

static void writeBinaryFileSeek(libusb_device_handle *handler,
				unsigned char cmd,
				unsigned int sector,
				const char *filename,
				unsigned int seek,
				unsigned int len,
				unsigned int sector2,
				const unsigned char *flags)
{
	unsigned char data[1024];
	static unsigned char buf1[2 * 1024 * 1024];

	int transferred;
	int r1;
	int i;
	int tLen;

	FILE *file1 = NULL;

	DEBUG("%s\n", __func__);

	memset(data, 0, sizeof(data));

	file1 = fopen(filename, "rb");
	if (file1 == NULL) {
		ERR("Error: cannot read file %s\n", filename);
		return;
	}
	fseek(file1, seek, SEEK_SET);

	/* signature */
	data[0] = '\x55';
	data[1] = '\x53';
	data[2] = '\x42';
	data[3] = '\x43';

	/* tag */
	data[4] = '\0';
	data[5] = '\0';
	data[6] = '\0';
	data[7] = '\0';

	/* Data Transfer Length */
	data[8] = len % 256;
	data[9] = (len / 256) % 256;
	data[10] = (len / 256 / 256) % 256;
	data[11] = (len / 256 / 256 / 256) % 256;

	/* flags */
	data[12] = '\x00';

	/* lun */
	data[13] = '\x00';

	/* cdb length: 16 */
	data[14] = '\x10';

	/* cbwcb */

	/* cbwcb - scsi cmd */
	data[15] = cmd;

	/* sector ? */
	data[16] = sector % 256;
	data[17] = (sector / 256) % 256;
	data[18] = (sector / 256 / 256) % 256;
	data[19] = (sector / 256 / 256 / 256) % 256;

	/* length ? */
	data[20] = len % 256;
	data[21] = (len /256) % 256;
	data[22] = (len /256 / 256) % 256;
	data[23] = (len /256 / 256 / 256) % 256;

	data[24] = sector2 % 256;
	data[25] = (sector2 /256) % 256;
	data[26] = (sector2 /256 / 256) % 256;
	data[27] = (sector2 /256 / 256 / 256) % 256;

	if (flags != NULL)
		for (i = 28; i < 31; i++)
			data[i] = flags[i - 28];

	INFO("CBW:");
	for (i = 0; i < 31; i++)
		INFO(" %02x", data[i]);
	INFO("\n");

	transferred=0;
	r1 = libusb_bulk_transfer(handler, 0x01, data, 31, &transferred, 0);
	if (r1 != 0)
		ERR("Error: cannot send CBW for read\n");

	if (transferred != 31)
		ERR("Error: transferred %d != 31\n", transferred);
	sleep(1);

	for (tLen = len; tLen > 0 && !feof(file1); ) {
		int rLen = 0;
		if (tLen > sizeof(buf1))
			rLen = fread(buf1,1,sizeof(buf1),file1);
		else
			rLen = fread(buf1,1,tLen, file1);

		if (rLen <= 0)
			break;

		transferred = 0;
		r1 = libusb_bulk_transfer(handler, 0x01, buf1, rLen,
					  &transferred, 0);
		if (r1 != 0) {
			ERR("Error: cannot send data(%d)\n", r1);
			break;
		}

		if (transferred != rLen) {
			ERR("Error: transferred %d != rLen = %d\n",
			    transferred, rLen);
			break;
		}
		sleep(1);
		INFO("Bulk transferred %d bytes\n",rLen);
		tLen -= rLen;
	}

	fclose(file1);
	file1 = NULL;

	readCSW(handler);
}


static void writeBinaryFile(libusb_device_handle *handler, unsigned char cmd,
			    unsigned int sector, const char *filename,
			    unsigned int sector2, const unsigned char *flags)
{
	unsigned int len = 0;
	int r1;

	FILE *file1 = NULL;
	struct stat stat1;

	DEBUG("%s\n", __func__);

	memset(&stat1, 0, sizeof(stat1));

	file1 = fopen(filename,"rb");
	if (file1 == NULL) {
		ERR("Error: cannot read file %s\n", filename);
		return;
	}

	r1 = fstat(fileno(file1), &stat1);
	fclose(file1);

	file1 = NULL;
	if (r1 != 0) {
		ERR("Error: cannot read file stat %s", filename);
		return;
	}

	len = (unsigned int)stat1.st_size;
	writeBinaryFileSeek(handler, cmd, sector, filename, 0, len,
			    sector2, flags);
}

static void unknownCMD07(libusb_device_handle *handler, unsigned int addr)
{
	unsigned char data[1024];
	int transferred;
	int r1;
	int i;

	DEBUG("%s\n", __func__);

	memset(data, 0, sizeof(data));

	/* signature */
	data[0] = '\x55';
	data[1] = '\x53';
	data[2] = '\x42';
	data[3] = '\x43';

	/* tag */
	data[4] = '\0';
	data[5] = '\0';
	data[6] = '\0';
	data[7] = '\0';

	/* Data Transfer Length */
	data[8] = 0;
	data[9] = 0;
	data[10] = 0;
	data[11] = 0;

	/* flags */
	data[12] = '\x00';

	/* lun */
	data[13] = '\x00';

	/* cdb length: 00 */
	data[14] = '\x00';

	/* cbwcb */
	/* cbwcb - scsi cmd */
	data[15] = '\x10';

	/* sector ? */
	data[16] = addr % 256;
	data[17] = (addr / 256) % 256;
	data[18] = (addr / 256 / 256) % 256;
	data[19] = (addr / 256 / 256 / 256) % 256;

	/* length ? */
	data[20] = 0;
	data[21] = 0;
	data[22] = 0;
	data[23] = 0;

	INFO("CBW:");
	for (i = 0; i < 31; i++)
		INFO(" %02x", data[i]);
	INFO("\n");

	transferred = 0;
	r1 = libusb_bulk_transfer(handler, 0x01, data, 31, &transferred, 0);
	if (r1 != 0)
		ERR("Error: cannot send CBW for read\n");

	INFO("Transffered: %d\n", transferred);

	readCSW(handler);
}

static int bubblegum_init(struct dfu_board *board)
{
	DEBUG("%s\n", __func__);

	b96_init_usb();

	libusb_handler = b96_init_device();
	if (libusb_handler == NULL) {
		printf("b96_init_device failed\n");
		return -1;
	}

	INFO("Handler: %p\n", libusb_handler);
	return 0;
}

static int bubblegum_exit(struct dfu_board *board)
{
	DEBUG("%s\n", __func__);

	if (libusb_handler != NULL) {
		b96_uninit_device(libusb_handler);
		libusb_handler = NULL;
	}

	b96_uninit_usb();
	return 0;
}

static int bubblegum_upload(struct dfu_board *board, const char *filename,
			    unsigned int addr)
{
	DEBUG("%s, filename %s, addr 0x%x\n", __func__, filename, addr);

	writeBinaryFile(libusb_handler, '\x05', addr, filename, 0, NULL);
	return 0;
}

static int bubblegum_run(struct dfu_board *board, unsigned int addr)
{
	DEBUG("%s, addr 0x%x\n", __func__, addr);

	unknownCMD07(libusb_handler, addr);

	return 0;
}

static struct dfu_board board_bubblegum = {
	.board_name = "bubblegum",

	.init = bubblegum_init,
	.exit = bubblegum_exit,
	.upload = bubblegum_upload,
	.run = bubblegum_run,
};

__attribute__((constructor)) void board_bubblegum_init(void)
{
	DEBUG("%s\n", __func__);

	dfu_board_register(&board_bubblegum);
}
