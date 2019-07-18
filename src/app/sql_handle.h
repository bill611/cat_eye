/*
 * =============================================================================
 *
 *       Filename:  sqlHandle.h
 *
 *    Description:  数据库操作接口
 *
 *        Version:  1.0
 *        Created:  2018-05-21 22:48:57 
 *       Revision:  none
 *
 *         Author:  xubin
 *        Company:  Taichuan
 *
 * =============================================================================
 */
#ifndef _SQL_HANDLE_H
#define _SQL_HANDLE_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#include <stdint.h>
	extern void sqlInsertUserInfoNoBack(
			char *user_id,
			char *login_token,
			char *nick_name,
			int type,
			int scope);
	extern void sqlInsertUserInfo(
			char *user_id,
			char *login_token,
			char *nick_name,
			int type,
			int scope);
	extern int sqlGetUserInfoUseType(
			int type,
			char *user_id,
			char *login_token,
			char *nick_name,
			int *scope);
	extern int sqlGetUserInfoUseUserId(
			char *user_id,
			char *nick_name,
			int *scope);
	extern int sqlGetUserInfoStart(int type);
	extern int sqlGetUserInfoUseScopeStart(int scope);
	extern void sqlGetUserInfosUseScope(
			char *user_id,
			char *nick_name);
	extern void sqlGetUserInfosUseScopeIndex(
			char *user_id,
			char *nick_name,
			int scope,
			int index);
	extern void sqlGetUserInfos(
			char *user_id,
			char *nick_name,
			int *scope);
	extern void sqlClearDevice(void);
	extern void sqlGetUserInfoEnd(void);
	extern void sqlInsertFace(char *user_id,
			char *nick_name,
			char *url,
			void *feature,
			int size);

    extern int sqlGetFaceCount(void);
	extern void sqlGetFaceStart(void);
	extern int sqlGetFace(char *user_id,char *nick_name,char *url,void *feature);
	extern void sqlGetFaceEnd(void);
	extern void sqlDeleteFace(char *id);
	extern void sqlCheckBack(void);
	extern void sqlInit(void);

	extern void sqlInsertRecordCapNoBack(
			char *date,
			uint64_t picture_id);
	extern void sqlInsertPicUrlNoBack(
			uint64_t picture_id,
			char *url);
	extern int sqlGetCapInfo(
			uint64_t picture_id,
			char *date);
	extern int sqlGetPicInfoStart(uint64_t picture_id);
	extern void sqlGetPicInfos(char *url);
	extern void sqlGetPicInfoEnd(void);
	extern void sqlInsertRecordAlarm(
			char *date_time,
			int type,
			int has_people,
			uint64_t picture_id);
	extern int sqlGetAlarmInfoUseDateType(
			char *date_time,
			int type,
			int *has_people);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
