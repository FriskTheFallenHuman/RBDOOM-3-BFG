/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 2016 Daniel Gibson
Copyright (C) 2022 Stephen Pridham

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

#include "../imgui/BFGimgui.h"
#include "../idlib/CmdArgs.h"

#include "afeditor/AfEditor.h"
#include "lighteditor/LightEditor.h"

class idImGuiToolsLocal final : public idImGuiTools
{
public:
	idImGuiToolsLocal();

	virtual void	InitTool( const toolFlag_t tool, const idDict* dict, idEntity* entity );
	virtual bool	AreEditorsActive();
	virtual void	DrawToolWindows();
	virtual void	SetReleaseToolMouse( bool doRelease );
	virtual bool	ReleaseMouseForTools();

private:
	bool g_releaseMouse;
};

idImGuiToolsLocal	imguiToolsLocal;
idImGuiTools* imguiTools = &imguiToolsLocal;

static void LightEditorInit( const idDict* dict, idEntity* ent )
{
	if( dict == NULL || ent == NULL )
	{
		return;
	}

	// NOTE: we can't access idEntity (it's just a declaration), because it should
	// be game/mod specific. but we can at least check the spawnclass from the dict.
	idassert( idStr::Icmp( dict->GetString( "spawnclass" ), "idLight" ) == 0
			  && "LightEditorInit() must only be called with light entities or NULL!" );


	ImGuiTools::LightEditor::Instance().ShowIt( true );
	imguiTools->SetReleaseToolMouse( true );

	ImGuiTools::LightEditor::ReInit( dict, ent );
}

static void AFEditorInit( const idDict* dict )
{
	ImGuiTools::AfEditor::Instance().ShowIt( true );
	imguiTools->SetReleaseToolMouse( true );
}

idImGuiToolsLocal::idImGuiToolsLocal() : g_releaseMouse( false )
{
}

void idImGuiToolsLocal::InitTool( const toolFlag_t tool, const idDict* dict, idEntity* entity )
{
	if( tool & EDITOR_LIGHT )
	{
		LightEditorInit( dict, entity );
	}
	else if( tool & EDITOR_AF )
	{
		AFEditorInit( dict );
	}
}

bool idImGuiToolsLocal::AreEditorsActive()
{
	return cvarSystem->GetCVarInteger( "g_editEntityMode" ) > 0 || com_editors != 0;
}

void idImGuiToolsLocal::SetReleaseToolMouse( bool doRelease )
{
	g_releaseMouse = doRelease;
}

void idImGuiToolsLocal::DrawToolWindows()
{
	if( ImGuiTools::LightEditor::Instance().IsShown() )
	{
		ImGuiTools::LightEditor::Instance().Draw();
	}
	else if( ImGuiTools::AfEditor::Instance().IsShown() )
	{
		ImGuiTools::AfEditor::Instance().Draw();
	}
}

bool idImGuiToolsLocal::ReleaseMouseForTools()
{
	return AreEditorsActive() && g_releaseMouse;
}