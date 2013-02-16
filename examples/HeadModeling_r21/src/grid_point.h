#ifndef __GRID_POINT__
#define __GRID_POINT__

typedef struct {
    float PP;
    float P[3];
    float R[3];
    
    float source;
    float * sigmap;
} GridPoint;

#endif
