/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 2015 Daniel Gibson

This file is part of the Doom 3 BFG Edition GPL Source Code ("Doom 3 BFG Edition Source Code").

Doom 3 BFG Edition Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 BFG Edition Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 BFG Edition Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 BFG Edition Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 BFG Edition Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#ifndef __BFGIMGUI_H__
#define __BFGIMGUI_H__

#include "ImGui_Hooks.h"

#include <imgui.h>
#include <imgui_stdlib.h>

#include "../idlib/math/Vector.h"

// add custom functions for imgui
namespace ImGui
{

bool DragVec3( const char* label, idVec3& v, float v_speed = 1.0f,
			   float v_min = 0.0f, float v_max = 0.0f,
			   const char* display_format = "%.1f",
			   float power = 1.0f, bool ignoreLabelWidth = true );

bool DragVec3fitLabel( const char* label, idVec3& v, float v_speed = 1.0f,
					   float v_min = 0.0f, float v_max = 0.0f,
					   const char* display_format = "%.1f", float power = 1.0f );

}

class idImGuiHookLocal : public idImGuiHook
{
public:
	idImGuiHookLocal();

	// ---------------------- Public idImGuiHook Interface -------------------

	virtual bool	Init( int windowWidth, int windowHeight );

	virtual bool	IsInitialized();

	// tell imgui that the (game) window size has changed
	virtual void	NotifyDisplaySizeChanged( int width, int height );

	// inject a sys event (keyboard, mouse, unicode character)
	virtual bool	InjectSysEvent( const sysEvent_t* keyEvent );

	// inject the current mouse wheel delta for scrolling
	virtual bool	InjectMouseWheel( int delta );

	// call this once per frame *before* calling ImGui::* commands to draw widgets etc
	// (but ideally after getting all new events)
	virtual void	NewFrame();

	// call this to enable custom ImGui windows which are not editors
	virtual bool	IsReadyToRender();

	// call this once per frame (at the end) - it'll render all ImGui::* commands
	// since NewFrame()
	virtual void	Render();

	virtual void	Destroy();

protected:

	bool			HandleKeyEvent( const sysEvent_t& keyEvent );
	// Gross hack. I'm sorry.
	// sets the kb-layout specific keys in the keymap
	void			FillCharKeys( int* keyMap );
	// Sys_GetClipboardData() expects that you Mem_Free() its returned data
	// ImGui can't do that, of course, so copy it into a static buffer here,
	// Mem_Free() and return the copy
	static const char* 	GetClipboardText( void* );
	static void		SetClipboardText( void*, const char* text );
	bool			ShowWindows();
	bool			UseInput();

private:

	bool	g_IsInit;
	double	g_Time;
	bool	g_MousePressed[5];
	float	g_MouseWheel;
	ImVec2	g_MousePos;
	ImVec2	g_DisplaySize;
	bool	g_haveNewFrame;
};

extern idImGuiHookLocal imguiLocal;

#endif /* __BFGIMGUI_H__ */
