/*
	PinEdit
	©Alex Waugh 1999,2002

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.
	
	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

*/

/*	Includes  */

#include "DeskLib:Window.h"
#include "DeskLib:Error.h"
#include "DeskLib:Event.h"
#include "DeskLib:EventMsg.h"
#include "DeskLib:Handler.h"
#include "DeskLib:Hourglass.h"
#include "DeskLib:Icon.h"
#include "DeskLib:Menu.h"
#include "DeskLib:Msgs.h"
#include "DeskLib:Resource.h"
#include "DeskLib:Screen.h"
#include "DeskLib:Template.h"
#include "DeskLib:File.h"
#include "DeskLib:Filing.h"
#include "DeskLib:Str.h"
#include "DeskLib:KernelSWIs.h"
#include "DeskLib:Pane.h"

#include "AJWLib:Window.h"
#include "AJWLib:Menu.h"
#include "AJWLib:Msgs.h"
#include "AJWLib:Misc.h"
#include "AJWLib:Handler.h"
#include "AJWLib:Error.h"
#include "AJWLib:Pane.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "kernel.h"

/*	Macros  */

#define VERSION "0.02 (1-Dec-02)"
#define DIRPREFIX "PinEdit"

#define icon_NUMBER 0
#define icon_DIRECTORY 3
#define icon_TEXTMSG 4
#define icon_TIME 7
#define icon_UP 2
#define icon_DOWN 1
#define icon_SAVE 5
#define icon_CANCEL 6

#define menuitem_INFO 0
#define menuitem_QUIT 1

#define MAXDIRS 20

typedef struct {
	char pathname[256];
	char text[1024];
} datas;

/*	Variables  */

window_handle mainwin;
datas data[MAXDIRS];
int numdirs=0;
int starttime=0;
int showtime=0;
int dirnum=1;
int olddirnum=0;

/*	Functions  */

void ShowInfo(void)
{
	char num[5];
	if (olddirnum!=0) {
		Icon_GetText(mainwin,icon_DIRECTORY,data[olddirnum-1].pathname);
		Icon_GetText(mainwin,icon_TEXTMSG,data[olddirnum-1].text);
		Icon_GetText(mainwin,icon_TIME,num);
		showtime=100*atoi(num);
	}
	sprintf(num,"%d",dirnum);
	Icon_SetText(mainwin,icon_NUMBER,num);
	Icon_SetText(mainwin,icon_DIRECTORY,data[dirnum-1].pathname);
	Icon_SetText(mainwin,icon_TEXTMSG,data[dirnum-1].text);
	sprintf(num,"%d",showtime/100);
	Icon_SetText(mainwin,icon_TIME,num);
	Icon_SetCaret(mainwin,icon_DIRECTORY);
	olddirnum=dirnum;
}

BOOL ReceiveMsg(event_pollblock *block, void *r)
{
	Icon_SetText(mainwin,icon_DIRECTORY,block->data.message.data.dataopen.filename);
	return TRUE;
}

void LoadDirs(void)
{
	int i=0,c;
	char pathname[260],text[1024];
	_kernel_oserror *error;
	FILE *file=fopen("<Pin$Dir>.Choices","r");
	error=_kernel_last_oserror();
	if (file==NULL) Error_ReportFatal(1,"Unable to open choices file: %s",error->errmess);
	fscanf(file,"%d\n",&showtime);
	c='\n';
	numdirs=0;
	while (c!=EOF && numdirs<MAXDIRS) {
		i=0;
		c=fgetc(file);
		while (c!=' ' && c!=EOF && c!='\n' && i<255) {
			pathname[i++]=c;
			c=fgetc(file);
		}
		pathname[i]='\0';
		if (c=='\n' || c==EOF) text[0]='\0';
		if (c!=EOF) {
			if (c==' ') {
				i=0;
				c=fgetc(file);
				while (c!=EOF && c!='\n' && i<1023) {
					text[i++]=c;
					c=fgetc(file);
				}
				text[i]='\0';
			}
		}
		strcpy(data[numdirs].pathname,pathname);
		strcpy(data[numdirs++].text,text);
	}
	fclose(file);
}

BOOL CloseWin(event_pollblock *block, void *r)
{
	Event_CloseDown();
	return TRUE;
}

BOOL Save(void)
{
	_kernel_oserror *error;
	int i;
	FILE *file=fopen("<Pin$Dir>.Choices","w");
	error=_kernel_last_oserror();
	if (file==NULL) Error_ReportFatal(1,"Unable to open choices file: %s",error->errmess);
	ShowInfo();
	fprintf(file,"%d\n",showtime);
	for (i=0;i<MAXDIRS;i++) {
		if (data[i].pathname[0]!='\n') fprintf(file,"%s %s\n",data[i].pathname,data[i].text);
	}
	fclose(file);
	return TRUE;
}

BOOL Click(event_pollblock *block, void *r)
{
	switch (block->data.mouse.icon) {
		case icon_UP:
			if (block->data.mouse.button.data.select) dirnum++;
			if (block->data.mouse.button.data.adjust) dirnum--;
			if (dirnum<1) dirnum=1;
			if (dirnum>MAXDIRS) dirnum=MAXDIRS;
			ShowInfo();
			break;
		case icon_DOWN:
			if (block->data.mouse.button.data.select) dirnum--;
			if (block->data.mouse.button.data.adjust) dirnum++;
			if (dirnum<1) dirnum=1;
			if (dirnum>MAXDIRS) dirnum=MAXDIRS;
			ShowInfo();
			break;
		case icon_CANCEL:
			if (block->data.mouse.button.data.select) CloseWin(NULL,NULL);
			break;
		case icon_SAVE:
			if (block->data.mouse.button.data.select || block->data.mouse.button.data.adjust) {
				Save();
				if (block->data.mouse.button.data.select) Event_CloseDown();
			}
	}
	return TRUE;
}

int main(void)
{
	/*Error_RegisterSignals();*/
	Resource_Initialise(DIRPREFIX);
	Msgs_LoadFile("Messages");
	Event_Initialise(Msgs_TempLookup("Task.Name:"));
	EventMsg_Initialise();
	Screen_CacheModeInfo();
	Template_Initialise();
	EventMsg_Claim(message_MODECHANGE,event_ANY,Handler_ModeChange,NULL);
	Event_Claim(event_CLOSE,event_ANY,event_ANY,CloseWin,NULL);
	Event_Claim(event_OPEN,event_ANY,event_ANY,Handler_OpenWindow,NULL);
	Event_Claim(event_KEY,event_ANY,event_ANY,Handler_KeyPress,NULL);
	Event_Claim(event_REDRAW,event_ANY,event_ANY,Handler_HatchRedraw,NULL);
/*	Icon_BarIcon(Msgs_TempLookup("Task.Icon:"),iconbar_RIGHT);
	info=Window_CreateInfoWindowFromMsgs("Task.Name:","Task.Purpose:","©Alex Waugh 1998",VERSION);*/
	Template_LoadFile("Templates");
	mainwin=Window_CreateAndShow("Main",template_TITLEMIN,open_CENTERED);
/*	iconbarmenu=Menu_CreateFromMsgs("Menu.Title:","Menu.IconBar:Info,Quit",IconBarMenuClick,NULL);
	Menu_AddSubMenu(iconbarmenu,menuitem_INFO,(menu_ptr)info);
	Menu_Attach(window_ICONBAR,event_ANY,iconbarmenu,button_MENU);
	Event_Claim(event_CLICK,window_ICONBAR,event_ANY,IconBarClick,NULL);*/
	Event_Claim(event_CLICK,mainwin,event_ANY,Click,NULL);
	EventMsg_Claim(message_DATALOAD,event_ANY,ReceiveMsg,NULL);
	LoadDirs();
	dirnum=1;
	ShowInfo();
	while (TRUE) Event_Poll();
	return 0;
}

