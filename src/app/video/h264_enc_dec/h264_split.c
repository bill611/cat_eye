/*
 * =============================================================================
 *
 *       Filename:  h264_split.c
 *
 *    Description:  h264 sps pps分隔
 *
 *        Version:  virsion
 *        Created:  2019-08-15 09:01:47
 *       Revision:  none
 *
 *         Author:  xubin
 *        Company:  Taichuan
 *
 * =============================================================================
 */
/* ---------------------------------------------------------------------------*
 *                      include head files
 *----------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdint.h>


/* ---------------------------------------------------------------------------*
 *                  extern variables declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                  internal functions declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                        macro define
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/

// Copy from ffmpeg.
static const uint8_t *find_startcode_internal(const uint8_t *p,
		const uint8_t *end) 
{
	const uint8_t *a = p + 4 - ((intptr_t)p & 3);

	for (end -= 3; p < a && p < end; p++) {
		if (p[0] == 0 && p[1] == 0 && p[2] == 1)
			return p;
	}

	for (end -= 3; p < end; p += 4) {
		uint32_t x = *(const uint32_t *)p;
		//      if ((x - 0x01000100) & (~x) & 0x80008000) // little endian
		//      if ((x - 0x00010001) & (~x) & 0x00800080) // big endian
		if ((x - 0x01010101) & (~x) & 0x80808080) { // generic
			if (p[1] == 0) {
				if (p[0] == 0 && p[2] == 1)
					return p;
				if (p[2] == 0 && p[3] == 1)
					return p + 1;
			}
			if (p[3] == 0) {
				if (p[2] == 0 && p[4] == 1)
					return p + 2;
				if (p[4] == 0 && p[5] == 1)
					return p + 3;
			}
		}
	}

	for (end += 3; p < end; p++) {
		if (p[0] == 0 && p[1] == 0 && p[2] == 1)
			return p;
	}

	return end + 3;
}

static const uint8_t *find_h264_startcode(const uint8_t *p, const uint8_t *end) 
{
	const uint8_t *out = find_startcode_internal(p, end);
	if (p < out && out < end && !out[-1])
		out--;
	return out;
}

int split_h264_separate(const uint8_t *buffer, size_t length,int *sps_length,int *pps_length) 
{
	const uint8_t *p = buffer;
	const uint8_t *end = p + length;
	const uint8_t *nal_start = NULL, *nal_end = NULL;
	nal_start = find_h264_startcode(p, end);
	// 00 00 01 or 00 00 00 01
	size_t start_len = (nal_start[2] == 1 ? 3 : 4);
	uint8_t nal_type = 1;
	for (;;) {
		if (nal_start == end)
			break;
		nal_start += start_len;
		nal_end = find_h264_startcode(nal_start, end);
		size_t size = nal_end - nal_start + start_len;
		nal_type = (*nal_start) & 0x1F;
		if (nal_type == 7) {
			*sps_length = size;
		} else if (nal_type == 8) {
			*pps_length = size;
		}
		// printf("nal_type:%d,length:%ld,size:%ld\n",nal_type,length,size );
		nal_start = nal_end;
	}
	return nal_type;
}
