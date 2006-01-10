/****************************************************************************
 * Client placement functions.
 * Copyright (C) 2005 Joe Wingbermuehle
 ****************************************************************************/

#include "jwm.h"
#include "place.h"
#include "client.h"
#include "screen.h"
#include "border.h"
#include "tray.h"
#include "main.h"

typedef struct BoundingBox {
	int x, y;
	int width, height;
} BoundingBox;

typedef struct Strut {
	ClientNode *client;
	BoundingBox box;
	struct Strut *prev;
	struct Strut *next;
} Strut;

static Strut *struts = NULL;
static Strut *strutsTail = NULL;

/* desktopCount x screenCount */
/* Note that we assume x and y are 0 based for all screens here. */
static int *cascadeOffsets = NULL;

static void GetScreenBounds(int index, BoundingBox *box);
static void UpdateTrayBounds(BoundingBox *box, unsigned int layer);
static void UpdateStrutBounds(BoundingBox *box);
static void SubtractBounds(const BoundingBox *src, BoundingBox *dest);

/****************************************************************************
 ****************************************************************************/
void InitializePlacement() {
}

/****************************************************************************
 ****************************************************************************/
void StartupPlacement() {

	int count;
	int x;

	count = desktopCount * GetScreenCount();
	cascadeOffsets = Allocate(count * sizeof(int));

	for(x = 0; x < count; x++) {
		cascadeOffsets[x] = borderWidth + titleHeight;
	}

}

/****************************************************************************
 ****************************************************************************/
void ShutdownPlacement() {

	Strut *sp;

	Release(cascadeOffsets);

	while(struts) {
		sp = struts->next;
		Release(struts);
		struts = sp;
	}
	strutsTail = NULL;

}

/****************************************************************************
 ****************************************************************************/
void DestroyPlacement() {
}

/****************************************************************************
 ****************************************************************************/
void RemoveClientStrut(ClientNode *np) {

	Strut *sp;

	for(sp = struts; sp; sp = sp->next) {
		if(sp->client == np) {
			if(sp->prev) {
				sp->prev->next = sp->next;
			} else {
				struts = sp->next;
			}
			if(sp->next) {
				sp->next->prev = sp->prev;
			} else {
				strutsTail = sp->prev;
			}
			Release(sp);
		}
	}

}

/****************************************************************************
 ****************************************************************************/
void ReadClientStrut(ClientNode *np) {

	BoundingBox box;
	Strut *sp;
	int status;
	Atom actualType;
	int actualFormat;
	unsigned long count;
	unsigned long bytesLeft;
	unsigned char *value;
	long *lvalue;
	long leftWidth, rightWidth, topHeight, bottomHeight;
	long leftStart, leftEnd, rightStart, rightEnd;
	long topStart, topEnd, bottomStart, bottomEnd;

	RemoveClientStrut(np);

	box.x = 0;
	box.y = 0;
	box.width = 0;
	box.height = 0;

	/* First try to read _NET_WM_STRUT_PARTIAL */
	/* Format is:
	 *   left_width, right_width, top_width, bottom_width,
	 *   left_start_y, left_end_y, right_start_y, right_end_y,
	 *   top_start_x, top_end_x, bottom_start_x, bottom_end_x
	 */
	status = JXGetWindowProperty(display, np->window,
		atoms[ATOM_NET_WM_STRUT_PARTIAL], 0, 12, False, XA_CARDINAL,
		&actualType, &actualFormat, &count, &bytesLeft, &value);
	if(status == Success) {
		if(count == 12) {
			lvalue = (long*)value;
			leftWidth = lvalue[0];
			rightWidth = lvalue[1];
			topHeight = lvalue[2];
			bottomHeight = lvalue[3];
			leftStart = lvalue[4];
			leftEnd = lvalue[5];
			rightStart = lvalue[6];
			rightEnd = lvalue[7];
			topStart = lvalue[8];
			topEnd = lvalue[9];
			bottomStart = lvalue[10];
			bottomEnd = lvalue[11];

			if(leftWidth > 0) {
				box.width = leftWidth;
				box.x = leftStart;
			}

			if(rightWidth > 0) {
				box.width = rightWidth;
				box.x = rightStart;
			}

			if(topHeight > 0) {
				box.height = topHeight;
				box.y = topStart;
			}

			if(bottomHeight > 0) {
				box.height = bottomHeight;
				box.y = bottomStart;
			}

			sp = Allocate(sizeof(Strut));
			sp->client = np;
			sp->box = box;
			sp->prev = NULL;
			sp->next = struts;
			if(struts) {
				struts->prev = sp;
			} else {
				strutsTail = sp;
			}
			struts = sp;

		}
		JXFree(value);
		return;
	}

	/* Next try to read _NET_WM_STRUT */
	/* Format is: left_width, right_width, top_width, bottom_width */
	status = JXGetWindowProperty(display, np->window,
		atoms[ATOM_NET_WM_STRUT], 0, 4, False, XA_CARDINAL,
		&actualType, &actualFormat, &count, &bytesLeft, &value);
	if(status == Success) {
		if(count == 4) {
			lvalue = (long*)value;
			leftWidth = lvalue[0];
			rightWidth = lvalue[1];
			topHeight = lvalue[2];
			bottomHeight = lvalue[3];

			if(leftWidth > 0) {
				box.x = 0;
				box.width = leftWidth;
			}

			if(rightWidth > 0) {
				box.x = rootWidth - rightWidth;
				box.width = rightWidth;
			}

			if(topHeight > 0) {
				box.y = 0;
				box.height = topHeight;
			}

			if(bottomHeight > 0) {
				box.y = rootHeight - bottomHeight;
				box.height = bottomHeight;
			}

			sp = Allocate(sizeof(Strut));
			sp->client = np;
			sp->box = box;
			sp->prev = NULL;
			sp->next = struts;
			if(struts) {
				struts->prev = sp;
			} else {
				strutsTail = sp;
			}
			struts = sp;

		}
		JXFree(value);
		return;
	}

}

/****************************************************************************
 ****************************************************************************/
void GetScreenBounds(int index, BoundingBox *box) {

	box->x = GetScreenX(index);
	box->y = GetScreenY(index);
	box->width = GetScreenWidth(index);
	box->height = GetScreenHeight(index);

}

/****************************************************************************
 * Shrink dest such that it does not intersect with src.
 ****************************************************************************/
void SubtractBounds(const BoundingBox *src, BoundingBox *dest) {

	BoundingBox boxes[4];

	if(src->x + src->width <= dest->x) {
		return;
	}
	if(src->y + src->height <= dest->y) {
		return;
	}
	if(dest->x + dest->width <= src->x) {
		return;
	}
	if(dest->y + dest->height <= src->y) {
		return;
	}

	/* There are four ways to do this:
	 *  1. Increase the x-coordinate and decrease the width of dest.
	 *  2. Increase the y-coordinate and decrease the height of dest.
	 *  3. Decrease the width of dest.
	 *  4. Decrease the height of dest.
	 * We will chose the option which leaves the greatest area.
	 * Note that negative areas are possible.
	 */

	/* 1 */
	boxes[0] = *dest;
	boxes[0].x = src->x + src->width;
	boxes[0].width = dest->x + dest->width - boxes[0].x;

	/* 2 */
	boxes[1] = *dest;
	boxes[1].y = src->y + src->height;
	boxes[1].height = dest->y + dest->height - boxes[1].y;

	/* 3 */
	boxes[2] = *dest;
	boxes[2].width = src->x - dest->x;

	/* 4 */
	boxes[3] = *dest;
	boxes[3].height = src->y - dest->y;

	/* 1 and 2, winner in 1. */
	if(boxes[0].width * boxes[0].height < boxes[1].width * boxes[1].height) {
		boxes[0] = boxes[1];
	}

	/* 3 and 4, winner in 3. */
	if(boxes[2].width * boxes[2].height < boxes[3].width * boxes[3].height) {
		boxes[2] = boxes[3];
	}

	/* 1 and 3, winner in dest. */
	if(boxes[0].width * boxes[0].height < boxes[2].width * boxes[2].height) {
		*dest = boxes[2];
	} else {
		*dest = boxes[0];
	}

}

/****************************************************************************
 ****************************************************************************/
void UpdateTrayBounds(BoundingBox *box, unsigned int layer) {

	TrayType *tp;
	BoundingBox src;
	BoundingBox last;

	for(tp = GetTrays(); tp; tp = tp->next) {

		if(tp->layer > layer) {

			src.x = tp->x;
			src.y = tp->y;
			src.width = tp->width;
			src.height = tp->height;

			last = *box;
			SubtractBounds(&src, box);
			if(box->width * box->height <= 0) {
				*box = last;
				break;
			}

		}

	}

}

/****************************************************************************
 ****************************************************************************/
void UpdateStrutBounds(BoundingBox *box) {

	Strut *sp;
	BoundingBox last;

	for(sp = struts; sp; sp = sp->next) {
		if(sp->client->state.desktop == currentDesktop
			|| (sp->client->state.status & STAT_STICKY)) {
			continue;
		}
		last = *box;
		SubtractBounds(&sp->box, box);
		if(box->width * box->height <= 0) {
			*box = last;
			break;
		}
	}

}

/****************************************************************************
 ****************************************************************************/
void PlaceClient(ClientNode *np, int alreadyMapped) {

	BoundingBox box;
	int north, west;
	int screenIndex;
	int cascadeMultiplier;
	int cascadeIndex;
	int overflow;

	Assert(np);

	north = 0;
	west = 0;
	if(np->state.border & BORDER_OUTLINE) {
		north = borderWidth;
		west = borderWidth;
	}
	if(np->state.border & BORDER_TITLE) {
		north += titleHeight;
	}

	screenIndex = GetMouseScreen();

	GetScreenBounds(screenIndex, &box);

	if(alreadyMapped || (np->sizeFlags & (PPosition | USPosition))) {

		if(np->x + np->width - box.x > box.width) {
			np->x = box.x;
		}
		if(np->y + np->height - box.y > box.height) {
			np->y = box.y;
		}

		GravitateClient(np, 0);

	} else {

		UpdateTrayBounds(&box, np->state.layer);
		UpdateStrutBounds(&box);

		cascadeMultiplier = GetScreenCount() * desktopCount;
		cascadeIndex = screenIndex * cascadeMultiplier + currentDesktop;

		/* Set the cascaded location. */
		np->x = box.x + west + cascadeOffsets[cascadeIndex];
		np->y = box.y + north + cascadeOffsets[cascadeIndex];
		cascadeOffsets[cascadeIndex] += borderWidth + titleHeight;

		/* Check for cascade overflow. */
		overflow = 0;
		if(np->x + np->width - box.x > box.width) {
			overflow = 1;
		} else if(np->y + np->height - box.y > box.height) {
			overflow = 1;
		}

		if(overflow) {

			cascadeOffsets[cascadeIndex] = borderWidth + titleHeight;
			np->x = box.x + west + cascadeOffsets[cascadeIndex];
			np->y = box.y + north + cascadeOffsets[cascadeIndex];

			/* Check for client overflow. */
			overflow = 0;
			if(np->x + np->width - box.x > box.width) {
				overflow = 1;
			} else if(np->y + np->height - box.y > box.height) {
				overflow = 1;
			}

			/* Update cascade position or position client. */
			if(overflow) {
				np->x = box.x + west;
				np->y = box.y + north;
			} else {
				cascadeOffsets[cascadeIndex] += borderWidth + titleHeight;
			}

		}

	}

	JXMoveWindow(display, np->parent, np->x - west, np->y - north);

}

/****************************************************************************
 ****************************************************************************/
void PlaceMaximizedClient(ClientNode *np) {

	BoundingBox box;
	int screenIndex;
	int north, west;

	np->oldx = np->x;
	np->oldy = np->y;
	np->oldWidth = np->width;
	np->oldHeight = np->height;

	north = 0;
	west = 0;
	if(np->state.border & BORDER_OUTLINE) {
		north = borderWidth;
		west = borderWidth;
	}
	if(np->state.border & BORDER_TITLE) {
		north += titleHeight;
	}

	screenIndex = GetCurrentScreen(np->x, np->y);
	GetScreenBounds(screenIndex, &box);
	UpdateTrayBounds(&box, np->state.layer);
	UpdateStrutBounds(&box);

	box.x += west;
	box.y += north;
	box.width -= west + west;
	box.height -= north + west;

	if(box.width > np->maxWidth) {
		box.width = np->maxWidth;
	}
	if(box.height > np->maxHeight) {
		box.height = np->maxHeight;
	}

	if(np->sizeFlags & PAspect) {
		if((float)box.width / box.height
			< (float)np->aspect.minx / np->aspect.miny) {
			box.height = box.width * np->aspect.miny / np->aspect.minx;
		}
		if((float)box.width / box.height
			> (float)np->aspect.maxx / np->aspect.maxy) {
			box.width = box.height * np->aspect.maxx / np->aspect.maxy;
		}
	}

	np->x = box.x;
	np->y = box.y;
	np->width = box.width - (box.width % np->xinc);
	np->height = box.height - (box.height % np->yinc);

	np->state.status |= STAT_MAXIMIZED;

}

/****************************************************************************
 ****************************************************************************/
void GetBorderSize(const ClientNode *np,
	int *north, int *south, int *east, int *west) {

	Assert(np);
	Assert(north);
	Assert(south);
	Assert(east);
	Assert(west);

	*north = 0;
	*south = 0;
	*east = 0;
	*west = 0;
	if(np->state.border & BORDER_OUTLINE) {
		*north = borderWidth;
		*south = borderWidth;
		*east = borderWidth;
		*west = borderWidth;
	}
	if(np->state.border & BORDER_TITLE) {
		*north += titleHeight;
	}

}

/****************************************************************************
 ****************************************************************************/
void GetGravityDelta(int gravity, int north, int south, int east, int west,
	int *x, int  *y) {

	Assert(x);
	Assert(y);

	*y = 0;
	*x = 0;
	switch(gravity) {
	case NorthWestGravity:
		*y = -north;
		*x = -west;
		break;
	case NorthGravity:
		*y = -north;
		break;
	case NorthEastGravity:
		*y = -north;
		*x = west;
		break;
	case WestGravity:
		*x = -west;
		break;
	case CenterGravity:
		*y = (north + south) / 2;
		*x = (east + west) / 2;
		break;
	case EastGravity:
		*x = west;
		break;
	case SouthWestGravity:
		*y = south;
		*x = -west;
		break;
	case SouthGravity:
		*y = south;
		break;
	case SouthEastGravity:
		*y = south;
		*x = west;
		break;
	default: /* Static */
		break;
	}

}

/****************************************************************************
 * Move the window in the specified direction for reparenting.
 ****************************************************************************/
void GravitateClient(ClientNode *np, int negate) {

	int north, south, east, west;
	int deltax, deltay;

	Assert(np);

	GetBorderSize(np, &north, &south, &east, &west);
	GetGravityDelta(np->gravity, north, south, east, west, &deltax, &deltay);

	if(negate) {
		np->x += deltax;
		np->y += deltay;
	} else {
		np->x -= deltax;
		np->y -= deltay;
	}

}
