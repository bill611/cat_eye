/*
 * =====================================================================================
 *
 *       Filename:  LinkList.c
 *
 *    Description:  通用链表函数
 *
 *        Version:  1.0
 *        Created:  2015-11-04 17:28:56
 *       Revision:  none
 *
 *         Author:  xubin
 *        Company:  Taichuan
 *
 * =====================================================================================
 */
/* ----------------------------------------------------------------*
 *                      include head files
 *-----------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>

#include "linklist.h"
#include "debug.h"


/* ----------------------------------------------------------------*
 *                  extern variables declare
 *-----------------------------------------------------------------*/

/* ----------------------------------------------------------------*
 *                  internal functions declare
 *-----------------------------------------------------------------*/

/* ----------------------------------------------------------------*
 *                        macro define
 *-----------------------------------------------------------------*/
typedef struct node {
    void * data;
    struct node * next;
}ChainNode;

typedef struct _ListPriv {
    ChainNode *head; // 头节点
    int nodesize;  // 数据大小
    ChainNode *tail; // 尾节点
    ChainNode *temp; // foreach 暂存节点
}ListPriv;

/* ----------------------------------------------------------------*
 *                      variables define
 *-----------------------------------------------------------------*/

/* ----------------------------------------------------------------*/
/**
 * @brief newChainNode 新建链表节点
 *
 * @param data 节点数据
 *
 * @returns 1成功 0失败
 */
/* ----------------------------------------------------------------*/
static ChainNode * newChainNode(void * data)
{
	ChainNode * pChain = 0;
	pChain = ( ChainNode * )calloc(1, sizeof(ChainNode) );

	if( !pChain ) {
		DPRINT("Err: [%s] malloc fail\n", __FUNCTION__);
		return 0;
	}

	pChain->data = data;
	pChain->next = 0;

	return pChain;
}

/* ----------------------------------------------------------------*/
/**
 * @brief listAppend 追加链表元素
 *
 * @param This 目标链表
 * @param pos 加入的数据
 *
 * @returns 1成功 0失败
 */
/* ----------------------------------------------------------------*/
static int listAppend(List * This,void *pos)
{
	ChainNode * newpt = 0;
	void * data;

	if( !This )
		return LISTLINK_FAIL;

	data = (void *)malloc(This->priv->nodesize);
	if( !data )
		return LISTLINK_FAIL;

	memcpy(data,pos,This->priv->nodesize);
	newpt = newChainNode(data);

    if( !newpt ) {
        free(data);
		return LISTLINK_FAIL;
    }

	if (!This->priv->head) {
		This->priv->head = newpt;
		This->priv->tail = This->priv->head;
	} else {
		This->priv->tail->next = newpt;
		This->priv->tail = newpt;
	}

	return LISTLINK_OK;
}

/* ----------------------------------------------------------------*/
/**
 * @brief listGetAddr 取得编号为n的元素所在地址
 *
 * @param This 目标链表
 * @param n 目标编号位置
 *
 * @returns 1成功 0失败
 */
/* ----------------------------------------------------------------*/
static ChainNode * listGetAddr(List * This,int n)
{
	ChainNode * pt = 0;
	int a = 0;

	if( n < 0) {
		DPRINT("Err: [%s] need n > 0\n", __FUNCTION__);
		return NULL;
	}

	pt = This->priv->head;

	while( pt && a < n ) {
		pt = pt->next;
		a++;
	}
	return pt;
}

/* ----------------------------------------------------------------*/
/**
 * @brief listInsert 加入元素
 *
 * @param This 目标链表
 * @param n 加入位置,当前链表后移
 * @param pos 加入的数据
 *
 * @returns 1成功 0失败
 */
/* ----------------------------------------------------------------*/
static int listInsert(List * This, int n ,void *pos)
{
	ChainNode * pt = NULL;
	ChainNode * newpt = NULL;
	void * data = NULL;
	// 数据先分配内存拷贝,再创建新节点
	data = (void*)malloc(This->priv->nodesize);
	if( !data )
		goto insert_err;
	memcpy(data,pos,This->priv->nodesize);

	newpt = newChainNode(data);
	if( !newpt )
		return LISTLINK_FAIL;

	if (n == 0) {
		pt = This->priv->head;
		This->priv->head = newpt;
		This->priv->head->next = pt;
	} else  {
		pt = listGetAddr( This, n-1 );
	}
	if( !pt )
		goto insert_err;

	newpt->next = pt->next;

	pt->next = newpt;

	return LISTLINK_OK;

insert_err:
	if (newpt)
		free(newpt);
	if (data)
		free(data);
	return LISTLINK_FAIL;
}

/* ----------------------------------------------------------------*/
/**
 * @brief listGetElem 取得第几个元素的值
 *
 * @param This 目标链表
 * @param n 取得元素的位置
 * @param data 取得元素的数据
 *
 * @returns 1成功 0失败
 */
/* ----------------------------------------------------------------*/
static int listGetElem(List * This,int n,void * data)
{
	ChainNode * pt = 0;

	if( !data )
		return LISTLINK_FAIL;

	pt = listGetAddr(This,n);
	if( ! pt )
		return LISTLINK_FAIL;

	memcpy(data, pt->data ,This->priv->nodesize);

	return LISTLINK_OK;
}

/* ----------------------------------------------------------------*/
/**
 * @brief listGetElemTail 取得最后一个元素的数据
 *
 * @param This 目标链表
 *
 * @returns 元素内容
 */
/* ----------------------------------------------------------------*/
static int listGetElemTail(List *This,void *data)
{
    if (!data) {
        DPRINT("data is null\n");
        return LISTLINK_FAIL;
    }
	memcpy(data, This->priv->tail->data ,This->priv->nodesize);
    return LISTLINK_OK;
}
/* ----------------------------------------------------------------*/
/**
 * @brief listTraverseList 遍历访问，访问某个节点元素用函数处理
 *
 * @param This 目标链表
 * @param func 访问该节点元素的处理函数
 *
 * @returns 1成功 0失败
 */
/* ----------------------------------------------------------------*/
static int listTraverseList(List* This,int (*func)(void * ))
{
	ChainNode * pt = 0;
	int a=0;

	if( !(This && This->priv->head) )
		return LISTLINK_FAIL;
	for( a = 0 ,pt = This->priv->head->next; pt ; pt = pt->next ) {
		if( ! func( (pt->data)) )
			return a+1;
		a++;
	}
	return LISTLINK_FAIL;
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief listForeachStart 链表遍历开始
 *
 * @param This
 * @param n 从链表第几个下标开始 首地址为1
 *
 * @returns 
 */
/* ---------------------------------------------------------------------------*/
static int listForeachStart(List* This,int n)
{
    if (n < 0) {
        DPRINT("Err: [%s] need n > 0\n",__FUNCTION__ );
        This->priv->temp = NULL;
        return LISTLINK_FAIL;
    }
    This->priv->temp = listGetAddr(This,n);    
    return LISTLINK_OK;
}
static int listForeachNext(List* This)
{
    This->priv->temp = This->priv->temp->next; 
    return LISTLINK_OK;
}
static int listForeachGetElem(List* This,void *data)
{
    if (!This->priv->temp)
		return LISTLINK_FAIL;

	memcpy(data, This->priv->temp->data ,This->priv->nodesize);
	return LISTLINK_OK;
}
static int listForeachEnd(List* This)
{
    if (This->priv->temp)
        return LISTLINK_OK;
    else
        return LISTLINK_FAIL;
}
/* ----------------------------------------------------------------*/
/**
 * @brief listDelete 删除第几个元素
 *
 * @param This 目标链表
 * @param n 删除元素位置
 *
 * @returns 1成功 0失败
 */
/* ----------------------------------------------------------------*/
static int listDelete( List * This,  int n )
{
	ChainNode * pt =0;
	ChainNode * pf=0;

	if (!This->priv->head) {
		DPRINT("Err: [%s] empty list \n",__FUNCTION__);
		return LISTLINK_FAIL;
	}

	if (n == 0) {
		pf = This->priv->head;
		This->priv->head = pf->next;
		goto delete_end;
	} else {
		pt = listGetAddr( This,n-1 );
	}
	if( !(pt && pt->next ))
		return LISTLINK_FAIL;

	if ( pt->next == This->priv->tail )
		This->priv->tail = pt;


	pf = pt->next;
	pt->next = pt->next->next;

delete_end:
	if (pf) {
		if (pf->data)
			free(pf->data);
		free(pf);
	}

	return LISTLINK_OK;

}

/* ----------------------------------------------------------------*/
/**
 * @brief listClearList 清空链表
 *
 * @param This 目标链表
 */
/* ----------------------------------------------------------------*/
static void listClearList(List * This)
{
	while ( listDelete(This,0) == LISTLINK_OK );
}

/* ----------------------------------------------------------------*/
/**
 * @brief listDestory 销毁链表
 *
 * @param This 销毁对象
 */
/* ----------------------------------------------------------------*/
static void listDestory(List * This)
{
	This->clear(This);

	free(This->priv->head);
	This->priv->head = 0;
    
    free(This->priv);

	free(This);
	This = 0;

}

/* ----------------------------------------------------------------*/
/**
 * @brief CreateList 创建链表
 *
 * @param size 数据长度
 *
 * @returns 创建的链表
 */
/* ----------------------------------------------------------------*/
List *listCreate(unsigned int size )
{
	List * This = 0;
	// void * data = 0;
	This=(List*)calloc(1, sizeof(List) );
	if( !This )
		return LISTLINK_FAIL;
    This->priv = (ListPriv *)calloc(1,sizeof(ListPriv));
    if (!This->priv) {
        free(This);
        return LISTLINK_FAIL;
    }

	This->priv->head = 0;//newChainNode(data );
	// if( ! This->priv->head ) {
		// free(This);
		// return LISTLINK_FAIL;
	// }
	This->priv->nodesize = size;
	This->priv->tail = This->priv->head;
	
	This->clear 	= listClearList;
	This->append 	= listAppend;
	This->insert 	= listInsert;
	This->delete 	= listDelete;

	This->foreachStart 	= listForeachStart;
	This->foreachNext 	= listForeachNext;
	This->foreachGetElem 	= listForeachGetElem;
	This->foreachEnd 	= listForeachEnd;

	This->getElem 		= listGetElem;
	This->getElemTail 	= listGetElemTail;
	This->traverse 	= listTraverseList;
	This->destory	= listDestory;

	return This;
}

#ifdef TEST
int main(int argc, char *argv[])
{
	char *a = "123";
	char *a1 = "223";
	char *a2 = "323";
	char *a3 = "423";
    List *plist = listCreate(sizeof(char *));
    plist->append(plist,&a);
    plist->append(plist,&a1);
	plist->append(plist,&a2);

    plist->insert(plist,1,&a3);

    plist->delete(plist,3);

    char *b;
	if (plist->getElemTail(plist,&b) == LISTLINK_OK)
		DPRINT("[get tail]%s\n", b);
    if (plist->getElem(plist,0,&b) == LISTLINK_OK)
        DPRINT("[get elem]%s\n", b);

	plist->foreachStart(plist,0);
	while(plist->foreachEnd(plist)) {
		char *c;
		plist->foreachGetElem(plist,&c);
		DPRINT("[foreach]%s\n", c);
		plist->foreachNext(plist);
	}


	return 0;
}
#endif
