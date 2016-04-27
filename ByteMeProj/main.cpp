

#include "main.h"


using namespace irr;
using namespace core;
using namespace scene;
using namespace video;
using namespace io;
using namespace gui;
using namespace std;


/*********************************************************************/
void GameData::setDefault ()
{
	debugState = EDS_OFF;
	gravityState = 1;
	flyTroughState = 0;
	wireFrame = 0;
	guiActive = 1;
	guiInputActive = 0;
	GammaValue = 1.f;

	// default deviceParam;

	deviceParam.Fullscreen = true;
	deviceParam.Bits = 24;
	deviceParam.ZBufferBits = 16;
	deviceParam.Vsync = false;
	deviceParam.AntiAlias = false;

	// default Quake3 loadParam
	loadParam.defaultLightMapMaterial = EMT_LIGHTMAP;
	loadParam.defaultModulate = EMFN_MODULATE_1X;
	loadParam.defaultFilter = EMF_ANISOTROPIC_FILTER;
	loadParam.verbose = 2;
	loadParam.mergeShaderBuffer = 1;		// merge meshbuffers with same material
	loadParam.cleanUnResolvedMeshes = 1;	// should unresolved meshes be cleaned. otherwise blue texture
	loadParam.loadAllShaders = 1;			// load all scripts in the script directory
	loadParam.loadSkyShader = 0;			// load sky Shader
	loadParam.alpharef = 1;

	sound = 0;

	CurrentMapName = "";
	CurrentArchiveList.clear ();


	CurrentArchiveList.push_back ( StartupDir + "./media/" );

	// Add the original quake3 files before you load your custom map
	// Most mods are using the original shaders, models&items&weapons
	CurrentArchiveList.push_back("/q/baseq3/");

	CurrentArchiveList.push_back(StartupDir + "./media/map/map-20kdm2.pk3");
}

/*********************************************************************/
s32 GameData::load ( const path &filename )
{
	if (!Device) return 0;

	// the quake3 mesh loader can also handle *.shader and *.cfg file
	IQ3LevelMesh* mesh = (IQ3LevelMesh*) Device->getSceneManager()->getMesh ( filename );
	if (!mesh) return 0;

	tQ3EntityList &entityList = mesh->getEntityList ();

	stringc s;
	u32 pos;

	for ( u32 e = 0; e != entityList.size (); e++ )
	{
		//dumpShader ( s, &entityList[e], false );
		//printf ( s.c_str () );

		for ( u32 g = 0; g != entityList[e].getGroupSize (); g++ )
		{
			const SVarGroup *group = entityList[e].getGroup ( g );

			for ( u32 index = 0; index < group->Variable.size (); index++ )
			{
				const SVariable &v = group->Variable[index];
				pos = 0;
				if ( v.name == "playerposition" )
				{
					PlayerPosition = getAsVector3df ( v.content, pos );
				}
				else
				if ( v.name == "playerrotation" )
				{
					PlayerRotation = getAsVector3df ( v.content, pos );
				}
			}
		}
	}

	return 1;
}




/*********************************************************************/
s32 GameData::save ( const path &filename )
{
	//return 0;
	if (!Device) return 0;

	c8 buf[128];
	u32 i;

	// Store current Archive for restart
	CurrentArchiveList.clear();
	IFileSystem *fs = Device->getFileSystem();
	for ( i = 0; i != fs->getFileArchiveCount(); ++i )
	{
		CurrentArchiveList.push_back ( fs->getFileArchive(i)->getFileList()->getPath() );
	}

	// Store Player Position and Rotation
	ICameraSceneNode * camera = Device->getSceneManager()->getActiveCamera ();
	if ( camera )
	{
		PlayerPosition = camera->getPosition ();
		PlayerRotation = camera->getRotation ();
	}

	IWriteFile *file = fs->createAndWriteFile ( filename );
	if (!file) return 0;

	snprintf ( buf, 128, "playerposition %.f %.f %.f\nplayerrotation %.f %.f %.f\n",
			PlayerPosition.X, PlayerPosition.Z, PlayerPosition.Y,
			PlayerRotation.X, PlayerRotation.Z, PlayerRotation.Y);
	file->write ( buf, (s32) strlen ( buf ) );
	for ( i = 0; i != fs->getFileArchiveCount(); ++i )
	{
		snprintf ( buf, 128, "archive %s\n",stringc ( fs->getFileArchive(i)->getFileList()->getPath() ).c_str () );
		file->write ( buf, (s32) strlen ( buf ) );
	}

	file->drop ();
	return 1;
}


/*********************************************************************/
struct Q3Player : public IAnimationEndCallBack
{
	Q3Player ()
	: Device(0), MapParent(0), Mesh(0), WeaponNode(0), StartPositionCurrent(0)
	{
		animation[0] = 0;
		memset(Anim, 0, sizeof(TimeFire)*4);
	}

	virtual void OnAnimationEnd(IAnimatedMeshSceneNode* node);

	void create (	IrrlichtDevice *device,
					IQ3LevelMesh* mesh,
					ISceneNode *mapNode,
					IMetaTriangleSelector *meta
				);
	void shutdown ();
	void setAnim ( const c8 *name );
	void respawn ();
	void setpos ( const vector3df &pos, const vector3df& rotation );

	ISceneNodeAnimatorCollisionResponse * cam() { return camCollisionResponse ( Device ); }

	IrrlichtDevice *Device;
	ISceneNode* MapParent;
	IQ3LevelMesh* Mesh;
	IAnimatedMeshSceneNode* WeaponNode;
	s32 StartPositionCurrent;
	TimeFire Anim[4];
	c8 animation[64];
	c8 buf[64];
};

/*********************************************************************/
void Q3Player::shutdown ()
{
	setAnim ( 0 );

	dropElement (WeaponNode);

	if ( Device )
	{
		ICameraSceneNode* camera = Device->getSceneManager()->getActiveCamera();
		dropElement ( camera );
		Device = 0;
	}

	MapParent = 0;
	Mesh = 0;
}


/*********************Playable char!!**********************************/
void Q3Player::create ( IrrlichtDevice *device, IQ3LevelMesh* mesh, ISceneNode *mapNode, IMetaTriangleSelector *meta )
{
	setTimeFire ( Anim + 0, 200, FIRED );
	setTimeFire ( Anim + 1, 5000 );

	if (!device)
		return;
	// load FPS weapon to Camera
	Device = device;
	Mesh = mesh;
	MapParent = mapNode;

	ISceneManager *smgr = device->getSceneManager ();
	IVideoDriver * driver = device->getVideoDriver();

	ICameraSceneNode* camera = 0;

	SKeyMap keyMap[10];
	keyMap[0].Action = EKA_MOVE_FORWARD;
	keyMap[0].KeyCode = KEY_UP;
	keyMap[1].Action = EKA_MOVE_FORWARD;
	keyMap[1].KeyCode = KEY_KEY_Z;

	keyMap[2].Action = EKA_MOVE_BACKWARD;
	keyMap[2].KeyCode = KEY_DOWN;
	keyMap[3].Action = EKA_MOVE_BACKWARD;
	keyMap[3].KeyCode = KEY_KEY_S;

	keyMap[4].Action = EKA_STRAFE_LEFT;
	keyMap[4].KeyCode = KEY_LEFT;
	keyMap[5].Action = EKA_STRAFE_LEFT;
	keyMap[5].KeyCode = KEY_KEY_Q;

	keyMap[6].Action = EKA_STRAFE_RIGHT;
	keyMap[6].KeyCode = KEY_RIGHT;
	keyMap[7].Action = EKA_STRAFE_RIGHT;
	keyMap[7].KeyCode = KEY_KEY_D;

	keyMap[8].Action = EKA_JUMP_UP;
	keyMap[8].KeyCode = KEY_SPACE;

	keyMap[9].Action = respawn();
	keyMap[9].KeyCode = KEY_KEY_P;


	/**************************à remplacer par camera 3eme persone***************************/
	camera = smgr->addCameraSceneNodeFPS(0, 100.0f, 0.6f, -1, keyMap, 10, false, 0.6f);
	camera->setName ( "First Person Camera" );
	//camera->setFOV ( 100.f * core::DEGTORAD );
	camera->setFarValue( 20000.f );
	/*********************************************************************/

	IAnimatedMeshMD2* weaponMesh = (IAnimatedMeshMD2*) smgr->getMesh("gun.md2");
	if ( 0 == weaponMesh )
		return;

	if ( weaponMesh->getMeshType() == EAMT_MD2 )
	{
		s32 count = weaponMesh->getAnimationCount();
		for ( s32 i = 0; i != count; ++i )
		{
			snprintf ( buf, 64, "Animation: %s", weaponMesh->getAnimationName(i) );
			device->getLogger()->log(buf, ELL_INFORMATION);
		}
	}

	WeaponNode = smgr->addAnimatedMeshSceneNode(
						weaponMesh,
						smgr->getActiveCamera(),
						10,
						vector3df( 0, 0, 0),
						vector3df(-90,-90,90)
						);
	WeaponNode->setMaterialFlag(EMF_LIGHTING, false);
	WeaponNode->setMaterialTexture(0, driver->getTexture( "gun.jpg"));
	WeaponNode->setLoopMode ( false );
	WeaponNode->setName ( "tommi the gun man" );

	//create a collision auto response animator
	ISceneNodeAnimator* anim =
		smgr->createCollisionResponseAnimator( meta, camera,
			vector3df(30,45,30),
			getGravity ( "earth" ),
			vector3df(0,40,0),
			0.0005f
		);

	camera->addAnimator( anim );
	anim->drop();

	if ( meta )
	{
		meta->drop ();
	}

	respawn ();
	setAnim ( "idle" );
}


/*********************************************************************/
void Q3Player::respawn ()
{
	if (!Device) return;
	ICameraSceneNode* camera = Device->getSceneManager()->getActiveCamera();

	Device->getLogger()->log( "respawn" );

	if ( StartPositionCurrent >= Q3StartPosition (
			Mesh, camera,StartPositionCurrent++,
			cam ()->getEllipsoidTranslation() )
		)
	{
		StartPositionCurrent = 0;
	}
}


/*********************************************************************/
void Q3Player::setpos ( const vector3df &pos, const vector3df &rotation )
{
	if (!Device) return;
	Device->getLogger()->log( "setpos" );

	ICameraSceneNode* camera = Device->getSceneManager()->getActiveCamera();
	if ( camera )
	{
		camera->setPosition ( pos );
		camera->setRotation ( rotation );
		//! New. FPSCamera and animators catches reset on animate 0
		camera->OnAnimate ( 0 );
	}
}


/*********************************************************************/
void Q3Player::setAnim ( const c8 *name )
{
	if ( name )
	{
		snprintf ( animation, 64, "%s", name );
		if ( WeaponNode )
		{
			WeaponNode->setAnimationEndCallback ( this );
			WeaponNode->setMD2Animation ( animation );
		}
	}
	else
	{
		animation[0] = 0;
		if ( WeaponNode )
		{
			WeaponNode->setAnimationEndCallback ( 0 );
		}
	}
}

/*********************************************************************/
void Q3Player::OnAnimationEnd(IAnimatedMeshSceneNode* node)
{
	setAnim ( 0 );
}

/*********************************************************************/
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{

    /*
    The most important function of the engine is the 'createDevice'
    function. The Irrlicht Device can be created with it, which is the
    root object for doing everything with the engine.
    createDevice() has 7 paramters:
    deviceType: Type of the device. This can currently be the Null-device,
       the Software device, DirectX8, DirectX9, or OpenGL. In this example we use
       EDT_SOFTWARE, but to try out, you might want to change it to
       EDT_NULL, EDT_DIRECTX8 , EDT_DIRECTX9, or EDT_OPENGL.
    windowSize: Size of the Window or FullscreenMode to be created. In this
       example we use 640x480.
    bits: Amount of bits per pixel when in fullscreen mode. This should
       be 16 or 32. This parameter is ignored when running in windowed mode.
    fullscreen: Specifies if we want the device to run in fullscreen mode
       or not.
    stencilbuffer: Specifies if we want to use the stencil buffer for drawing shadows.
    vsync: Specifies if we want to have vsync enabled, this is only useful in fullscreen
      mode.
    eventReceiver: An object to receive events. We do not want to use this
       parameter here, and set it to 0.
    */

    IrrlichtDevice *device =
        createDevice(EDT_OPENGL, dimension2d<u32>(GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN)), 16,
            true, false, false, 0);


    device->setWindowCaption(L"Byte me!");

    IVideoDriver* driver = device->getVideoDriver();
    ISceneManager* smgr = device->getSceneManager();
    IGUIEnvironment* guienv = device->getGUIEnvironment();

   /*
    guienv->addStaticText(L"Hello World! This is the Irrlicht Software renderer!",
        rect<int>(10,10,200,22), true);
    */

    IAnimatedMesh* mesh = smgr->getMesh("./media/images/sydney.md2");
    IAnimatedMeshSceneNode* node = smgr->addAnimatedMeshSceneNode( mesh );

    /*
    To let the mesh look a little bit nicer, we change its material a
    little bit: We disable lighting because we do not have a dynamic light
    in here, and the mesh would be totally black. Then we set the frame
    loop, so that the animation is looped between the frames 0 and 310.
    And at last, we apply a texture to the mesh. Without it the mesh
    would be drawn using only a color.
    */
    if (node)
    {
        node->setMaterialFlag(EMF_LIGHTING, false);
        node->setFrameLoop(0, 310);
        node->setMaterialTexture( 0, driver->getTexture("../irrlicht-1.8.3/media/sydney.bmp") );
    }

    /*
    To look at the mesh, we place a camera into 3d space at the position
    (0, 30, -40). The camera looks from there to (0,5,0).
    */
    smgr->addCameraSceneNode(0, vector3df(0,30,-40), vector3df(0,5,0));

    while(device->run())
    {
        /*
        Anything can be drawn between a beginScene() and an endScene()
        call. The beginScene clears the screen with a color and also the
        depth buffer if wanted. Then we let the Scene Manager and the
        GUI Environment draw their content. With the endScene() call
        everything is presented on the screen.
        */
        driver->beginScene(true, true, SColor(0,200,200,200));

        smgr->drawAll();
        guienv->drawAll();

        driver->endScene();
    }


    device->drop();

    return 0;
}

