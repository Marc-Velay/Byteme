#ifndef MAIN_H_INCLUDED
#define MAIN_H_INCLUDED


#include <irrlicht.h>
#include "wtypes.h"
#include "q3factory.h"
#include "init.h"

struct GameData
{
	GameData ( const path &startupDir) :
		retVal(0), StartupDir(startupDir), createExDevice(0), Device(0)
	{
		setDefault ();
	}

	void setDefault ();
	s32 save ( const path &filename );
	s32 load ( const path &filename );

	s32 debugState;
	s32 gravityState;
	s32 flyTroughState;
	s32 wireFrame;
	s32 guiActive;
	s32 guiInputActive;
	f32 GammaValue;
	s32 retVal;
	s32 sound;

	path StartupDir;
	stringw CurrentMapName;
	array<path> CurrentArchiveList;

	vector3df PlayerPosition;
	vector3df PlayerRotation;

	tQ3EntityList Variable;

	Q3LevelLoadParameter loadParam;
	SIrrlichtCreationParameters deviceParam;
	funcptr_createDeviceEx createExDevice;
	IrrlichtDevice *Device;
};


#endif // MAIN_H_INCLUDED
