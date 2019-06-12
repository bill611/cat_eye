/*
 * Copyright (C) 2019 Rockchip Electronics Co., Ltd.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL), available from the file
 * COPYING in the main directory of this source tree, or the
 * OpenIB.org BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef HAL_I2C_H_
#define HAL_I2C_H_
#include <inttypes.h>

#define I2C_SUCCESS  0x00
#define I2C_ERROR    0x01

#define I2C_BUFFER_SIZE 64

#define I2C_BUS 1
#define I2C_SLAVE_ADDRESS 0x10

int32_t i2c_init(uint16_t busno, uint16_t slaveaddr);

int32_t i2c_read_byte(uint16_t regbase, uint16_t regoffset, uint8_t *val);

int32_t i2c_read_word(uint16_t reg, uint16_t subreg, uint16_t *val);

int32_t i2c_read_double_word(uint16_t reg, uint16_t subreg, uint32_t *val);

int32_t i2c_read_n_bytes(uint8_t *reg, uint8_t len, uint8_t *val);

int32_t i2c_write_byte(uint16_t regbase, uint16_t regoffset, uint8_t val);

int32_t i2c_write_word(uint16_t reg, uint16_t subreg, uint16_t val);

int32_t i2c_write_double_word(uint16_t reg, uint16_t subreg, uint32_t val);

int32_t i2c_write_n_bytes(uint8_t len,uint8_t *val);

int32_t i2c_release();


#endif // HAL_I2C_H_
