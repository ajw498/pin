/*
	Pin
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
#define DIRPREFIX "Pin"

#define menuitem_INFO 0
#define menuitem_QUIT 1
#define MAXDIRS 20

typedef struct {
	char pathname[256];
	char text[1024];
} datas;

/*	Variables  */

window_handle mainwin,shadow;
datas data[MAXDIRS];
int numdirs=0;
int starttime=0;
int showtime=0;

/*	Functions  */

BOOL Null(event_pollblock *block, void *r)
{
	int endtime;
	window_info blk;
	Window_GetInfo(mainwin,&blk);
	if (blk.block.behind!=-1) Pane_BringToFront(shadow);
	SWI(0,1,SWI_OS_ReadMonotonicTime,&endtime);
	if (starttime==-1) {
		SWI(0,1,SWI_OS_ReadMonotonicTime,&starttime);
	} else {
		if (starttime==0) {
			starttime=-1;
		} else {
			if (endtime-starttime>showtime) {
				Pane_Hide(shadow);
				event_mask.data.null=TRUE;
			}
		}
	}
	return TRUE;
}

BOOL ReceiveMsg(event_pollblock *block, void *r)
{
	char buffer[260];
	int i;
	OS_GSTrans(block->data.message.data.dataopen.filename,buffer,256,NULL);
	for (i=0;i<numdirs;i++) {
		if (stricmp(buffer,data[i].pathname)==0) {
			Icon_SetText(mainwin,0,data[i].text);
			Pane_Show(shadow,open_CENTERED);
			event_mask.data.null=FALSE;
			starttime=0;
		}
	}
	return TRUE;
}

void LoadDirs(void)
{
	int i=0,c;
	char pathname[260],text[1024];
	_kernel_oserror *error;
	FILE *file=fopen("<"DIRPREFIX"$Dir>.Choices","r");
	error=_kernel_last_oserror();
	if (file==NULL) Error_ReportFatal(1,"Unable to open choices file: %s",error->errmess);
	fscanf(file,"%d",&showtime);
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
		if (c=='\n') text[0]='\0';
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
		OS_GSTrans(pathname,data[numdirs].pathname,256,NULL);
		strcpy(data[numdirs++].text,text);
	}
	fclose(file);
	if (numdirs==0) Event_CloseDown();
}

int main(void)
{
	wimp_point offset={-12,-12};
	/*Error_RegisterSignals();*/
	Resource_Initialise(DIRPREFIX);
	Msgs_LoadFile("Messages");
	Event_Initialise(Msgs_TempLookup("Task.Name:"));
	EventMsg_Initialise();
	Screen_CacheModeInfo();
	Template_Initialise();
	EventMsg_Claim(message_MODECHANGE,event_ANY,Handler_ModeChange,NULL);
	Event_Claim(event_CLOSE,event_ANY,event_ANY,Handler_CloseWindow,NULL);
	Event_Claim(event_OPEN,event_ANY,event_ANY,Pane_OpenEventHandler,NULL);
	Event_Claim(event_KEY,event_ANY,event_ANY,Handler_KeyPress,NULL);
	Event_Claim(event_REDRAW,event_ANY,event_ANY,Handler_HatchRedraw,NULL);
	Event_Claim(event_NULL,event_ANY,event_ANY,Null,NULL);
/*	Icon_BarIcon(Msgs_TempLookup("Task.Icon:"),iconbar_RIGHT);
	info=Window_CreateInfoWindowFromMsgs("Task.Name:","Task.Purpose:","©Alex Waugh 1998",VERSION);*/
	Template_LoadFile("Templates");
	mainwin=Window_Create("Main",template_TITLEMIN);
	shadow=Window_Create("Shadow",template_TITLEMIN);
	Pane_Link(shadow,mainwin,&offset,NULL,pane_FIXED);
/*	iconbarmenu=Menu_CreateFromMsgs("Menu.Title:","Menu.IconBar:Info,Quit",IconBarMenuClick,NULL);
	Menu_AddSubMenu(iconbarmenu,menuitem_INFO,(menu_ptr)info);
	Menu_Attach(window_ICONBAR,event_ANY,iconbarmenu,button_MENU);
	Event_Claim(event_CLICK,window_ICONBAR,event_ANY,IconBarClick,NULL);*/
	EventMsg_Claim(message_DATAOPEN,event_ANY,ReceiveMsg,NULL);
	LoadDirs();
	event_mask.data.null=TRUE;
	while (TRUE) Event_Poll();
	return 0;
}

