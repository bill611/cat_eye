/*
 * =============================================================================
 *
 *       Filename:  screen.c
 *
 *    Description:  窗口链表
 *
 *        Version:  1.0
 *        Created:  2019-04-20 15:08:19
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
#include <stdlib.h>
#include <string.h>
#include "screen.h"

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

ScreenForm Screen;

static BOOL screenAddForm(HWND hWnd,const char *Class)
{
	FormClass * Form = (FormClass *)malloc(sizeof(FormClass));
	memset(Form,0,sizeof(FormClass));
	Form->hWnd = hWnd;
	strncpy(Form->Class,Class,15);
	if(Screen.head == NULL) {
		Screen.head = Screen.tail = Form;
	} else {
		Screen.tail->next = Form;
		Screen.tail = Form;
	}
	Screen.Count++;
	return TRUE;
}
//--------------------------------------------------------------------------
static BOOL screenDelForm(HWND hWnd)
{
	FormClass * form,*parentform;
	form = Screen.head;
	while(form)	{
		if(form->hWnd != hWnd) {
			parentform = form;
			form = form->next;
			continue;
		}
		// DBG_P("Form(%s)Del\n",form->Class);
		if(Screen.head == form) {
			Screen.head = form->next;
			if(Screen.tail == form) Screen.tail = NULL;
		} else {
			parentform->next = form->next;
			if(Screen.tail == form) Screen.tail = parentform;
		}
		free(form);
		Screen.Count--;
		return TRUE;
	}
	return FALSE;
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief screenReturnMainForm 回到主界面
 */
/* ---------------------------------------------------------------------------*/
static void screenReturnMainForm(void)
{
	int FormCnt=0;
	HWND *Forms;
	FormClass * form;
	form = Screen.head;
	Forms = (HWND *)calloc(sizeof(HWND),Screen.Count);
	while(form && FormCnt<Screen.Count) {
		//留下主窗口
		if(strcmp(form->Class,"TFrmMain"))
			Forms[FormCnt++] = form->hWnd;
		form = form->next;
	}
	while(FormCnt) {
        ShowWindow(Forms[FormCnt-1],SW_HIDE);
		// SendMessage(Forms[FormCnt-1],MSG_CLOSE,0,-1);
		FormCnt--;
	}
	free(Forms);
}

//--------------------------------------------------------------------------
static HWND screenFindForm(const char *Class)
{
	FormClass * form = Screen.head;
	if(Class==NULL)
		return 0;

	while(form) {
		if(strncmp(Class,form->Class,15)==0)
			return form->hWnd;
		form = form->next;
	}
	return 0;
}
/* ---------------------------------------------------------------------------*/
/**
 * @brief screenForeachForm 遍历所有窗口，发送消息
 *
 * @param Class
 *
 * @returns 
 */
/* ---------------------------------------------------------------------------*/
static void screenForeachForm(int iMsg, WPARAM wParam, LPARAM lParam)
{
	int FormCnt=0;
	FormClass * form;
	form = Screen.head;
	while(form && FormCnt<Screen.Count) {
		SendMessage(form->hWnd,iMsg,wParam,lParam);
		form = form->next;
	}
}

void screenInit(void)
{
	Screen.Add = screenAddForm;
	Screen.Del = screenDelForm;
	Screen.Find = screenFindForm;
	Screen.ReturnMain = screenReturnMainForm;
	Screen.foreachForm = screenForeachForm;
}

