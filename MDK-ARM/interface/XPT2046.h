#ifndef XPT2046_H
#define XPT2046_H

#include "main.h"
#include <stdbool.h>

void XPT2046_Init(void);
void XPT2046_Read(uint16_t *x, uint16_t *y);
bool XPT2046_IsTouched(void);
void XPT2046_IRQ_Callback(void);
void XPT2046_SetCalibration(float x_fac, float y_fac, short x_off, short y_off);

#endif
