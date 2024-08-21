/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (c) 2014-2015 Omar Cornut and ImGui contributors
Copyright (C) 2015 Daniel Gibson
Copyright (C) 2016-2023 Robert Beckebans

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
#include "precompiled.h"
#pragma hdrstop

#include "BFGimgui.h"
#include "ImGuizmo.h"
#include "renderer/RenderCommon.h"
#include "renderer/RenderBackend.h"


idCVar imgui_showDemoWindow( "imgui_showDemoWindow", "0", CVAR_GUI | CVAR_BOOL, "show big ImGui demo window" );

// our custom ImGui functions from BFGimgui.h

// like DragFloat3(), but with "X: ", "Y: " or "Z: " prepended to each display_format, for vectors
// if !ignoreLabelWidth, it makes sure the label also fits into the current item width.
//    note that this screws up alignment with consecutive "value+label widgets" (like Drag* or ColorEdit*)
bool ImGui::DragVec3( const char* label, idVec3& v, float v_speed,
					  float v_min, float v_max, const char* display_format, float power, bool ignoreLabelWidth )
{
	bool value_changed = false;
	ImGui::BeginGroup();
	ImGui::PushID( label );

	ImGuiStyle& style = ImGui::GetStyle();
	float wholeWidth = ImGui::CalcItemWidth() - 2.0f * style.ItemSpacing.x;
	float spacing = style.ItemInnerSpacing.x;
	float labelWidth = ignoreLabelWidth ? 0.0f : ( ImGui::CalcTextSize( label, NULL, true ).x + spacing );
	float coordWidth = ( wholeWidth - labelWidth - 2.0f * spacing ) * ( 1.0f / 3.0f ); // width of one x/y/z dragfloat

	ImGui::PushItemWidth( coordWidth );
	for( int i = 0; i < 3; i++ )
	{
		ImGui::PushID( i );
		char format[64];
		idStr::snPrintf( format, sizeof( format ), "%c: %s", "XYZ"[i], display_format );
		value_changed |= ImGui::DragFloat( "##v", &v[i], v_speed, v_min, v_max, format, power );

		ImGui::PopID();
		ImGui::SameLine( 0.0f, spacing );
	}
	ImGui::PopItemWidth();
	ImGui::PopID();

	const char* labelEnd = strstr( label, "##" );
	ImGui::TextUnformatted( label, labelEnd );

	ImGui::EndGroup();

	return value_changed;
}

// shortcut for DragXYZ with ignorLabelWidth = false
// very similar, but adjusts width to width of label to make sure it's not cut off
// sometimes useful, but might not align with consecutive "value+label widgets" (like Drag* or ColorEdit*)
bool ImGui::DragVec3fitLabel( const char* label, idVec3& v, float v_speed,
							  float v_min, float v_max, const char* display_format, float power )
{
	return ImGui::DragVec3( label, v, v_speed, v_min, v_max, display_format, power, false );
}

idImGuiHookLocal	imguiLocal;
idImGuiHook* imgui = &imguiLocal;

// the ImGui hooks to integrate it into the engine

idImGuiHookLocal::idImGuiHookLocal() :
	g_IsInit( false ),
	g_Time( 0.0f ),
	g_MouseWheel( 0.0f ),
	g_MousePos( -1.0f, -1.0f ),
	g_DisplaySize( 0.0f, 0.0f ),
	g_haveNewFrame( false )
{
	for( int i = 0; i < 5; ++i )
	{
		g_MousePressed[i] = false;
	}
}

bool idImGuiHookLocal::Init( int windowWidth, int windowHeight )
{
	if( IsInitialized() )
	{
		Destroy();
	}

	common->Printf( "Initializing ImGui\n" );

	IMGUI_CHECKVERSION();

	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	// Keyboard mapping. ImGui will use those indices to peek into the io.KeyDown[] array.
	io.KeyMap[ImGuiKey_Tab] = K_TAB;
	io.KeyMap[ImGuiKey_LeftArrow] = K_LEFTARROW;
	io.KeyMap[ImGuiKey_RightArrow] = K_RIGHTARROW;
	io.KeyMap[ImGuiKey_UpArrow] = K_UPARROW;
	io.KeyMap[ImGuiKey_DownArrow] = K_DOWNARROW;
	io.KeyMap[ImGuiKey_PageUp] = K_PGUP;
	io.KeyMap[ImGuiKey_PageDown] = K_PGDN;
	io.KeyMap[ImGuiKey_Home] = K_HOME;
	io.KeyMap[ImGuiKey_End] = K_END;
	io.KeyMap[ImGuiKey_Delete] = K_DEL;
	io.KeyMap[ImGuiKey_Backspace] = K_BACKSPACE;
	io.KeyMap[ImGuiKey_Enter] = K_ENTER;
	io.KeyMap[ImGuiKey_Escape] = K_ESCAPE;

	FillCharKeys( io.KeyMap );

	g_DisplaySize.x = windowWidth;
	g_DisplaySize.y = windowHeight;
	io.DisplaySize = g_DisplaySize;

	// RB: FIXME double check
	io.SetClipboardTextFn = SetClipboardText;
	io.GetClipboardTextFn = GetClipboardText;
	io.ClipboardUserData = NULL;

	// SRS - store imgui.ini file in fs_savepath (not in cwd please!)
	static idStr BFG_IniFilename = fileSystem->BuildOSPath( cvarSystem->GetCVarString( "fs_savepath" ), io.IniFilename );
	io.IniFilename = BFG_IniFilename;

	// Setup style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsLight();
	//ImGui::StyleColorsClassic();

	ImGuiStyle* style = &ImGui::GetStyle();
	ImVec4* colors = style->Colors;

	colors[ImGuiCol_Text]                   = ImVec4( 0.92f, 0.92f, 0.92f, 1.00f );
	colors[ImGuiCol_TextDisabled]           = ImVec4( 0.44f, 0.44f, 0.44f, 1.00f );
	colors[ImGuiCol_WindowBg]               = ImVec4( 0.06f, 0.06f, 0.06f, 1.00f );
	colors[ImGuiCol_ChildBg]                = ImVec4( 0.00f, 0.00f, 0.00f, 0.00f );
	colors[ImGuiCol_PopupBg]                = ImVec4( 0.08f, 0.08f, 0.08f, 0.94f );
	colors[ImGuiCol_Border]                 = ImVec4( 0.51f, 0.36f, 0.15f, 1.00f );
	colors[ImGuiCol_BorderShadow]           = ImVec4( 0.00f, 0.00f, 0.00f, 0.00f );
	colors[ImGuiCol_FrameBg]                = ImVec4( 0.11f, 0.11f, 0.11f, 1.00f );
	colors[ImGuiCol_FrameBgHovered]         = ImVec4( 0.51f, 0.36f, 0.15f, 1.00f );
	colors[ImGuiCol_FrameBgActive]          = ImVec4( 0.78f, 0.55f, 0.21f, 1.00f );
	colors[ImGuiCol_TitleBg]                = ImVec4( 0.51f, 0.36f, 0.15f, 1.00f );
	colors[ImGuiCol_TitleBgActive]          = ImVec4( 0.91f, 0.64f, 0.13f, 1.00f );
	colors[ImGuiCol_TitleBgCollapsed]       = ImVec4( 0.00f, 0.00f, 0.00f, 0.51f );
	colors[ImGuiCol_MenuBarBg]              = ImVec4( 0.11f, 0.11f, 0.11f, 1.00f );
	colors[ImGuiCol_ScrollbarBg]            = ImVec4( 0.06f, 0.06f, 0.06f, 0.53f );
	colors[ImGuiCol_ScrollbarGrab]          = ImVec4( 0.21f, 0.21f, 0.21f, 1.00f );
	colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4( 0.47f, 0.47f, 0.47f, 1.00f );
	colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4( 0.81f, 0.83f, 0.81f, 1.00f );
	colors[ImGuiCol_CheckMark]              = ImVec4( 0.78f, 0.55f, 0.21f, 1.00f );
	colors[ImGuiCol_SliderGrab]             = ImVec4( 0.91f, 0.64f, 0.13f, 1.00f );
	colors[ImGuiCol_SliderGrabActive]       = ImVec4( 0.91f, 0.64f, 0.13f, 1.00f );
	colors[ImGuiCol_Button]                 = ImVec4( 0.51f, 0.36f, 0.15f, 1.00f );
	colors[ImGuiCol_ButtonHovered]          = ImVec4( 0.91f, 0.64f, 0.13f, 1.00f );
	colors[ImGuiCol_ButtonActive]           = ImVec4( 0.78f, 0.55f, 0.21f, 1.00f );
	colors[ImGuiCol_Header]                 = ImVec4( 0.51f, 0.36f, 0.15f, 1.00f );
	colors[ImGuiCol_HeaderHovered]          = ImVec4( 0.91f, 0.64f, 0.13f, 1.00f );
	colors[ImGuiCol_HeaderActive]           = ImVec4( 0.93f, 0.65f, 0.14f, 1.00f );
	colors[ImGuiCol_Separator]              = ImVec4( 0.21f, 0.21f, 0.21f, 1.00f );
	colors[ImGuiCol_SeparatorHovered]       = ImVec4( 0.91f, 0.64f, 0.13f, 1.00f );
	colors[ImGuiCol_SeparatorActive]        = ImVec4( 0.78f, 0.55f, 0.21f, 1.00f );
	colors[ImGuiCol_ResizeGrip]             = ImVec4( 0.21f, 0.21f, 0.21f, 1.00f );
	colors[ImGuiCol_ResizeGripHovered]      = ImVec4( 0.91f, 0.64f, 0.13f, 1.00f );
	colors[ImGuiCol_ResizeGripActive]       = ImVec4( 0.78f, 0.55f, 0.21f, 1.00f );
	colors[ImGuiCol_Tab]                    = ImVec4( 0.51f, 0.36f, 0.15f, 1.00f );
	colors[ImGuiCol_TabHovered]             = ImVec4( 0.91f, 0.64f, 0.13f, 1.00f );
	colors[ImGuiCol_TabActive]              = ImVec4( 0.78f, 0.55f, 0.21f, 1.00f );
	colors[ImGuiCol_TabUnfocused]           = ImVec4( 0.07f, 0.10f, 0.15f, 0.97f );
	colors[ImGuiCol_TabUnfocusedActive]     = ImVec4( 0.14f, 0.26f, 0.42f, 1.00f );
	colors[ImGuiCol_TabSelectedOverline]	= ImVec4( 0.99f, 0.59f, 0.20f, 1.00f );
	colors[ImGuiCol_TabDimmed]				= ImVec4( 0.15f, 0.11f, 0.07f, 0.97f );
	colors[ImGuiCol_TabDimmedSelected]		= ImVec4( 0.42f, 0.29f, 0.14f, 1.00f );
	colors[ImGuiCol_TextLink]				= ImVec4( 0.98f, 0.69f, 0.26f, 1.00f );
	colors[ImGuiCol_TextSelectedBg]			= ImVec4( 0.98f, 0.69f, 0.26f, 0.35f );
	colors[ImGuiCol_PlotLines]              = ImVec4( 0.61f, 0.61f, 0.61f, 1.00f );
	colors[ImGuiCol_PlotLinesHovered]       = ImVec4( 1.00f, 0.43f, 0.35f, 1.00f );
	colors[ImGuiCol_PlotHistogram]          = ImVec4( 0.90f, 0.70f, 0.00f, 1.00f );
	colors[ImGuiCol_PlotHistogramHovered]   = ImVec4( 1.00f, 0.60f, 0.00f, 1.00f );
	colors[ImGuiCol_TextSelectedBg]         = ImVec4( 0.26f, 0.59f, 0.98f, 0.35f );
	colors[ImGuiCol_DragDropTarget]         = ImVec4( 1.00f, 1.00f, 0.00f, 0.90f );
	colors[ImGuiCol_DockingPreview]			= ImVec4( 0.98f, 0.76f, 0.26f, 0.70f );
	colors[ImGuiCol_NavHighlight]           = ImVec4( 0.26f, 0.59f, 0.98f, 1.00f );
	colors[ImGuiCol_NavWindowingHighlight]  = ImVec4( 1.00f, 1.00f, 1.00f, 0.70f );
	colors[ImGuiCol_NavWindowingDimBg]      = ImVec4( 0.80f, 0.80f, 0.80f, 0.20f );
	colors[ImGuiCol_NavHighlight]			= ImVec4( 0.98f, 0.69f, 0.26f, 1.00f );
	colors[ImGuiCol_ModalWindowDimBg]       = ImVec4( 0.80f, 0.80f, 0.80f, 0.35f );

	style->FramePadding = ImVec2( 4, 2 );
	style->ItemSpacing = ImVec2( 10, 2 );
	style->IndentSpacing = 12;
	style->ScrollbarSize = 15;

	style->WindowRounding = 2;
	style->FrameRounding = 2;
	style->PopupRounding = 2;
	style->ScrollbarRounding = 3;
	style->GrabRounding = 3;
	style->TabRounding = 3;

	style->DisplaySafeAreaPadding = ImVec2( 4, 4 );

	g_IsInit = true;

	return true;
}

void idImGuiHookLocal::NotifyDisplaySizeChanged( int width, int height )
{
	if( g_DisplaySize.x != width || g_DisplaySize.y != height )
	{
		g_DisplaySize = ImVec2( ( float )width, ( float )height );

		if( IsInitialized() )
		{
			Destroy();
			Init( width, height );

			// reuse the default ImGui font
			const idMaterial* image = declManager->FindMaterial( "_imguiFont" );

			ImGuiIO& io = ImGui::GetIO();

			byte* pixels = NULL;
			io.Fonts->GetTexDataAsRGBA32( &pixels, &width, &height );

			io.Fonts->TexID = ( void* )image;
		}
	}
}

// inject a sys event
bool idImGuiHookLocal::InjectSysEvent( const sysEvent_t* event )
{
	if( IsInitialized() && UseInput() )
	{
		if( event == NULL )
		{
			assert( 0 ); // I think this shouldn't happen
			return false;
		}

		const sysEvent_t& ev = *event;

		switch( ev.evType )
		{
			case SE_KEY:
				return HandleKeyEvent( ev );

			case SE_MOUSE_ABSOLUTE:
				// RB: still allow mouse movement if right mouse button is pressed
				//if( !g_MousePressed[1] )
			{
				g_MousePos.x = ev.evValue;
				g_MousePos.y = ev.evValue2;
				return true;
			}

			case SE_CHAR:
				if( ev.evValue < 0x10000 )
				{
					ImGui::GetIO().AddInputCharacter( ev.evValue );
					return true;
				}
				break;

			case SE_MOUSE_LEAVE:
				g_MousePos = ImVec2( -1.0f, -1.0f );
				return true;

			default:
				break;
		}
	}
	return false;
}

bool idImGuiHookLocal::InjectMouseWheel( int delta )
{
	if( IsInitialized() && UseInput() && delta != 0 )
	{
		g_MouseWheel = ( delta > 0 ) ? 1 : -1;
		return true;
	}
	return false;
}

void idImGuiHookLocal::NewFrame()
{
	if( !g_haveNewFrame && IsInitialized() && ShowWindows() )
	{
		ImGuiIO& io = ImGui::GetIO();

		// Setup display size (every frame to accommodate for window resizing)
		io.DisplaySize = g_DisplaySize;

		// Setup time step
		int	time = Sys_Milliseconds();
		double current_time = time * 0.001;
		io.DeltaTime = g_Time > 0.0 ? ( float )( current_time - g_Time ) : ( float )( 1.0f / 60.0f );

		if( io.DeltaTime <= 0.0F )
		{
			io.DeltaTime = ( 1.0f / 60.0f );
		}

		g_Time = current_time;

		// Setup inputs
		io.MousePos = g_MousePos;

		// If a mouse press event came, always pass it as "mouse held this frame",
		// so we don't miss click-release events that are shorter than 1 frame.
		for( int i = 0; i < 5; ++i )
		{
			io.MouseDown[i] = g_MousePressed[i] || usercmdGen->KeyState( K_MOUSE1 + i ) == 1;
			//g_MousePressed[i] = false;
		}

		io.MouseWheel = g_MouseWheel;
		g_MouseWheel = 0.0f;

		// Hide OS mouse cursor if ImGui is drawing it TODO: hide mousecursor?
		// ShowCursor(io.MouseDrawCursor ? 0 : 1);

		ImGui::GetIO().MouseDrawCursor = UseInput();

		// Start the frame
		ImGui::NewFrame();
		ImGuizmo::BeginFrame();

		g_haveNewFrame = true;
	}
}

bool idImGuiHookLocal::IsReadyToRender()
{
	if( IsInitialized() && ShowWindows() )
	{
		if( !g_haveNewFrame )
		{
			// for screenshots etc, where we didn't go through idCommonLocal::Frame()
			// before idRenderSystemLocal::SwapCommandBuffers_FinishRendering()
			NewFrame();
		}

		return true;
	}

	return false;
}

void idImGuiHookLocal::Render()
{
	if( IsInitialized() && ShowWindows() )
	{
		if( !g_haveNewFrame )
		{
			// for screenshots etc, where we didn't go through idCommonLocal::Frame()
			// before idRenderSystemLocal::SwapCommandBuffers_FinishRendering()
			NewFrame();
		}

		// make dockspace transparent
		static ImGuiDockNodeFlags dockspaceFlags = ImGuiDockNodeFlags_PassthruCentralNode;
		ImGui::DockSpaceOverViewport( NULL, NULL, dockspaceFlags, NULL );

		if( imguiTools )
		{
			imguiTools->DrawToolWindows();
		}

		if( imgui_showDemoWindow.GetBool() )
		{
			ImGui::ShowDemoWindow();
		}

		ImGui::Render();
		idRenderBackend::ImGui_RenderDrawLists( ImGui::GetDrawData() );
		g_haveNewFrame = false;
	}
}

void idImGuiHookLocal::Destroy()
{
	if( IsInitialized() )
	{
		common->Printf( "Shutting down ImGui\n" );

		common->Printf( "delete imguiTools;\n" );
		imguiTools = NULL;

		ImGui::DestroyContext();
		g_IsInit = false;
		g_haveNewFrame = false;
	}
}

bool idImGuiHookLocal::IsInitialized()
{
	// checks if imgui is up and running
	return g_IsInit;
}

bool idImGuiHookLocal::HandleKeyEvent( const sysEvent_t& keyEvent )
{
	idassert( keyEvent.evType == SE_KEY );

	keyNum_t keyNum = static_cast<keyNum_t>( keyEvent.evValue );
	bool pressed = keyEvent.evValue2 > 0;

	ImGuiIO& io = ImGui::GetIO();

	if( keyNum < K_JOY1 )
	{
		// keyboard input as direct input scancodes
		io.KeysDown[keyNum] = pressed;

		io.KeyAlt = usercmdGen->KeyState( K_LALT ) == 1 || usercmdGen->KeyState( K_RALT ) == 1;
		io.KeyCtrl = usercmdGen->KeyState( K_LCTRL ) == 1 || usercmdGen->KeyState( K_RCTRL ) == 1;
		io.KeyShift = usercmdGen->KeyState( K_LSHIFT ) == 1 || usercmdGen->KeyState( K_RSHIFT ) == 1;

		return true;
	}
	else if( keyNum >= K_MOUSE1 && keyNum <= K_MOUSE5 )
	{
		int buttonIdx = keyNum - K_MOUSE1;

		// K_MOUSE* are contiguous, so they can be used as indexes into imgui's
		// g_MousePressed[] - imgui even uses the same order (left, right, middle, X1, X2)
		g_MousePressed[buttonIdx] = pressed;

		return true; // let's pretend we also handle mouse up events
	}

	return false;
}

void idImGuiHookLocal::FillCharKeys( int* keyMap )
{
	// set scancodes as default values in case the real keys aren't found below
	keyMap[ImGuiKey_A] = K_A;
	keyMap[ImGuiKey_C] = K_C;
	keyMap[ImGuiKey_V] = K_V;
	keyMap[ImGuiKey_X] = K_X;
	keyMap[ImGuiKey_Y] = K_Y;
	keyMap[ImGuiKey_Z] = K_Z;

	// try all probable keys for whether they're ImGuiKey_A/C/V/X/Y/Z
	for( int k = K_1; k < K_RSHIFT; ++k )
	{
		const char* kn = idKeyInput::LocalizedKeyName( ( keyNum_t )k );
		if( kn[0] == '\0' || kn[1] != '\0' || kn[0] == '?' )
		{
			// if the key wasn't found or the name has more than one char,
			// it's not what we're looking for.
			continue;
		}
		switch( kn[0] )
		{
			case 'a': // fall-through
			case 'A':
				keyMap [ImGuiKey_A] = k;
				break;
			case 'c': // fall-through
			case 'C':
				keyMap [ImGuiKey_C] = k;
				break;

			case 'v': // fall-through
			case 'V':
				keyMap [ImGuiKey_V] = k;
				break;

			case 'x': // fall-through
			case 'X':
				keyMap [ImGuiKey_X] = k;
				break;

			case 'y': // fall-through
			case 'Y':
				keyMap [ImGuiKey_Y] = k;
				break;

			case 'z': // fall-through
			case 'Z':
				keyMap [ImGuiKey_Z] = k;
				break;
		}
	}
}

const char* idImGuiHookLocal::GetClipboardText( void* )
{
	char* txt = Sys_GetClipboardData();
	if( txt == NULL )
	{
		return NULL;
	}

	static idStr clipboardBuf;
	clipboardBuf = txt;

	Mem_Free( txt );

	return clipboardBuf.c_str();
}

void idImGuiHookLocal::SetClipboardText( void*, const char* text )
{
	Sys_SetClipboardData( text );
}

bool idImGuiHookLocal::ShowWindows()
{
	return ( imguiTools && imguiTools->AreEditorsActive() || imgui_showDemoWindow.GetBool() || com_showFPS.GetInteger() > 1 );
}

bool idImGuiHookLocal::UseInput()
{
	return imguiTools && imguiTools->ReleaseMouseForTools() || imgui_showDemoWindow.GetBool();
}

namespace ImGuiHook
{
namespace
{
static inline ImVec4 ImLerp( const ImVec4& a, const ImVec4& b, float t )
{
	return ImVec4( a.x + ( b.x - a.x ) * t, a.y + ( b.y - a.y ) * t, a.z + ( b.z - a.z ) * t, a.w + ( b.w - a.w ) * t );
}

static inline ImVec4 operator*( const ImVec4& lhs, const ImVec4& rhs )
{
	return ImVec4( lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z, lhs.w * rhs.w );
}

static inline ImVec4  operator+( const ImVec4& lhs, const ImVec4& rhs )
{
	return ImVec4( lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z, lhs.w + rhs.w );
}
} //anon namespace

} //namespace ImGuiHook
