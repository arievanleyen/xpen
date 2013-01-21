/* file : coordinate.h */
#ifndef _COORDINATE_H
#define _COORDINATE_H

typedef struct _CoordinateList {
	int x;
	int y;
	struct _CoordinateList *next;
} CoordinateList;

void record_coordinate(int, int);
void free_coordinates(void);

#endif /* _COORDINATE_H */
