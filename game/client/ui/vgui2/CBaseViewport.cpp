//TODO: resolve conflicts - Solokiller
#define NO_CLAMP_DEF
#include "hud.h"
#include "cl_util.h"
#include "demo_api.h"

//TODO: Need to merge the vector classes - Solokiller
#define VECTOR2D_H

#include <vgui/IPanel.h>
#include <vgui/ISurface.h>

#include "CBackGroundPanel.h"
#include "IViewportPanel.h"
#include "ViewportPanelNames.h"

#include "ui/CClientVGUI.h"

#include "CBaseViewport.h"

CBaseViewport* g_pViewport = nullptr;

CBaseViewport::CBaseViewport()
	: BaseClass( nullptr, "ViewPort" )
{
	g_pViewport = this;

	SetKeyBoardInputEnabled( false );
	SetMouseInputEnabled( false );

	vgui2::HScheme scheme = vgui2::scheme()->LoadSchemeFromFile( "resource/ClientScheme.res", "ClientScheme" );

	SetScheme( scheme );
	SetProportional( true );

	//TODO: add the animation controller? - Solokiller
}

CBaseViewport::~CBaseViewport()
{
	if( m_pBackGround )
	{
		m_pBackGround->MarkForDeletion();
		m_pBackGround = nullptr;
	}

	RemoveAllPanels();
}

void CBaseViewport::Initialize( CreateInterfaceFn* pFactories, int iNumFactories )
{
}

void CBaseViewport::Start()
{
	// recreate all the default panels
	RemoveAllPanels();

	m_pBackGround = new CBackGroundPanel( nullptr );

	m_pBackGround->SetZPos( -20 ); // send it to the back 
	m_pBackGround->SetVisible( false );

	CreateDefaultPanels();

	vgui2::ipanel()->MoveToBack( m_pBackGround->GetVPanel() ); // really send it to the back 
}

vgui2::Panel* g_pPanel = nullptr;
vgui2::Panel* g_pPanel2 = nullptr;

void CBaseViewport::SetParent( vgui2::VPANEL parent )
{
	m_pBackGround->SetParent( parent );

	g_pPanel = new vgui2::Panel();

	g_pPanel->SetParent( parent );

	g_pPanel->SetSize( 200, 200 );
	g_pPanel->SetPos( 10, 10 );

	g_pPanel->SetVisible( true );

	g_pPanel2 = new vgui2::Panel();

	g_pPanel2->SetParent( parent );

	g_pPanel2->SetSize( 100, 100 );
	g_pPanel2->SetPos( 10, 150 );

	g_pPanel2->SetVisible( true );
}

int CBaseViewport::UseVGUI1()
{
	return true;
}

void CBaseViewport::HideScoreBoard()
{
}

void CBaseViewport::HideAllVGUIMenu()
{
	//vgui2::surface()->UnlockCursor();
}

void CBaseViewport::ActivateClientUI()
{
}

void CBaseViewport::HideClientUI()
{
}

void CBaseViewport::Shutdown()
{
}

void CBaseViewport::OnThink()
{
	// Clear our active panel pointer if the panel has made
	// itself invisible. Need this so we don't bring up dead panels
	// if they are stored as the last active panel
	if( m_pActivePanel && !m_pActivePanel->IsVisible() )
	{
		if( m_pLastActivePanel )
		{
			m_pActivePanel = m_pLastActivePanel;
			ShowPanel( m_pActivePanel, true );
			m_pLastActivePanel = NULL;
		}
		else
			m_pActivePanel = NULL;
	}

	//m_pAnimController->UpdateAnimations( gEngfuncs.GetClientTime() );

	// check the auto-reload cvar
	//m_pAnimController->SetAutoReloadScript( hud_autoreloadscript.GetBool() );

	auto count = m_Panels.Count();

	for( decltype( count ) i = 0; i< count; ++i )
	{
		auto panel = m_Panels[ i ];
		if( panel->NeedsUpdate() && panel->IsVisible() )
		{
			panel->Update();
		}
	}

	int w, h;
	vgui2::surface()->GetScreenSize( w, h );

	if( m_OldSize[ 0 ] != w || m_OldSize[ 1 ] != h )
	{
		m_OldSize[ 0 ] = w;
		m_OldSize[ 1 ] = h;
		/*g_pClientMode->*/Layout();
	}

	BaseClass::OnThink();
}

void CBaseViewport::Layout()
{
	vgui2::VPANEL pRoot;
	int wide, tall;

	// Make the viewport fill the root panel.
	if( ( pRoot = clientVGUI()->GetRootPanel() ) != NULL )
	{
		vgui2::ipanel()->GetSize( pRoot, wide, tall );

		const bool changed = wide != m_nRootSize[ 0 ] || tall != m_nRootSize[ 1 ];
		m_nRootSize[ 0 ] = wide;
		m_nRootSize[ 1 ] = tall;

		SetBounds( 0, 0, wide, tall );
		if( changed )
		{
			//TODO - Solokiller
			//ReloadScheme();
		}
	}
}

void CBaseViewport::CreateDefaultPanels()
{
}

void CBaseViewport::UpdateAllPanels()
{
	auto count = m_Panels.Count();

	for( decltype( count ) iIndex = 0; iIndex < count; ++iIndex )
	{
		auto pPanel = m_Panels[ iIndex ];

		if( pPanel->IsVisible() )
		{
			pPanel->Update();
		}
	}
}

IViewportPanel* CBaseViewport::CreatePanelByName( const char* pszName )
{
	IViewportPanel* pPanel = nullptr;

	/*
	if( Q_strcmp( "name", pszName ) == 0 )
	{
		pPanel = new CClassName( this );
	}
	*/

	return pPanel;
}

IViewportPanel* CBaseViewport::FindPanelByName( const char* pszName )
{
	auto count = m_Panels.Count();

	for( decltype( count ) iIndex = 0; iIndex < count; ++iIndex )
	{
		if( Q_strcmp( m_Panels[ iIndex ]->GetName(), pszName ) == 0 )
			return m_Panels[ iIndex ];
	}

	return nullptr;
}

bool CBaseViewport::AddNewPanel( IViewportPanel* pPanel )
{
	if( !pPanel )
	{
		Con_Printf( "CBaseViewport::AddNewPanel: Null panel!\n" );
		return false;
	}

	if( FindPanelByName( pPanel->GetName() ) )
	{
		Con_Printf( "CBaseViewport::AddNewPanel: A panel with name '%s' already exists.\n", pPanel->GetName() );
		return false;
	}

	m_Panels.AddToTail( pPanel );
	pPanel->SetParent( GetVPanel() );

	return true;
}

void CBaseViewport::ShowPanel( const char* pszName, const bool bState )
{
	if( Q_strcmp( VIEWPORT_PANEL_ALL, pszName ) == 0 )
	{
		auto count = m_Panels.Count();

		for( decltype( count ) iIndex = 0; iIndex < count; ++iIndex )
		{
			ShowPanel( m_Panels[ iIndex ], bState );
		}

		return;
	}

	IViewportPanel* pPanel = nullptr;

	if( Q_strcmp( VIEWPORT_PANEL_ACTIVE, pszName ) == 0 )
	{
		pPanel = m_pActivePanel;
	}
	else
	{
		pPanel = FindPanelByName( pszName );
	}

	if( !pPanel )
		return;

	ShowPanel( pPanel, bState );
}

void CBaseViewport::ShowPanel( IViewportPanel* pPanel, const bool bState )
{
	if( bState )
	{
		// if this is an 'active' panel, deactivate old active panel
		if( pPanel->HasInputElements() )
		{
			// don't show input panels during normal demo playback
			if( gEngfuncs.pDemoAPI->IsPlayingback() && !gEngfuncs.IsSpectateOnly() )
				return;

			if( ( m_pActivePanel != nullptr ) && ( m_pActivePanel != pPanel ) )
			{
				// store a pointer to the currently active panel
				// so we can restore it later
				m_pLastActivePanel = m_pActivePanel;
				m_pActivePanel->ShowPanel( false );
			}

			m_pActivePanel = pPanel;
		}
	}
	else
	{
		// if this is our current active panel
		// update m_pActivePanel pointer
		if( m_pActivePanel == pPanel )
		{
			m_pActivePanel = nullptr;
		}

		// restore the previous active panel if it exists
		if( m_pLastActivePanel )
		{
			m_pActivePanel = m_pLastActivePanel;
			m_pLastActivePanel = nullptr;

			m_pActivePanel->ShowPanel( true );
		}
	}

	// just show/hide panel
	pPanel->ShowPanel( bState );

	UpdateAllPanels(); // let other panels rearrange
}

void CBaseViewport::RemoveAllPanels()
{
	auto count = m_Panels.Count();

	for( decltype( count ) iIndex = 0; iIndex < count; ++iIndex )
	{
		auto vPanel = m_Panels[ iIndex ]->GetVPanel();

		vgui2::ipanel()->DeletePanel( vPanel );
	}

	if( m_pBackGround )
	{
		m_pBackGround->MarkForDeletion();
		m_pBackGround = nullptr;
	}

	m_Panels.Purge();

	m_pActivePanel = nullptr;
	m_pLastActivePanel = nullptr;
}

IViewportPanel* CBaseViewport::GetActivePanel()
{
	return m_pActivePanel;
}

IViewportPanel* CBaseViewport::GetLastActivePanel()
{
	return m_pLastActivePanel;
}

bool CBaseViewport::IsBackGroundVisible() const
{
	return m_pBackGround->IsVisible();
}

void CBaseViewport::ShowBackGround( const bool bState )
{
	m_pBackGround->SetVisible( bState );
}
