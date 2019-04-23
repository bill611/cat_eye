/*
 * =====================================================================================
 *
 *       Filename:  LinkList.h
 *
 *    Description:  通用链表函数
 *
 *        Version:  1.0
 *        Created:  2015-11-04 16:11:54
 *       Revision:  none
 *
 *         Author:  xubin
 *        Company:  Taichuan
 *
 * =====================================================================================
 */
#ifndef _LINK_LIST_H
#define _LINK_LIST_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

// #define TEST

#define LISTLINK_OK 1
#define LISTLINK_FAIL 0
    struct _ListPriv;
	typedef struct _List{
        struct _ListPriv *priv;   

		void (*clear)(struct _List*);						/* 清空链表 */
		int (*append)(struct _List*,void *);				/* 追加元素 */
		int (*insert)(struct _List*,int,void *);			/* 加入元素 */
		int (*delete)(struct _List *,int);					/* 删除第几个元素 */

		int (*foreachStart)(struct _List*,int );			/* 开始遍历 */
		int (*foreachNext)(struct _List*);					/* 遍历下一个 */
		int (*foreachGetElem)(struct _List*,void *);		/* 获取遍历元素 */
		int (*foreachEnd)(struct _List*);					/* 判断是否遍历结束 */

		int (*getElem)(struct _List*,int,void * );			/* 取得第几个元素的值用第三个参数返回 */
		int (*getElemTail)(struct _List*,void *);			/* 取得最后一个元素的内容 */
		int (*traverse)(struct _List*,int (*)(void * ));	/* 遍历访问，访问某个节点元素用函数处理 */
		void (*destory)(struct _List*);						/* 销毁链表 */
	}List;

	List * listCreate(unsigned int size);						/* 创建链表 */

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
