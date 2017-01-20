/*********************************************************************
 * klt_util.h
 *********************************************************************/

#ifndef _KLT_UTIL_H_
#define _KLT_UTIL_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct  {
  int ncols;
  int nrows;
  float *data;
}  _KLT_FloatImageRec, *_KLT_FloatImage;

_KLT_FloatImage _KLTCreateFloatImage(
  int ncols,
  int nrows);

void _KLTFreeFloatImage(
  _KLT_FloatImage);

#if 0
void _KLTPrintSubFloatImage(
  _KLT_FloatImage floatimg,
  int x0, int y0,
  int width, int height);
#endif

#if 0
void _KLTWriteFloatImageToPGM(
  _KLT_FloatImage img,
  char *filename);

/* for affine mapping */
void _KLTWriteAbsFloatImageToPGM(
  _KLT_FloatImage img,
  char *filename,float scale);
#endif

void _KLTApply3x3HomogToXY(
  float x0, float y0, const float *h,
  float *x1, float *y1);

#ifdef __cplusplus
}
#endif

#endif


/* Copyright: public domain */
