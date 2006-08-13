/****************************************************************************
 * Header for client window resize functions.
 * Copyright (C) 2004 Joe Wingbermuehle
 ****************************************************************************/

#ifndef RESIZE_H
#define RESIZE_H

#include "border.h"

struct ClientNode;

typedef enum {
	RESIZE_OPAQUE,   /**< Show window contents while resizing. */
	RESIZE_OUTLINE   /**< Show an outline while resizing. */
} ResizeModeType;

/** Resize a client window.
 * @param np The client to resize.
 * @param action The location on the border where the move should take place.
 * @param startx The starting mouse x-coordinate (window relative).
 * @param starty The starting mouse y-coordinate (window relative).
 */
void ResizeClient(struct ClientNode *np, BorderActionType action,
	int startx, int starty);

/** Resize a client window using the keyboard (mouse optional).
 * @param np The client to resize.
 */
void ResizeClientKeyboard(struct ClientNode *np);

/** Set the resize mode to use.
 * @param mode The resize mode to use.
 */
void SetResizeMode(ResizeModeType mode);

#endif

