/*
 * =============================================================================
 *
 *       Filename:  qrenc.h
 *
 *    Description:  生成二维码PNG图片
 *
 *        Version:  1.0
 *        Created:  2019-05-25 14:52:46 
 *       Revision:  none
 *
 *         Author:  xubin
 *        Company:  Taichuan
 *
 * =============================================================================
 */
#ifndef _QRENC_H
#define _QRENC_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

	/* ---------------------------------------------------------------------------*/
	/**
	 * @brief qrcodeString 调用接口生成二维码
	 *
	 * @param string 二维码字符串内容
	 * @param path 保存图片路径，包含扩展名，例如xx/xx.png
	 *
	 * @returns 
	 */
	/* ---------------------------------------------------------------------------*/
	int qrcodeString(unsigned char *string,char *path);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
