/*
 * dfu_board.c
 *
 * Board abstraction for Device Firmware Upgrating.
 *
 * Copyright (C) 2016 wowotech
 * Subject to the GNU Public License, version 2.
 *
 * Author: wowo<www.wowotech.net>
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "dfu_board.h"
#include "debug.h"

#define MAX_BOARD_NUM		(10)

DEBUG_SET_LEVEL(DEBUG_LEVEL_INFO);

static struct dfu_board		*g_board_array[MAX_BOARD_NUM];
static int			g_board_num;

int dfu_board_register(struct dfu_board *board)
{
	DEBUG("%s\n", __func__);

	if (board == NULL)
		return -1;

	if (g_board_num == MAX_BOARD_NUM) {
		printf("g_board_array is full!\n");
		return -1;
	}

	g_board_array[g_board_num] = board;
	g_board_num++;

	return 0;
}

int dfu_board_unregister(struct dfu_board *board)
{
	DEBUG("%s, do nothing now\n", __func__);
	return 0;
}

struct dfu_board *dfu_board_get_by_name(const char *name)
{
	int i;

	DEBUG("%s, name %s\n", __func__, name);

	for (i = 0; i < g_board_num; i++)
		if (strcmp(name, g_board_array[i]->board_name) == 0)
			return g_board_array[i];

	return NULL;
}

