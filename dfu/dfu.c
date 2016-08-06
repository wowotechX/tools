/*
 * dfu.c
 *
 * Device Firmware Upgrating.
 *
 * Copyright (C) 2016 wowotech
 * Subject to the GNU Public License, version 2.
 *
 * Author: wowo<www.wowotech.net>
 */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "dfu_board.h"
#include "debug.h"

DEBUG_SET_LEVEL(DEBUG_LEVEL_INFO);

static void usage(int argc, char **argv) {
	INFO("Usage: %s board_name address [filename] [need_run]\n", argv[0]);
	INFO("     board_name: board's name, bubblegum etc.\n");
	INFO("     address: operation address, upload to or run to.\n");
	INFO("     filename: the file will be upload to 'address'. Optional.\n");
	INFO("     need_run: if run to 'address' after uploading, 1 yes, 0 no. Optional.\n\n");
	INFO("     NOTE:\n");
	INFO("     if no filename or need_run = 1, run to 'address'\n");
	INFO("     or, just upload 'filename' to 'address'\n");
}

int main(int argc, char **argv)
{
	const char *filename = NULL;

	unsigned int address;
	int need_run = 0;

	struct dfu_board *board;

	if (argc < 3) {
		usage(argc, argv);
		return -1;
	}

	sscanf(argv[2], "%x", &address);

	if (argc > 3)
		filename = argv[3];

	if (argc > 4)
		need_run = atoi(argv[4]);

	INFO("board %s\n", argv[1]);
	INFO("address 0x%x\n", address);
	INFO("filename %s\n", filename);
	INFO("need_run %d\n", need_run);

	board = dfu_board_get_by_name(argv[1]);
	if (board == NULL) {
		ERR("there is no '%s' board!\n", argv[1]);
		return -1;
	}

	if (board->init(board) < 0) {
		ERR("board->init failed!\n");
		return -1;
	}

	if (filename != NULL && board->upload != NULL)
		board->upload(board, filename, address);

	if ((filename == NULL || need_run == 1) && board->run != NULL) {
		sleep(1);
		board->run(board, address);
	}

	board->exit(board);

	return 0;
}
