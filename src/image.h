/****************************************************************************
 * Functions to load images.
 * Copyright (C) 2005 Joe Wingbermuehle
 ****************************************************************************/

#ifndef IMAGE_H
#define IMAGE_H

/****************************************************************************
 ****************************************************************************/
typedef struct ImageNode {

#ifdef USE_PNG
	png_uint_32 width;
	png_uint_32 height;
#else
	int width;
	int height;
#endif

	unsigned long *data;

} ImageNode;

/** Load an image from a file.
 * @param fileName The file containing the image.
 * @return A new image node (NULL if the image could not be loaded).
 */
ImageNode *LoadImage(const char *fileName);

/** Load an image from data.
 * The data must be in the format from the EWMH spec.
 * @param data The image data.
 * @return A new image node (NULL if there were errors.
 */
ImageNode *LoadImageFromData(char **data);

/** Destroy an image node.
 * @param image The image to destroy.
 */
void DestroyImage(ImageNode *image);

#endif

