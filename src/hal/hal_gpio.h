/*
 * =============================================================================
 *
 *       Filename:  hal_gpio.h
 *
 *    Description:  硬件层  GPIO控制
 *
 *        Version:  1.0
 *        Created:  2018-12-12 16:28:49 
 *       Revision:  none
 *
 *         Author:  xubin
 *        Company:  Taichuan
 *
 * =============================================================================
 */
#ifndef _HAL_GPIO_H
#define _HAL_GPIO_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */
	enum {
		HAL_INPUT,  // 输入
		HAL_OUTPUT, // 输出
	};
	/* ---------------------------------------------------------------------------*/
	/**
	 * @brief halGpioSetMode 硬件层 设置IO为输入或输出
	 *
	 * @param port_id IO编号
	 * @param port_mask IO码  （针对GPIOA,0123...的情况，没有则填-1）
	 * @param dir IO方向, HAL_INPUT或HAL_OUTPUT
	 */
	/* ---------------------------------------------------------------------------*/
	void halGpioSetMode(int port_id,int port_mask,int dir);
	/* ---------------------------------------------------------------------------*/
	/**
	 * @brief halGpioOut 硬件层 设置输出电平
	 *
	 * @param port_id
	 * @param port_mask
	 * @param value  0低电平  1高电平
	 */
	/* ---------------------------------------------------------------------------*/
	void halGpioOut(int port_id,int port_mask,int value);
	/* ---------------------------------------------------------------------------*/
	/**
	 * @brief halGpioIn 硬件层 获取电平输入值
	 *
	 * @param port_id
	 * @param port_mask
	 *
	 * @returns  0低电平  1高电平
	 */
	/* ---------------------------------------------------------------------------*/
	int halGpioIn(int port_id,int port_mask);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
