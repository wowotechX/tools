/*
 * dfu_board.h
 *
 * Board abstraction for Device Firmware Upgrating.
 *
 * Copyright (C) 2016 wowotech
 * Subject to the GNU Public License, version 2.
 *
 * Author: wowo<www.wowotech.net>
 */
#ifndef __DFU_BOARD_H_
#define __DFU_BOARD_H_

struct dfu_board {
	const char board_name[50];

	int (*init)(struct dfu_board *board);
	int (*exit)(struct dfu_board *board);
	int (*upload)(struct dfu_board *board, const char *filename,
		      unsigned int addr);
	int (*run)(struct dfu_board *board, unsigned int addr);
};

int dfu_board_register(struct dfu_board *board);
int dfu_board_unregister(struct dfu_board *board);

struct dfu_board *dfu_board_get_by_name(const char *name);
#endif
