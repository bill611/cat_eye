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
	extern void sqlGetUserInfos(
			char *user_id,
			char *nick_name,
			int *scope);
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

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
