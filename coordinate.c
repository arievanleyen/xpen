/* file : coordinate.c */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include "coordinate.h"

CoordinateList *cl = NULL;
CoordinateList *cl_end = NULL;
int num = 0;

void record_coordinate(int x, int y)
{
	CoordinateList *pcl;

	pcl = (CoordinateList *) g_malloc0(sizeof(CoordinateList));
	pcl->x = x;
	pcl->y = y;
	pcl->next = NULL;
	if (!cl) {
		cl = cl_end = pcl;
		num += 2;
	} else {
		cl_end->next = pcl;
		cl_end = pcl;
		num += 2;
	}
}

void free_coordinates()
{
	CoordinateList *pcl = cl;

	while (pcl) {
		pcl = pcl->next;
		g_free(cl);
		cl = pcl;
	}
	cl = cl_end = NULL;
	num=0;
}
