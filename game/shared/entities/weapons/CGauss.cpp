/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "entities/NPCs/Monsters.h"
#include "Weapons.h"
#include "CGauss.h"
#include "nodes/Nodes.h"
#include "CBasePlayer.h"
#include "entities/CSoundEnt.h"
#include "shake.h"
#include "gamerules/GameRules.h"

#ifdef SERVER_DLL
BEGIN_DATADESC( CGauss )
	DEFINE_FIELD( m_InAttack, FIELD_INTEGER ),
	//DEFINE_FIELD( m_flStartCharge, FIELD_TIME ),
	//DEFINE_FIELD( m_flPlayAftershock, FIELD_TIME ),
	//DEFINE_FIELD( m_flNextAmmoBurn, FIELD_TIME ),
	DEFINE_FIELD( m_fPrimaryFire, FIELD_BOOLEAN ),
END_DATADESC()
#endif

LINK_ENTITY_TO_CLASS( weapon_gauss, CGauss );

CGauss::CGauss()
	: BaseClass( WEAPON_GAUSS )
{
}

float CGauss::GetFullChargeTime()
{
	if ( bIsMultiplayer() )
	{
		return 1.5;
	}

	return 4;
}

#ifdef CLIENT_DLL
extern bool g_brunninggausspred;
#endif

void CGauss::Spawn()
{
	Precache( );
	SetModel( "models/w_gauss.mdl");

	FallInit();// get ready to fall down.
}


void CGauss::Precache()
{
	BaseClass::Precache();

	PRECACHE_MODEL("models/w_gauss.mdl");
	PRECACHE_MODEL("models/v_gauss.mdl");
	PRECACHE_MODEL("models/p_gauss.mdl");

	PRECACHE_SOUND("items/9mmclip1.wav");

	PRECACHE_SOUND("weapons/gauss2.wav");
	PRECACHE_SOUND("weapons/electro4.wav");
	PRECACHE_SOUND("weapons/electro5.wav");
	PRECACHE_SOUND("weapons/electro6.wav");
	PRECACHE_SOUND("ambience/pulsemachine.wav");
	
	m_iGlow = PRECACHE_MODEL( "sprites/hotglow.spr" );
	m_iBalls = PRECACHE_MODEL( "sprites/hotglow.spr" );
	m_iBeam = PRECACHE_MODEL( "sprites/smoke.spr" );

	m_usGaussFire = PRECACHE_EVENT( 1, "events/gauss.sc" );
	m_usGaussSpin = PRECACHE_EVENT( 1, "events/gaussspin.sc" );
}

bool CGauss::AddToPlayer( CBasePlayer *pPlayer )
{
	if ( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, pPlayer );
			WRITE_BYTE( m_iId );
		MESSAGE_END();
		return true;
	}
	return false;
}

bool CGauss::IsUseable()
{
	//Currently charging, allow the player to fire it first. - Solokiller
	return BaseClass::IsUseable() || m_InAttack != AttackState::NOT_ATTACKING;
}

bool CGauss::Deploy()
{
	m_pPlayer->m_flPlayAftershock = 0.0;
	return DefaultDeploy( "models/v_gauss.mdl", "models/p_gauss.mdl", GAUSS_DRAW, "gauss" );
}

void CGauss::Holster()
{
	PLAYBACK_EVENT_FULL( FEV_RELIABLE | FEV_GLOBAL, m_pPlayer->edict(), m_usGaussFire, 0.01, m_pPlayer->GetAbsOrigin(), m_pPlayer->pev->angles, 0.0, 0.0, 0, 0, 0, 1 );
	
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	
	SendWeaponAnim( GAUSS_HOLSTER );
	m_InAttack = AttackState::NOT_ATTACKING;
}


void CGauss::PrimaryAttack()
{
	// don't fire underwater
	if ( m_pPlayer->GetWaterLevel() == WATERLEVEL_HEAD )
	{
		PlayEmptySound( );
		m_flNextSecondaryAttack = m_flNextPrimaryAttack = GetNextAttackDelay(0.15);
		return;
	}

	if ( m_pPlayer->m_rgAmmo[ PrimaryAmmoIndex() ] < AMMO_PER_PRIMARY_SHOT )
	{
		PlayEmptySound( );
		m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
		return;
	}

	m_pPlayer->m_iWeaponVolume = GAUSS_PRIMARY_FIRE_VOLUME;
	m_fPrimaryFire = true;

	m_pPlayer->m_rgAmmo[ PrimaryAmmoIndex() ] -= AMMO_PER_PRIMARY_SHOT;

	StartFire();
	m_InAttack = AttackState::NOT_ATTACKING;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0;
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.2;
}

void CGauss::SecondaryAttack()
{
	// don't fire underwater
	if ( m_pPlayer->GetWaterLevel() == WATERLEVEL_HEAD )
	{
		if ( m_InAttack != AttackState::NOT_ATTACKING )
		{
			EMIT_SOUND_DYN( m_pPlayer, CHAN_WEAPON, "weapons/electro4.wav", 1.0, ATTN_NORM, 0, 80 + RANDOM_LONG(0,0x3f));
			SendWeaponAnim( GAUSS_IDLE );
			m_InAttack = AttackState::NOT_ATTACKING;
		}
		else
		{
			PlayEmptySound( );
		}

		m_flNextSecondaryAttack = m_flNextPrimaryAttack = GetNextAttackDelay(0.5);
		return;
	}

	if ( m_InAttack == AttackState::NOT_ATTACKING )
	{
		if ( m_pPlayer->m_rgAmmo[ PrimaryAmmoIndex() ] <= 0 )
		{
			EMIT_SOUND( m_pPlayer, CHAN_WEAPON, "weapons/357_cock1.wav", 0.8, ATTN_NORM);
			m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
			return;
		}

		m_fPrimaryFire = false;

		m_pPlayer->m_rgAmmo[ PrimaryAmmoIndex() ]--;// take one ammo just to start the spin
		m_pPlayer->m_flNextAmmoBurn = UTIL_WeaponTimeBase();

		// spin up
		m_pPlayer->m_iWeaponVolume = GAUSS_PRIMARY_CHARGE_VOLUME;
		
		SendWeaponAnim( GAUSS_SPINUP );
		m_InAttack = AttackState::CHARGING_START;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.5;
		m_pPlayer->m_flStartCharge = gpGlobals->time;
		m_pPlayer->m_flAmmoStartCharge = UTIL_WeaponTimeBase() + GetFullChargeTime();

		PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usGaussSpin, 0.0, g_vecZero, g_vecZero, 0.0, 0.0, 110, 0, 0, 0 );

		m_iSoundState = SND_CHANGE_PITCH;
	}
	else if ( m_InAttack == AttackState::CHARGING_START )
	{
		if (m_flTimeWeaponIdle < UTIL_WeaponTimeBase())
		{
			SendWeaponAnim( GAUSS_SPIN );
			m_InAttack = AttackState::CHARGING;
		}
	}
	else
	{
		//Moved to before the ammo burn.
		//Because we drained 1 in AttackState::NOT_ATTACKING, then 1 again now before checking if we're out of ammo,
		//this resuled in the player having -1 ammo, which in turn caused CanDeploy to think it could be deployed.
		//This will need to be fixed further down the line by preventing negative ammo unless explicitly required (infinite ammo?),
		//But this check will prevent the problem for now. - Solokiller
		//TODO: investigate further.
		if( m_pPlayer->m_rgAmmo[ PrimaryAmmoIndex() ] <= 0 )
		{
			// out of ammo! force the gun to fire
			StartFire();
			m_InAttack = AttackState::NOT_ATTACKING;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0;
			//Need to set m_flNextPrimaryAttack so the weapon gets a chance to complete its secondary fire animation. - Solokiller
			m_flNextPrimaryAttack = m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1;
			return;
		}

		// during the charging process, eat one bit of ammo every once in a while
		if ( UTIL_WeaponTimeBase() >= m_pPlayer->m_flNextAmmoBurn && m_pPlayer->m_flNextAmmoBurn != 1000 )
		{
			if ( bIsMultiplayer() )
			{
				m_pPlayer->m_rgAmmo[ PrimaryAmmoIndex() ]--;
				m_pPlayer->m_flNextAmmoBurn = UTIL_WeaponTimeBase() + 0.1;
			}
			else
			{
				m_pPlayer->m_rgAmmo[ PrimaryAmmoIndex() ]--;
				m_pPlayer->m_flNextAmmoBurn = UTIL_WeaponTimeBase() + 0.3;
			}
		}
		
		if ( UTIL_WeaponTimeBase() >= m_pPlayer->m_flAmmoStartCharge )
		{
			// don't eat any more ammo after gun is fully charged.
			m_pPlayer->m_flNextAmmoBurn = 1000;
		}

		int pitch = ( gpGlobals->time - m_pPlayer->m_flStartCharge ) * ( 150 / GetFullChargeTime() ) + 100;
		if ( pitch > 250 ) 
			 pitch = 250;
		
		// ALERT( at_console, "%d %d %d\n", m_fInAttack, m_iSoundState, pitch );

		if ( m_iSoundState == 0 )
			ALERT( at_console, "sound state %d\n", m_iSoundState );

		PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usGaussSpin, 0.0, g_vecZero, g_vecZero, 0.0, 0.0, pitch, 0, ( m_iSoundState == SND_CHANGE_PITCH ) ? 1 : 0, 0 );

		m_iSoundState = SND_CHANGE_PITCH; // hack for going through level transitions

		m_pPlayer->m_iWeaponVolume = GAUSS_PRIMARY_CHARGE_VOLUME;
		
		// m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.1;
		if ( m_pPlayer->m_flStartCharge < gpGlobals->time - 10 )
		{
			// Player charged up too long. Zap him.
			EMIT_SOUND_DYN( m_pPlayer, CHAN_WEAPON, "weapons/electro4.wav", 1.0, ATTN_NORM, 0, 80 + RANDOM_LONG(0,0x3f));
			EMIT_SOUND_DYN( m_pPlayer, CHAN_ITEM,   "weapons/electro6.wav", 1.0, ATTN_NORM, 0, 75 + RANDOM_LONG(0,0x3f));
			
			m_InAttack = AttackState::NOT_ATTACKING;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0;
			m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0;
				
#ifndef CLIENT_DLL
			m_pPlayer->TakeDamage( CWorld::GetInstance(), CWorld::GetInstance(), 50, DMG_SHOCK );
			UTIL_ScreenFade( m_pPlayer, Vector(255,128,0), 2, 0.5, 128, FFADE_IN );
#endif
			SendWeaponAnim( GAUSS_IDLE );
			
			// Player may have been killed and this weapon dropped, don't execute any more code after this!
			return;
		}
	}
}

//=========================================================
// StartFire- since all of this code has to run and then 
// call Fire(), it was easier at this point to rip it out 
// of weaponidle() and make its own function then to try to
// merge this into Fire(), which has some identical variable names 
//=========================================================
void CGauss::StartFire()
{
	float flDamage;
	
	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );
	Vector vecAiming = gpGlobals->v_forward;
	Vector vecSrc = m_pPlayer->GetGunPosition( ); // + gpGlobals->v_up * -8 + gpGlobals->v_right * 8;
	
	if ( gpGlobals->time - m_pPlayer->m_flStartCharge > GetFullChargeTime() )
	{
		flDamage = 200;
	}
	else
	{
		flDamage = 200 * (( gpGlobals->time - m_pPlayer->m_flStartCharge) / GetFullChargeTime() );
	}

	if ( m_fPrimaryFire )
	{
		// fixed damage on primary attack
#ifdef CLIENT_DLL
		flDamage = 20;
#else 
		flDamage = gSkillData.GetPlrDmgGauss();
#endif
	}

	//m_InAttack is never 3, so this check is always true. - Solokiller
	//if ( m_InAttack != 3)
	{
		//ALERT ( at_console, "Time:%f Damage:%f\n", gpGlobals->time - m_pPlayer->m_flStartCharge, flDamage );

#ifndef CLIENT_DLL
		float flZVel = m_pPlayer->pev->velocity.z;

		if ( !m_fPrimaryFire )
		{
			m_pPlayer->pev->velocity = m_pPlayer->pev->velocity - gpGlobals->v_forward * flDamage * 5;
		}

		if ( !g_pGameRules->IsMultiplayer() )

		{
			// in deathmatch, gauss can pop you up into the air. Not in single play.
			m_pPlayer->pev->velocity.z = flZVel;
		}
#endif
		// player "shoot" animation
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
	}

	// time until aftershock 'static discharge' sound
	m_pPlayer->m_flPlayAftershock = gpGlobals->time + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0.3, 0.8 );

	Fire( vecSrc, vecAiming, flDamage );
}

void CGauss::Fire( Vector vecOrigSrc, Vector vecDir, float flDamage )
{
	m_pPlayer->m_iWeaponVolume = GAUSS_PRIMARY_FIRE_VOLUME;

	Vector vecSrc = vecOrigSrc;
	Vector vecDest = vecSrc + vecDir * 8192;
	edict_t		*pentIgnore;
	TraceResult tr, beam_tr;
	float flMaxFrac = 1.0;
	int	nTotal = 0;
	int fHasPunched = 0;
	int fFirstBeam = 1;
	int	nMaxHits = 10;

	pentIgnore = ENT( m_pPlayer->pev );

#ifdef CLIENT_DLL
	if ( !m_fPrimaryFire )
		g_brunninggausspred = true;
#endif
	
	// The main firing event is sent unreliably so it won't be delayed.
	PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usGaussFire, 0.0, m_pPlayer->GetAbsOrigin(), m_pPlayer->pev->angles, flDamage, 0.0, 0, 0, m_fPrimaryFire ? 1 : 0, 0 );

	// This reliable event is used to stop the spinning sound
	// It's delayed by a fraction of second to make sure it is delayed by 1 frame on the client
	// It's sent reliably anyway, which could lead to other delays

	PLAYBACK_EVENT_FULL( FEV_NOTHOST | FEV_RELIABLE, m_pPlayer->edict(), m_usGaussFire, 0.01, m_pPlayer->GetAbsOrigin(), m_pPlayer->pev->angles, 0.0, 0.0, 0, 0, 0, 1 );

	
	/*ALERT( at_console, "%f %f %f\n%f %f %f\n", 
		vecSrc.x, vecSrc.y, vecSrc.z, 
		vecDest.x, vecDest.y, vecDest.z );*/
	

//	ALERT( at_console, "%f %f\n", tr.flFraction, flMaxFrac );

#ifndef CLIENT_DLL
	while (flDamage > 10 && nMaxHits > 0)
	{
		nMaxHits--;

		// ALERT( at_console, "." );
		UTIL_TraceLine(vecSrc, vecDest, dont_ignore_monsters, pentIgnore, &tr);

		if (tr.fAllSolid)
			break;

		CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

		if (pEntity == NULL)
			break;

		if ( fFirstBeam )
		{
			m_pPlayer->pev->effects |= EF_MUZZLEFLASH;
			fFirstBeam = 0;
	
			nTotal += 26;
		}
		
		if (pEntity->pev->takedamage)
		{
			g_MultiDamage.Clear();
			pEntity->TraceAttack( CTakeDamageInfo( m_pPlayer, flDamage, DMG_BULLET ), vecDir, &tr );
			g_MultiDamage.ApplyMultiDamage( m_pPlayer, m_pPlayer );
		}

		if ( pEntity->ReflectGauss() )
		{
			float n;

			pentIgnore = NULL;

			n = -DotProduct(tr.vecPlaneNormal, vecDir);

			if (n < 0.5) // 60 degrees
			{
				// ALERT( at_console, "reflect %f\n", n );
				// reflect
				Vector r;
			
				r = 2.0 * tr.vecPlaneNormal * n + vecDir;
				flMaxFrac = flMaxFrac - tr.flFraction;
				vecDir = r;
				vecSrc = tr.vecEndPos + vecDir * 8;
				vecDest = vecSrc + vecDir * 8192;

				// explode a bit
				m_pPlayer->RadiusDamage( tr.vecEndPos, this, m_pPlayer, flDamage * n, EntityClassifications().GetNoneId(), DMG_BLAST );

				nTotal += 34;
				
				// lose energy
				if (n == 0) n = 0.1;
				flDamage = flDamage * (1 - n);
			}
			else
			{
				nTotal += 13;

				// limit it to one hole punch
				if (fHasPunched)
					break;
				fHasPunched = 1;

				// try punching through wall if secondary attack (primary is incapable of breaking through)
				if ( !m_fPrimaryFire )
				{
					UTIL_TraceLine( tr.vecEndPos + vecDir * 8, vecDest, dont_ignore_monsters, pentIgnore, &beam_tr);
					if (!beam_tr.fAllSolid)
					{
						// trace backwards to find exit point
						UTIL_TraceLine( beam_tr.vecEndPos, tr.vecEndPos, dont_ignore_monsters, pentIgnore, &beam_tr);

						float n = (beam_tr.vecEndPos - tr.vecEndPos).Length( );

						if (n < flDamage)
						{
							if (n == 0) n = 1;
							flDamage -= n;

							// ALERT( at_console, "punch %f\n", n );
							nTotal += 21;

							// exit blast damage
							//m_pPlayer->RadiusDamage( beam_tr.vecEndPos + vecDir * 8, pev, m_pPlayer->pev, flDamage, EntityClassifications().GetNoneId(), DMG_BLAST );
							float damage_radius;
							

							if ( g_pGameRules->IsMultiplayer() )
							{
								damage_radius = flDamage * 1.75;  // Old code == 2.5
							}
							else
							{
								damage_radius = flDamage * 2.5;
							}

							::RadiusDamage( beam_tr.vecEndPos + vecDir * 8, CTakeDamageInfo( this, m_pPlayer, flDamage, DMG_BLAST ), damage_radius, EntityClassifications().GetNoneId() );

							CSoundEnt::InsertSound ( bits_SOUND_COMBAT, GetAbsOrigin(), NORMAL_EXPLOSION_VOLUME, 3.0 );

							nTotal += 53;

							vecSrc = beam_tr.vecEndPos + vecDir;
						}
					}
					else
					{
						 //ALERT( at_console, "blocked %f\n", n );
						flDamage = 0;
					}
				}
				else
				{
					//ALERT( at_console, "blocked solid\n" );
					
					flDamage = 0;
				}

			}
		}
		else
		{
			vecSrc = tr.vecEndPos + vecDir;
			pentIgnore = ENT( pEntity->pev );
		}
	}
#endif
	// ALERT( at_console, "%d bytes\n", nTotal );
}

void CGauss::WeaponIdle()
{
	ResetEmptySound( );

	// play aftershock static discharge
	if ( m_pPlayer->m_flPlayAftershock && m_pPlayer->m_flPlayAftershock < gpGlobals->time )
	{
		switch (RANDOM_LONG(0,3))
		{
		case 0:	EMIT_SOUND( m_pPlayer, CHAN_WEAPON, "weapons/electro4.wav", RANDOM_FLOAT(0.7, 0.8), ATTN_NORM); break;
		case 1:	EMIT_SOUND( m_pPlayer, CHAN_WEAPON, "weapons/electro5.wav", RANDOM_FLOAT(0.7, 0.8), ATTN_NORM); break;
		case 2:	EMIT_SOUND( m_pPlayer, CHAN_WEAPON, "weapons/electro6.wav", RANDOM_FLOAT(0.7, 0.8), ATTN_NORM); break;
		case 3:	break; // no sound
		}
		m_pPlayer->m_flPlayAftershock = 0.0;
	}

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	if ( m_InAttack != AttackState::NOT_ATTACKING )
	{
		StartFire();
		m_InAttack = AttackState::NOT_ATTACKING;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.0;

		//Need to set m_flNextPrimaryAttack so the weapon gets a chance to complete its secondary fire animation. - Solokiller
		if( m_pPlayer->m_rgAmmo[ PrimaryAmmoIndex() ] <= 0 )
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.5;
	}
	else
	{
		int iAnim;
		float flRand = RANDOM_FLOAT(0, 1);
		if (flRand <= 0.5)
		{
			iAnim = GAUSS_IDLE;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
		}
		else if (flRand <= 0.75)
		{
			iAnim = GAUSS_IDLE2;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
		}
		else
		{
			iAnim = GAUSS_FIDGET;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 3;
		}

		return;
		SendWeaponAnim( iAnim );
		
	}
}

class CGaussAmmo : public CBasePlayerAmmo
{
public:
	DECLARE_CLASS( CGaussAmmo, CBasePlayerAmmo );

	void Spawn() override
	{ 
		Precache( );
		SetModel( "models/w_gaussammo.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache() override
	{
		PRECACHE_MODEL ("models/w_gaussammo.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	bool AddAmmo( CBaseEntity *pOther ) override
	{ 
		return UTIL_GiveAmmoToPlayer( this, pOther, AMMO_URANIUMBOX_GIVE, "uranium" );
	}
};

LINK_ENTITY_TO_CLASS( ammo_gaussclip, CGaussAmmo );
