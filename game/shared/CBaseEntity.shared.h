#ifndef GAME_SHARED_CBASEENTITY_SHARED_H
#define GAME_SHARED_CBASEENTITY_SHARED_H

#include <cstddef>

typedef void ( CBaseEntity::*BASEPTR )();
typedef void ( CBaseEntity::*ENTITYFUNCPTR )( CBaseEntity *pOther );
typedef void ( CBaseEntity::*USEPTR )( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

//
// Base Entity.  All entity types derive from this
//
class CBaseEntity
{
public:
	DECLARE_CLASS_NOBASE( CBaseEntity );
	DECLARE_DATADESC_NOBASE();

	// Constructor.  Set engine to use C/C++ callback functions
	// pointers to engine data
	entvars_t *pev;		// Don't need to save/restore this pointer, the engine resets it

						// path corners
	CBaseEntity			*m_pGoalEnt;// path corner we are heading towards
	CBaseEntity			*m_pLink;// used for temporary link-list operations. 

	/**
	*	Called when the entity is first created. - Solokiller
	*/
	virtual void OnCreate() {}

	/**
	*	Called when the entity is destroyed. - Solokiller
	*/
	virtual void  OnDestroy() {}

	/**
	*	Called when an entity is removed at runtime. Gives entities a chance to respond to it. Not called during map change or shutdown.
	*	Call the baseclass version after handling it.
	*	Used to be non-virtual - Solokiller
	*/
	virtual void UpdateOnRemove();

	// initialization functions

	/**
	*	Called once for each keyvalue provided in the bsp file for this entity.
	*	@param pkvd Keyvalue data. Set pkvd->fHandled to true if you handled the key.
	*/
	virtual void KeyValue( KeyValueData* pkvd ) { pkvd->fHandled = false; }

	/**
	*	Called when the entity should precache its resources.
	*	Should call the baseclass implementation first.
	*/
	virtual void Precache() {}

	/**
	*	Called after all keyvalues have been passed in.
	*	Should call Precache.
	*/
	virtual void Spawn() {}

	/**
	*	Called when the server activates. Gives entities a chance to connect with eachother.
	*	Is not called if the entity is created at runtime.
	*	If the entity has the FL_DORMANT set, this will not be called.
	*/
	virtual void Activate() {}

	/**
	*	Called when the entity is being saved to a save game file.
	*	Call the baseclass implementation first, return false on failure.
	*	@param save Save data.
	*	@return true if the entity was successfully saved, false otherwise.
	*/
	virtual bool Save( CSave &save );

	/**
	*	Called when the entity is being restored from a save game file.
	*	Call the baseclass implementation first, return false on failure.
	*	@param restore Restore data.
	*	@return true if the entity was successfully restored, false otherwise.
	*/
	virtual bool Restore( CRestore &restore );

	/**
	*	Object capabilities.
	*	@return A bit vector of FCapability values.
	*	@see FCapability
	*/
	virtual int ObjectCaps() const { return FCAP_ACROSS_TRANSITION; }

	/**
	*	Setup the object->object collision box (pev->mins / pev->maxs is the object->world collision box)
	*/
	virtual void SetObjectCollisionBox();

	/**
	*	Respawns the entity. Entities that can be respawned should override this and return a new instance.
	*/
	virtual CBaseEntity* Respawn() { return nullptr; }

	/**
	*	@return This entity's edict.
	*/
	const edict_t* edict() const { return ENT( pev ); }

	/**
	*	@copydoc edict_t* edict() const
	*/
	edict_t* edict() { return ENT( pev ); }

	/**
	*	@return Offset of this entity. This is the byte offset in the edict array.
	*	DO NOT USE THIS. Use entindex instead.
	*/
	EOFFSET eoffset() const { return OFFSET( pev ); }

	/**
	*	@return The index of this entity.
	*	0 is worldspawn.
	*	[ 1, gpGlobals->maxClients ] are players.
	*	] gpGlobals->maxClients, gpGlobals->maxEntities [ are normal entities.
	*/
	int entindex() const { return ENTINDEX( edict() ); }

	// fundamental callbacks
	BASEPTR			m_pfnThink;
	ENTITYFUNCPTR	m_pfnTouch;
	USEPTR			m_pfnUse;
	ENTITYFUNCPTR	m_pfnBlocked;

	virtual void Think()
	{
		if( m_pfnThink )
			( this->*m_pfnThink )();
	}

	virtual void Touch( CBaseEntity *pOther )
	{
		if( m_pfnTouch )
			( this->*m_pfnTouch )( pOther );
	}

	virtual void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
	{
		if( m_pfnUse )
			( this->*m_pfnUse )( pActivator, pCaller, useType, value );
	}

	virtual void Blocked( CBaseEntity *pOther )
	{
		if( m_pfnBlocked )
			( this->*m_pfnBlocked )( pOther );
	}

	// Ugly code to lookup all functions to make sure they are exported when set.
#ifdef _DEBUG
	void FunctionCheck( void *pFunction, char *name )
	{
		if( pFunction && !NAME_FOR_FUNCTION( ( uint32 ) pFunction ) )
			ALERT( at_error, "No EXPORT: %s:%s (%08lx)\n", STRING( pev->classname ), name, ( uint32 ) pFunction );
	}

	BASEPTR	ThinkSet( BASEPTR func, char *name )
	{
		m_pfnThink = func;
		FunctionCheck( ( void * )*( ( int * ) ( ( char * )this + ( offsetof( CBaseEntity, m_pfnThink ) ) ) ), name );
		return func;
	}
	ENTITYFUNCPTR TouchSet( ENTITYFUNCPTR func, char *name )
	{
		m_pfnTouch = func;
		FunctionCheck( ( void * )*( ( int * ) ( ( char * )this + ( offsetof( CBaseEntity, m_pfnTouch ) ) ) ), name );
		return func;
	}
	USEPTR	UseSet( USEPTR func, char *name )
	{
		m_pfnUse = func;
		FunctionCheck( ( void * )*( ( int * ) ( ( char * )this + ( offsetof( CBaseEntity, m_pfnUse ) ) ) ), name );
		return func;
	}
	ENTITYFUNCPTR	BlockedSet( ENTITYFUNCPTR func, char *name )
	{
		m_pfnBlocked = func;
		FunctionCheck( ( void * )*( ( int * ) ( ( char * )this + ( offsetof( CBaseEntity, m_pfnBlocked ) ) ) ), name );
		return func;
	}

#endif

	// allow engine to allocate instance data
	void *operator new( size_t stAllocateBlock, entvars_t *pev )
	{
		return ( void* ) ALLOC_PRIVATE( ENT( pev ), stAllocateBlock );
	}

	// don't use this.
#if _MSC_VER >= 1200 // only build this code if MSVC++ 6.0 or higher
	void operator delete( void *pMem, entvars_t *pev )
	{
		pev->flags |= FL_KILLME;
	}
#endif

	/**
	*	Returns the CBaseEntity instance of the given edict.
	*	@param pent Edict whose instance should be returned. If this is null, uses worldspawn.
	*	@return Entity instance, or null if no instance is assigned to it.
	*/
	static CBaseEntity *Instance( edict_t *pent )
	{
		if( !pent )
			pent = ENT( 0 );
		CBaseEntity *pEnt = ( CBaseEntity * ) GET_PRIVATE( pent );
		return pEnt;
	}

	/**
	*	Returns the CBaseEntity instance of the given entvars.
	*	@param pev Entvars whose instance should be returned.
	*	@return Entity instance, or null if no instance is assigned to it.
	*/
	static CBaseEntity *Instance( entvars_t *pev ) { return Instance( ENT( pev ) ); }

	/**
	*	Returns the CBaseEntity instance of the given eoffset.
	*	@param eoffset Entity offset whose instance should be returned.
	*	@return Entity instance, or null if no instance is assigned to it.
	*/
	static CBaseEntity *Instance( EOFFSET eoffset ) { return Instance( ENT( eoffset ) ); }

	/**
	*	Creates an entity by class name.
	*	@param szName Class name of the entity. This string must continue to exist for at least as long as the map itself.
	*	@param vecOrigin Intended entity origin.
	*	@param vecAngles Intended entity angles.
	*	@param pentOwner Optional. The owner of the newly created entity.
	*	@return Newly created entity, or null if the entity could not be created.
	*/
	static CBaseEntity *Create( char *szName, const Vector &vecOrigin, const Vector &vecAngles, edict_t *pentOwner = nullptr );

	/*
	*	Returns the type of group (i.e, "houndeye", or "human military" so that monsters with different classnames
	*	still realize that they are teammates. (overridden for monsters that form groups)
	*/
	virtual int Classify() { return CLASS_NONE; }

	virtual void	TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType );
	virtual int		TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType );

	/**
	*	Gives health to this entity. Negative values take health.
	*	Used to be called TakeHealth.
	*	- Solokiller
	*	@param flHealth Amount of health to give. Negative values take health.
	*	@param bitsDamageType Damage types bit vector. @see Damage enum.
	*	@return Actual amount of health that was given/taken.
	*/
	virtual float GiveHealth( float flHealth, int bitsDamageType );

	virtual void	Killed( entvars_t *pevAttacker, int iGib );

	/**
	*	@return This entity's blood color.
	*	@see BloodColor
	*/
	virtual int BloodColor() const { return DONT_BLEED; }

	/**
	*	Projects blood decals based on the given damage and traceline.
	*	@param flDamage Amount of damage being dealt.
	*	@param vecDir attack direction.
	*	@param ptr Attack traceline.
	*	@param bitsDamageType Bit vector of damage types.
	*	@see Damage
	*/
	virtual void TraceBleed( float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType );

	/**
	*	@param pActivator Activator.
	*	@return Whether this entity would be triggered by the given activator.
	*/
	virtual bool IsTriggered( const CBaseEntity* const pActivator ) const { return true; }

	/**
	*	@return This entity as a CBaseMonster instance, or null if it isn't a monster.
	*/
	virtual CBaseMonster* MyMonsterPointer() { return nullptr; }

	/**
	*	@return This entity as a CSquadMonster instance, or null if it isn't a squad monster.
	*/
	virtual CSquadMonster* MySquadMonsterPointer() { return nullptr; }

	//TODO: entities that use this function should check the classname, so casting to the actual type and using it is better than a costly virtual function hack - Solokiller
	virtual float	GetDelay() { return 0; }
	virtual bool	IsMoving() const { return pev->velocity != g_vecZero; }
	virtual void	OverrideReset() {}

	/**
	*	Returns the decal to project onto this entity given the damage types inflicted upon it. If this entity is alpha tested, returns -1.
	*	@param bitsDamageType
	*	@return Decal to use, or -1.
	*/
	virtual int DamageDecal( int bitsDamageType ) const;

	// This is ONLY used by the node graph to test movement through a door
	virtual void	SetToggleState( int state ) {}

	virtual void StartSneaking() {}
	virtual void StopSneaking() {}
	virtual bool IsSneaking() { return false; }

	/**
	*	Checks if the given entity can control this entity.
	*	@param pTest Entity to check for control.
	*	@return true if this entity can be controlled, false otherwise.
	*/
	virtual bool OnControls( const CBaseEntity* const pTest ) const { return false; }

	/**
	*	@return Whether this entity is alive.
	*/
	virtual bool IsAlive() const { return ( pev->deadflag == DEAD_NO ) && pev->health > 0; }

	/**
	*	@return Whether this is a BSP model.
	*/
	virtual bool IsBSPModel() const { return pev->solid == SOLID_BSP || pev->movetype == MOVETYPE_PUSHSTEP; }

	/**
	*	@return Whether gauss gun beams should reflect off of this entity.
	*/
	virtual bool ReflectGauss() const { return ( IsBSPModel() && !pev->takedamage ); }

	/**
	*	@return Whether this entity has the given target.
	*/
	virtual bool HasTarget( string_t targetname ) const { return FStrEq( STRING( targetname ), STRING( pev->targetname ) ); }

	/**
	*	@return Whether this entity is positioned in the world.
	*/
	virtual bool IsInWorld() const;

	/**
	*	@return Whether this is a player.
	*/
	virtual	bool IsPlayer() const { return false; }

	/**
	*	@return Whether this is a connected client. Fake clients do not qualify.
	*	TODO: this only applies to players and spectators. Move it? - Solokiller
	*/
	virtual bool IsNetClient() const { return false; }

	/**
	*	@return This entity's team name.
	*/
	virtual const char* TeamID() const { return ""; }

	/**
	*	@return The next entity that this entity should target.
	*/
	virtual CBaseEntity *GetNextTarget();

	// common member functions
	void EXPORT SUB_Remove();
	void EXPORT SUB_DoNothing();
	void EXPORT SUB_StartFadeOut();
	void EXPORT SUB_FadeOut();
	void EXPORT SUB_CallUseToggle() { this->Use( this, this, USE_TOGGLE, 0 ); }
	bool ShouldToggle( USE_TYPE useType, const bool currentState ) const;

	void FireBullets( const unsigned int cShots,
					  Vector vecSrc, Vector vecDirShooting, Vector vecSpread, 
					  float flDistance, int iBulletType, 
					  int iTracerFreq = 4, int iDamage = 0, entvars_t *pevAttacker = nullptr );

	Vector FireBulletsPlayer( const unsigned int cShots,
							  Vector vecSrc, Vector vecDirShooting, Vector vecSpread, 
							  float flDistance, int iBulletType, 
							  int iTracerFreq = 4, int iDamage = 0, entvars_t *pevAttacker = nullptr, int shared_rand = 0 );

	void SUB_UseTargets( CBaseEntity *pActivator, USE_TYPE useType, float value );
	// Do the bounding boxes of these two intersect?
	bool	Intersects( const CBaseEntity* const pOther ) const;
	void	MakeDormant();
	bool	IsDormant() const;
	//Made this virtual. Used to be non-virtual and redeclared in CBaseToggle - Solokiller
	virtual bool    IsLockedByMaster() const { return false; }

	//Made these static. No point in having member functions that don't access this. - Solokiller
	static CBaseMonster *GetMonsterPointer( entvars_t *pevMonster )
	{
		CBaseEntity *pEntity = Instance( pevMonster );
		if( pEntity )
			return pEntity->MyMonsterPointer();
		return nullptr;
	}

	static CBaseMonster *GetMonsterPointer( edict_t *pentMonster )
	{
		CBaseEntity *pEntity = Instance( pentMonster );
		if( pEntity )
			return pEntity->MyMonsterPointer();
		return nullptr;
	}

	// virtual functions used by a few classes

	/**
	*	Monster maker children use this to tell the monster maker that they have died.
	*/
	virtual void DeathNotice( CBaseEntity* pChild ) {}

	// used by monsters that are created by the MonsterMaker
	//TODO: seems to be unused. Remove? - Solokiller
	virtual	void UpdateOwner() {}

	/**
	*	Tries to send a monster into PRONE state.
	*	Right now only used when a barnacle snatches someone, so may have some special case stuff for that.
	*	@return Whether the entity has become prone.
	*	TODO: only used by barnacles so rename it.
	*/
	virtual bool FBecomeProne() { return false; }

	/**
	*	@return Center point of entity.
	*/
	virtual Vector Center() const { return ( pev->absmax + pev->absmin ) * 0.5; }

	/**
	*	@return Position of eyes.
	*/
	virtual Vector EyePosition() const { return pev->origin + pev->view_ofs; }

	/**
	*	@return Position of ears.
	*/
	virtual Vector EarPosition() const { return pev->origin + pev->view_ofs; }

	/**
	*	@return Position to shoot at.
	*/
	virtual Vector BodyTarget( const Vector &posSrc ) const { return Center(); }

	/**
	*	@return Entity illumination.
	*/
	virtual int Illumination() const { return GETENTITYILLUM( ENT( pev ) ); }

	/**
	*	@return Whether this entity is visible to the given entity.
	*/
	virtual	bool FVisible( const CBaseEntity *pEntity ) const;

	/**
	*	@return Whether this entity is visible from the given position.
	*/
	virtual	bool FVisible( const Vector &vecOrigin ) const;

	//TODO: find a way to get rid of this stuff. We need to be able to network arbitrary ammo counts - Solokiller
	//We use this variables to store each ammo count.
	int ammo_9mm;
	int ammo_357;
	int ammo_bolts;
	int ammo_buckshot;
	int ammo_rockets;
	int ammo_uranium;
	int ammo_hornets;
	int ammo_argrens;
};

// Ugly technique to override base member functions
// Normally it's illegal to cast a pointer to a member function of a derived class to a pointer to a 
// member function of a base class.  static_cast is a sleezy way around that problem.

#ifdef _DEBUG

#define SetThink( a ) ThinkSet( static_cast <void (CBaseEntity::*)(void)> (a), #a )
#define SetTouch( a ) TouchSet( static_cast <void (CBaseEntity::*)(CBaseEntity *)> (a), #a )
#define SetUse( a ) UseSet( static_cast <void (CBaseEntity::*)(	CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )> (a), #a )
#define SetBlocked( a ) BlockedSet( static_cast <void (CBaseEntity::*)(CBaseEntity *)> (a), #a )

#else

#define SetThink( a ) m_pfnThink = static_cast <void (CBaseEntity::*)(void)> (a)
#define SetTouch( a ) m_pfnTouch = static_cast <void (CBaseEntity::*)(CBaseEntity *)> (a)
#define SetUse( a ) m_pfnUse = static_cast <void (CBaseEntity::*)( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )> (a)
#define SetBlocked( a ) m_pfnBlocked = static_cast <void (CBaseEntity::*)(CBaseEntity *)> (a)

#endif

#endif //GAME_SHARED_CBASEENTITY_SHARED_H