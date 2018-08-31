#pragma once
#include "Z80.h"

void ZX_ReadFromFlash_Z80(Z80 *R, const uint8_t *data, uint16_t length);
void ZX_ReadFromTransfer_Z80(Z80 *R, uint8_t *data, uint16_t length);
uint16_t ZX_WriteToTransfer_Z80(Z80 *R, uint8_t *buffer_start);
