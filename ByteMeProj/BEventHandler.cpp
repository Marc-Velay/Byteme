#include "BEventHandler.h"




BEventHandler::BEventHandler(GameData *game)
: Game(game), Mesh(0), MapParent(0), ShaderParent(0), ItemParent(0), UnresolvedParent(0),
	BulletParent(0), FogParent(0), SkyNode(0), Meta(0)
{
    buf[0]=0;
	// Also use 16 Bit Textures for 16 Bit RenderDevice
	if ( Game->deviceParam.Bits == 16 )
	{
		game->Device->getVideoDriver()->setTextureCreationFlag(ETCF_ALWAYS_16_BIT, true);
	}

	// Quake3 Shader controls Z-Writing
	game->Device->getSceneManager()->getParameters()->setAttribute(scene::ALLOW_ZWRITE_ON_TRANSPARENT, true);

	// create internal textures
	createTextures ();

	sound_init ( game->Device );

	Game->Device->setEventReceiver ( this );
}

BEventHandler::~BEventHandler()
{
    Player[0].shutdown ();
	sound_shutdown ();

	Game->save( "explorer.cfg" );

	Game->Device->drop();
}


void BEventHandler::createTextures()
{
	IVideoDriver * driver = Game->Device->getVideoDriver();

	dimension2du dim(64, 64);

	video::IImage* image;
	u32 i;
	u32 x;
	u32 y;
	u32 * data;
	for ( i = 0; i != 8; ++i )
	{
		image = driver->createImage ( video::ECF_A8R8G8B8, dim);
		data = (u32*) image->lock ();
		for ( y = 0; y != dim.Height; ++y )
		{
			for ( x = 0; x != dim.Width; ++x )
			{
				data [x] = 0xFFFFFFFF;
			}
			data = (u32*) ( (u8*) data + image->getPitch() );
		}
		image->unlock();
		snprintf ( buf, 64, "smoke_%02d", i );
		driver->addTexture( buf, image );
		image->drop ();
	}

	// fog
	for ( i = 0; i != 1; ++i )
	{
		image = driver->createImage ( video::ECF_A8R8G8B8, dim);
		data = (u32*) image->lock ();
		for ( y = 0; y != dim.Height; ++y )
		{
			for ( x = 0; x != dim.Width; ++x )
			{
				data [x] = 0xFFFFFFFF;
			}
			data = (u32*) ( (u8*) data + image->getPitch() );
		}
		image->unlock();
		snprintf ( buf, 64, "fog_%02d", i );
		driver->addTexture( buf, image );
		image->drop ();
	}
}



void BEventHandler::CreateGUI()
{

	IGUIEnvironment *env = Game->Device->getGUIEnvironment();
	IVideoDriver * driver = Game->Device->getVideoDriver();

    /*
	gui.drop();

	// set skin font
	IGUIFont* font = env->getFont("fontlucida.png");
	if (font)
		env->getSkin()->setFont(font);
	env->getSkin()->setColor ( EGDC_BUTTON_TEXT, video::SColor(240,0xAA,0xAA,0xAA) );
	env->getSkin()->setColor ( EGDC_3D_HIGH_LIGHT, video::SColor(240,0x22,0x22,0x22) );
	env->getSkin()->setColor ( EGDC_3D_FACE, video::SColor(240,0x44,0x44,0x44) );
	env->getSkin()->setColor ( EGDC_EDITABLE, video::SColor(240,0x44,0x44,0x44) );
	env->getSkin()->setColor ( EGDC_FOCUSED_EDITABLE, video::SColor(240,0x54,0x54,0x54) );
	env->getSkin()->setColor ( EGDC_WINDOW, video::SColor(240,0x66,0x66,0x66) );

	// minimal gui size 800x600
	dimension2d<u32> dim ( 800, 600 );
	dimension2d<u32> vdim ( Game->Device->getVideoDriver()->getScreenSize() );

	if ( vdim.Height >= dim.Height && vdim.Width >= dim.Width )
	{
		//dim = vdim;
	}
	else
	{
	}

	gui.Window = env->addWindow ( rect<s32> ( 0, 0, dim.Width, dim.Height ), false, L"Quake3 Explorer" );
	gui.Window->setToolTipText ( L"Quake3Explorer. Loads and show various BSP File Format and Shaders." );
	gui.Window->getCloseButton()->setToolTipText ( L"Quit Quake3 Explorer" );

	// add a status line help text
	gui.StatusLine = env->addStaticText( 0, rect<s32>( 5,dim.Height - 30,dim.Width - 5,dim.Height - 10),
								false, false, gui.Window, -1, true
							);


	env->addStaticText ( L"VideoDriver:", rect<s32>( dim.Width - 400, 24, dim.Width - 310, 40 ),false, false, gui.Window, -1, false );
	gui.VideoDriver = env->addComboBox(rect<s32>( dim.Width - 300, 24, dim.Width - 10, 40 ),gui.Window);
	gui.VideoDriver->addItem(L"Direct3D 9.0c", EDT_DIRECT3D9 );
	gui.VideoDriver->addItem(L"Direct3D 8.1", EDT_DIRECT3D8 );
	gui.VideoDriver->addItem(L"OpenGL 1.5", EDT_OPENGL);
	gui.VideoDriver->addItem(L"Software Renderer", EDT_SOFTWARE);
	gui.VideoDriver->addItem(L"Burning's Video (TM) Thomas Alten", EDT_BURNINGSVIDEO);
	gui.VideoDriver->setSelected ( gui.VideoDriver->getIndexForItemData ( Game->deviceParam.DriverType ) );
	gui.VideoDriver->setToolTipText ( L"Use a VideoDriver" );

	env->addStaticText ( L"VideoMode:", rect<s32>( dim.Width - 400, 44, dim.Width - 310, 60 ),false, false, gui.Window, -1, false );
	gui.VideoMode = env->addComboBox(rect<s32>( dim.Width - 300, 44, dim.Width - 10, 60 ),gui.Window);
	gui.VideoMode->setToolTipText ( L"Supported Screenmodes" );
	IVideoModeList *modeList = Game->Device->getVideoModeList();
	if ( modeList )
	{
		s32 i;
		for ( i = 0; i != modeList->getVideoModeCount (); ++i )
		{
			u16 d = modeList->getVideoModeDepth ( i );
			if ( d < 16 )
				continue;

			u16 w = modeList->getVideoModeResolution ( i ).Width;
			u16 h = modeList->getVideoModeResolution ( i ).Height;
			u32 val = w << 16 | h;

			if ( gui.VideoMode->getIndexForItemData ( val ) >= 0 )
				continue;

			f32 aspect = (f32) w / (f32) h;
			const c8 *a = "";
			if ( core::equals ( aspect, 1.3333333333f ) ) a = "4:3";
			else if ( core::equals ( aspect, 1.6666666f ) ) a = "15:9 widescreen";
			else if ( core::equals ( aspect, 1.7777777f ) ) a = "16:9 widescreen";
			else if ( core::equals ( aspect, 1.6f ) ) a = "16:10 widescreen";
			else if ( core::equals ( aspect, 2.133333f ) ) a = "20:9 widescreen";

			snprintf ( buf, sizeof ( buf ), "%d x %d, %s",w, h, a );
			gui.VideoMode->addItem ( stringw ( buf ).c_str(), val );
		}
	}
	gui.VideoMode->setSelected ( gui.VideoMode->getIndexForItemData (
									Game->deviceParam.WindowSize.Width << 16 |
									Game->deviceParam.WindowSize.Height ) );

	gui.FullScreen = env->addCheckBox ( Game->deviceParam.Fullscreen, rect<s32>( dim.Width - 400, 64, dim.Width - 300, 80 ), gui.Window,-1, L"Fullscreen" );
	gui.FullScreen->setToolTipText ( L"Set Fullscreen or Window Mode" );

	gui.Bit32 = env->addCheckBox ( Game->deviceParam.Bits == 32, rect<s32>( dim.Width - 300, 64, dim.Width - 240, 80 ), gui.Window,-1, L"32Bit" );
	gui.Bit32->setToolTipText ( L"Use 16 or 32 Bit" );

	env->addStaticText ( L"MultiSample:", rect<s32>( dim.Width - 235, 64, dim.Width - 150, 80 ),false, false, gui.Window, -1, false );
	gui.MultiSample = env->addScrollBar( true, rect<s32>( dim.Width - 150, 64, dim.Width - 70, 80 ), gui.Window,-1 );
	gui.MultiSample->setMin ( 0 );
	gui.MultiSample->setMax ( 8 );
	gui.MultiSample->setSmallStep ( 1 );
	gui.MultiSample->setLargeStep ( 1 );
	gui.MultiSample->setPos ( Game->deviceParam.AntiAlias );
	gui.MultiSample->setToolTipText ( L"Set the MultiSample (disable, 1x, 2x, 4x, 8x )" );

	gui.SetVideoMode = env->addButton (rect<s32>( dim.Width - 60, 64, dim.Width - 10, 80 ), gui.Window, -1,L"set" );
	gui.SetVideoMode->setToolTipText ( L"Set Video Mode with current values" );

	env->addStaticText ( L"Gamma:", rect<s32>( dim.Width - 400, 104, dim.Width - 310, 120 ),false, false, gui.Window, -1, false );
	gui.Gamma = env->addScrollBar( true, rect<s32>( dim.Width - 300, 104, dim.Width - 10, 120 ), gui.Window,-1 );
	gui.Gamma->setMin ( 50 );
	gui.Gamma->setMax ( 350 );
	gui.Gamma->setSmallStep ( 1 );
	gui.Gamma->setLargeStep ( 10 );
	gui.Gamma->setPos ( core::floor32 ( Game->GammaValue * 100.f ) );
	gui.Gamma->setToolTipText ( L"Adjust Gamma Ramp ( 0.5 - 3.5)" );
	Game->Device->setGammaRamp ( Game->GammaValue, Game->GammaValue, Game->GammaValue, 0.f, 0.f );


	env->addStaticText ( L"Tesselation:", rect<s32>( dim.Width - 400, 124, dim.Width - 310, 140 ),false, false, gui.Window, -1, false );
	gui.Tesselation = env->addScrollBar( true, rect<s32>( dim.Width - 300, 124, dim.Width - 10, 140 ), gui.Window,-1 );
	gui.Tesselation->setMin ( 2 );
	gui.Tesselation->setMax ( 12 );
	gui.Tesselation->setSmallStep ( 1 );
	gui.Tesselation->setLargeStep ( 1 );
	gui.Tesselation->setPos ( Game->loadParam.patchTesselation );
	gui.Tesselation->setToolTipText ( L"How smooth should curved surfaces be rendered" );

	gui.Collision = env->addCheckBox ( true, rect<s32>( dim.Width - 400, 150, dim.Width - 300, 166 ), gui.Window,-1, L"Collision" );
	gui.Collision->setToolTipText ( L"Set collision on or off ( flythrough ). \nPress F7 on your Keyboard" );
	gui.Visible_Map = env->addCheckBox ( true, rect<s32>( dim.Width - 300, 150, dim.Width - 240, 166 ), gui.Window,-1, L"Map" );
	gui.Visible_Map->setToolTipText ( L"Show or not show the static part the Level. \nPress F3 on your Keyboard" );
	gui.Visible_Shader = env->addCheckBox ( true, rect<s32>( dim.Width - 240, 150, dim.Width - 170, 166 ), gui.Window,-1, L"Shader" );
	gui.Visible_Shader->setToolTipText ( L"Show or not show the Shader Nodes. \nPress F4 on your Keyboard" );
	gui.Visible_Fog = env->addCheckBox ( true, rect<s32>( dim.Width - 170, 150, dim.Width - 110, 166 ), gui.Window,-1, L"Fog" );
	gui.Visible_Fog->setToolTipText ( L"Show or not show the Fog Nodes. \nPress F5 on your Keyboard" );
	gui.Visible_Unresolved = env->addCheckBox ( true, rect<s32>( dim.Width - 110, 150, dim.Width - 10, 166 ), gui.Window,-1, L"Unresolved" );
	gui.Visible_Unresolved->setToolTipText ( L"Show the or not show the Nodes the Engine can't handle. \nPress F6 on your Keyboard" );
	gui.Visible_Skydome = env->addCheckBox ( true, rect<s32>( dim.Width - 110, 180, dim.Width - 10, 196 ), gui.Window,-1, L"Skydome" );
	gui.Visible_Skydome->setToolTipText ( L"Show the or not show the Skydome." );

	//Respawn = env->addButton ( rect<s32>( dim.Width - 260, 90, dim.Width - 10, 106 ), 0,-1, L"Respawn" );

	env->addStaticText ( L"Archives:", rect<s32>( 5, dim.Height - 530, dim.Width - 600,dim.Height - 514 ),false, false, gui.Window, -1, false );

	gui.ArchiveAdd = env->addButton ( rect<s32>( dim.Width - 725, dim.Height - 530, dim.Width - 665, dim.Height - 514 ), gui.Window,-1, L"add" );
	gui.ArchiveAdd->setToolTipText ( L"Add an archive, usually packed zip-archives (*.pk3) to the Filesystem" );
	gui.ArchiveRemove = env->addButton ( rect<s32>( dim.Width - 660, dim.Height - 530, dim.Width - 600, dim.Height - 514 ), gui.Window,-1, L"del" );
	gui.ArchiveRemove->setToolTipText ( L"Remove the selected archive from the FileSystem." );
	gui.ArchiveUp = env->addButton ( rect<s32>( dim.Width - 575, dim.Height - 530, dim.Width - 515, dim.Height - 514 ), gui.Window,-1, L"up" );
	gui.ArchiveUp->setToolTipText ( L"Arrange Archive Look-up Hirachy. Move the selected Archive up" );
	gui.ArchiveDown = env->addButton ( rect<s32>( dim.Width - 510, dim.Height - 530, dim.Width - 440, dim.Height - 514 ), gui.Window,-1, L"down" );
	gui.ArchiveDown->setToolTipText ( L"Arrange Archive Look-up Hirachy. Move the selected Archive down" );


	gui.ArchiveList = env->addTable ( rect<s32>( 5,dim.Height - 510, dim.Width - 450,dim.Height - 410 ), gui.Window  );
	gui.ArchiveList->addColumn ( L"Type", 0 );
	gui.ArchiveList->addColumn ( L"Real File Path", 1 );
	gui.ArchiveList->setColumnWidth ( 0, 60 );
	gui.ArchiveList->setColumnWidth ( 1, 284 );
	gui.ArchiveList->setToolTipText ( L"Show the attached Archives" );


	env->addStaticText ( L"Maps:", rect<s32>( 5, dim.Height - 400, dim.Width - 450,dim.Height - 380 ),false, false, gui.Window, -1, false );
	gui.MapList = env->addListBox ( rect<s32>( 5,dim.Height - 380, dim.Width - 450,dim.Height - 40  ), gui.Window, -1, true  );
	gui.MapList->setToolTipText ( L"Show the current Maps in all Archives.\n Double-Click the Map to start the level" );


	// create a visible Scene Tree
	env->addStaticText ( L"Scenegraph:", rect<s32>( dim.Width - 400, dim.Height - 400, dim.Width - 5,dim.Height - 380 ),false, false, gui.Window, -1, false );
	gui.SceneTree = env->addTreeView(	rect<s32>( dim.Width - 400, dim.Height - 380, dim.Width - 5, dim.Height - 40 ),
									gui.Window, -1, true, true, false );
	gui.SceneTree->setToolTipText ( L"Show the current Scenegraph" );
	gui.SceneTree->getRoot()->clearChildren();
	addSceneTreeItem ( Game->Device->getSceneManager()->getRootSceneNode(), gui.SceneTree->getRoot() );


	IGUIImageList* imageList = env->createImageList(	driver->getTexture ( "iconlist.png" ),
										dimension2di( 32, 32 ), true );

	if ( imageList )
	{
		gui.SceneTree->setImageList( imageList );
		imageList->drop ();
	}


	// load the engine logo
	gui.Logo = env->addImage( driver->getTexture("irrlichtlogo3.png"), position2d<s32>(5, 16 ), true, 0 );
	gui.Logo->setToolTipText ( L"The great Irrlicht Engine" );

	AddArchive ( "" );
	*/
}



void BEventHandler::dropMap ()
{
	IVideoDriver * driver = Game->Device->getVideoDriver();

	driver->removeAllHardwareBuffers ();
	driver->removeAllTextures ();

	Player[0].shutdown ();


	dropElement ( ItemParent );
	dropElement ( ShaderParent );
	dropElement ( UnresolvedParent );
	dropElement ( FogParent );
	dropElement ( BulletParent );


	Impacts.clear();

	if ( Meta )
	{
		Meta = 0;
	}

	dropElement ( MapParent );
	dropElement ( SkyNode );

	// clean out meshes, because textures are invalid
	// TODO: better texture handling;-)
	IMeshCache *cache = Game->Device->getSceneManager ()->getMeshCache();
	cache->clear ();
	Mesh = 0;
}




void BEventHandler::LoadMap ( const stringw &mapName, s32 collision )
{
	if ( 0 == mapName.size() )
		return;

	dropMap ();

	IFileSystem *fs = Game->Device->getFileSystem();
	ISceneManager *smgr = Game->Device->getSceneManager ();

	IReadFile* file = fs->createMemoryReadFile(&Game->loadParam,
				sizeof(Game->loadParam), L"levelparameter.cfg", false);

	// load cfg file
	smgr->getMesh( file );
	file->drop ();

	// load the actual map
	Mesh = (IQ3LevelMesh*) smgr->getMesh(mapName);
	if ( 0 == Mesh ) return;

	/*
		add the geometry mesh to the Scene ( polygon & patches )
		The Geometry mesh is optimised for faster drawing
	*/

	IMesh *geometry = Mesh->getMesh(E_Q3_MESH_GEOMETRY);
	if ( 0 == geometry || geometry->getMeshBufferCount() == 0)
		return;

	Game->CurrentMapName = mapName;

	//create a collision list
	Meta = 0;

	ITriangleSelector * selector = 0;
	if (collision)
		Meta = smgr->createMetaTriangleSelector();

	//IMeshBuffer *b0 = geometry->getMeshBuffer(0);
	//s32 minimalNodes = b0 ? core::s32_max ( 2048, b0->getVertexCount() / 32 ) : 2048;
	s32 minimalNodes = 2048;

	MapParent = smgr->addOctreeSceneNode(geometry, 0, -1, minimalNodes);
	MapParent->setName ( mapName );
	if ( Meta )
	{
		selector = smgr->createOctreeTriangleSelector( geometry,MapParent, minimalNodes);
		//selector = smgr->createTriangleSelector ( geometry, MapParent );
		Meta->addTriangleSelector( selector);
		selector->drop ();
	}

	// logical parent for the items
	ItemParent = smgr->addEmptySceneNode();
	if ( ItemParent )
		ItemParent->setName ( "Item Container" );

	ShaderParent = smgr->addEmptySceneNode();
	if ( ShaderParent )
		ShaderParent->setName ( "Shader Container" );

	UnresolvedParent = smgr->addEmptySceneNode();
	if ( UnresolvedParent )
		UnresolvedParent->setName ( "Unresolved Container" );

	FogParent = smgr->addEmptySceneNode();
	if ( FogParent )
		FogParent->setName ( "Fog Container" );

	// logical parent for the bullets
	BulletParent = smgr->addEmptySceneNode();
	if ( BulletParent )
		BulletParent->setName ( "Bullet Container" );

	/*
		now construct SceneNodes for each Shader
		The Objects are stored in the quake mesh E_Q3_MESH_ITEMS
		and the Shader ID is stored in the MaterialParameters
		mostly dark looking skulls and moving lava.. or green flashing tubes?
	*/
	Q3ShaderFactory ( Game->loadParam, Game->Device, Mesh, E_Q3_MESH_ITEMS,ShaderParent, Meta, false );
	Q3ShaderFactory ( Game->loadParam, Game->Device, Mesh, E_Q3_MESH_FOG,FogParent, 0, false );
	Q3ShaderFactory ( Game->loadParam, Game->Device, Mesh, E_Q3_MESH_UNRESOLVED,UnresolvedParent, Meta, true );

	/*
		Now construct Models from Entity List
	*/
	Q3ModelFactory ( Game->loadParam, Game->Device, Mesh, ItemParent, false );
}


void BEventHandler::addSceneTreeItem( ISceneNode * parent, IGUITreeViewNode* nodeParent)
{
	IGUITreeViewNode* node;
	wchar_t msg[128];

	s32 imageIndex;
	list<ISceneNode*>::ConstIterator it = parent->getChildren().begin();
	for (; it != parent->getChildren().end(); ++it)
	{
		switch ( (*it)->getType () )
		{
			case ESNT_Q3SHADER_SCENE_NODE: imageIndex = 0; break;
			case ESNT_CAMERA: imageIndex = 1; break;
			case ESNT_EMPTY: imageIndex = 2; break;
			case ESNT_MESH: imageIndex = 3; break;
			case ESNT_OCTREE: imageIndex = 3; break;
			case ESNT_ANIMATED_MESH: imageIndex = 4; break;
			case ESNT_SKY_BOX: imageIndex = 5; break;
			case ESNT_BILLBOARD: imageIndex = 6; break;
			case ESNT_PARTICLE_SYSTEM: imageIndex = 7; break;
			case ESNT_TEXT: imageIndex = 8; break;
			default:imageIndex = -1; break;
		}

		if ( imageIndex < 0 )
		{
			swprintf ( msg, 128, L"%hs,%hs",
				Game->Device->getSceneManager ()->getSceneNodeTypeName ( (*it)->getType () ),
				(*it)->getName()
				);
		}
		else
		{
			swprintf ( msg, 128, L"%hs",(*it)->getName() );
		}

		node = nodeParent->addChildBack( msg, 0, imageIndex );

		// Add all Animators
		list<ISceneNodeAnimator*>::ConstIterator ait = (*it)->getAnimators().begin();
		for (; ait != (*it)->getAnimators().end(); ++ait)
		{
			imageIndex = -1;
			swprintf ( msg, 128, L"%hs",
				Game->Device->getSceneManager ()->getAnimatorTypeName ( (*ait)->getType () )
				);

			switch ( (*ait)->getType () )
			{
				case ESNAT_FLY_CIRCLE:
				case ESNAT_FLY_STRAIGHT:
				case ESNAT_FOLLOW_SPLINE:
				case ESNAT_ROTATION:
				case ESNAT_TEXTURE:
				case ESNAT_DELETION:
				case ESNAT_COLLISION_RESPONSE:
				case ESNAT_CAMERA_FPS:
				case ESNAT_CAMERA_MAYA:
				default:
					break;
			}
			node->addChildBack( msg, 0, imageIndex );
		}

		addSceneTreeItem ( *it, node );
	}
}




void BEventHandler::CreatePlayers()
{
	Player[0].create ( Game->Device, Mesh, MapParent, Meta );
}



void CQuake3EventHandler::AddSky( u32 dome, const c8 *texture)
{
	ISceneManager *smgr = Game->Device->getSceneManager ();
	IVideoDriver * driver = Game->Device->getVideoDriver();

	bool oldMipMapState = driver->getTextureCreationFlag(video::ETCF_CREATE_MIP_MAPS);
	driver->setTextureCreationFlag(video::ETCF_CREATE_MIP_MAPS, false);

	if ( 0 == dome )
	{
		// irrlicht order
		//static const c8*p[] = { "ft", "lf", "bk", "rt", "up", "dn" };
		// quake3 order
		static const c8*p[] = { "ft", "rt", "bk", "lf", "up", "dn" };

		u32 i = 0;
		snprintf ( buf, 64, "%s_%s.jpg", texture, p[i] );
		SkyNode = smgr->addSkyBoxSceneNode( driver->getTexture ( buf ), 0, 0, 0, 0, 0 );

		if (SkyNode)
		{
			for ( i = 0; i < 6; ++i )
			{
				snprintf ( buf, 64, "%s_%s.jpg", texture, p[i] );
				SkyNode->getMaterial(i).setTexture ( 0, driver->getTexture ( buf ) );
			}
		}
	}
	else
	if ( 1 == dome )
	{
		snprintf ( buf, 64, "%s.jpg", texture );
		SkyNode = smgr->addSkyDomeSceneNode(
				driver->getTexture( buf ), 32,32,
				1.f, 1.f, 1000.f, 0, 11);
	}
	else
	if ( 2 == dome )
	{
		snprintf ( buf, 64, "%s.jpg", texture );
		SkyNode = smgr->addSkyDomeSceneNode(
				driver->getTexture( buf ), 16,8,
				0.95f, 2.f, 1000.f, 0, 11);
	}

	if (SkyNode)
		SkyNode->setName("Skydome");
	//SkyNode->getMaterial(0).ZBuffer = video::EMDF_DEPTH_LESS_EQUAL;

	driver->setTextureCreationFlag(video::ETCF_CREATE_MIP_MAPS, oldMipMapState);
}



void BEventHandler::SetGUIActive( s32 command)
{
	bool inputState = false;

	ICameraSceneNode * camera = Game->Device->getSceneManager()->getActiveCamera ();

	switch ( command )
	{
		case 0: Game->guiActive = 0; inputState = !Game->guiActive; break;
		case 1: Game->guiActive = 1; inputState = !Game->guiActive;;break;
		case 2: Game->guiActive ^= 1; inputState = !Game->guiActive;break;
		case 3:
			if ( camera )
				inputState = !camera->isInputReceiverEnabled();
			break;
	}

	if ( camera )
	{
		camera->setInputReceiverEnabled ( inputState );
		Game->Device->getCursorControl()->setVisible( !inputState );
	}

	if ( gui.Window )
	{
		gui.Window->setVisible ( Game->guiActive != 0 );
	}

	if ( Game->guiActive &&
			gui.SceneTree && Game->Device->getGUIEnvironment()->getFocus() != gui.SceneTree
		)
	{
		gui.SceneTree->getRoot()->clearChildren();
		addSceneTreeItem ( Game->Device->getSceneManager()->getRootSceneNode(), gui.SceneTree->getRoot() );
	}

	Game->Device->getGUIEnvironment()->setFocus ( Game->guiActive ? gui.Window: 0 );
}


// TO DO tomorrow!!
bool BEventHandler::OnEvent(const SEvent& eve)
{
	if ( eve.EventType == EET_LOG_TEXT_EVENT )
	{
		return false;
	}

	if ( Game->guiActive && eve.EventType == EET_GUI_EVENT )
	{
		if ( eve.GUIEvent.Caller == gui.MapList && eve.GUIEvent.EventType == gui::EGET_LISTBOX_SELECTED_AGAIN )
		{
			s32 selected = gui.MapList->getSelected();
			if ( selected >= 0 )
			{
				stringw loadMap = gui.MapList->getListItem ( selected );
				if ( 0 == MapParent || loadMap != Game->CurrentMapName )
				{
					printf ( "Loading map %ls\n", loadMap.c_str() );
					LoadMap ( loadMap , 1 );
					if ( 0 == Game->loadParam.loadSkyShader )
					{
						AddSky ( 1, "skydome2" );
					}
					CreatePlayers ();
					CreateGUI ();
					SetGUIActive ( 0 );
					return true;
				}
			}
		}
		else
		if ( eve.GUIEvent.Caller == gui.ArchiveRemove && eve.GUIEvent.EventType == gui::EGET_BUTTON_CLICKED )
		{
			Game->Device->getFileSystem()->removeFileArchive( gui.ArchiveList->getSelected() );
			Game->CurrentMapName = "";
			AddArchive ( "" );
		}
		else
		if ( eve.GUIEvent.Caller == gui.ArchiveAdd && eve.GUIEvent.EventType == gui::EGET_BUTTON_CLICKED )
		{
			if ( 0 == gui.ArchiveFileOpen )
			{
				Game->Device->getFileSystem()->setFileListSystem ( FILESYSTEM_NATIVE );
				gui.ArchiveFileOpen = Game->Device->getGUIEnvironment()->addFileOpenDialog ( L"Add Game Archive" , false,gui.Window  );
			}
		}
		else
		if ( eve.GUIEvent.Caller == gui.ArchiveFileOpen && eve.GUIEvent.EventType == gui::EGET_FILE_SELECTED )
		{
			AddArchive ( gui.ArchiveFileOpen->getFileName() );
			gui.ArchiveFileOpen = 0;
		}
		else
		if ( eve.GUIEvent.Caller == gui.ArchiveFileOpen && eve.GUIEvent.EventType == gui::EGET_DIRECTORY_SELECTED )
		{
			AddArchive ( gui.ArchiveFileOpen->getDirectoryName() );
		}
		else
		if ( eve.GUIEvent.Caller == gui.ArchiveFileOpen && eve.GUIEvent.EventType == gui::EGET_FILE_CHOOSE_DIALOG_CANCELLED )
		{
			gui.ArchiveFileOpen = 0;
		}
		else
		if ( ( eve.GUIEvent.Caller == gui.ArchiveUp || eve.GUIEvent.Caller == gui.ArchiveDown ) &&
			eve.GUIEvent.EventType == gui::EGET_BUTTON_CLICKED )
		{
			s32 rel = eve.GUIEvent.Caller == gui.ArchiveUp ? -1 : 1;
			if ( Game->Device->getFileSystem()->moveFileArchive ( gui.ArchiveList->getSelected (), rel ) )
			{
				s32 newIndex = core::s32_clamp ( gui.ArchiveList->getSelected() + rel, 0, gui.ArchiveList->getRowCount() - 1 );
				AddArchive ( "" );
				gui.ArchiveList->setSelected ( newIndex );
				Game->CurrentMapName = "";
			}
		}
		else
		if ( eve.GUIEvent.Caller == gui.VideoDriver && eve.GUIEvent.EventType == gui::EGET_COMBO_BOX_CHANGED )
		{
			Game->deviceParam.DriverType = (E_DRIVER_TYPE) gui.VideoDriver->getItemData ( gui.VideoDriver->getSelected() );
		}
		else
		if ( eve.GUIEvent.Caller == gui.VideoMode && eve.GUIEvent.EventType == gui::EGET_COMBO_BOX_CHANGED )
		{
			u32 val = gui.VideoMode->getItemData ( gui.VideoMode->getSelected() );
			Game->deviceParam.WindowSize.Width = val >> 16;
			Game->deviceParam.WindowSize.Height = val & 0xFFFF;
		}
		else
		if ( eve.GUIEvent.Caller == gui.FullScreen && eve.GUIEvent.EventType == gui::EGET_CHECKBOX_CHANGED )
		{
			Game->deviceParam.Fullscreen = gui.FullScreen->isChecked();
		}
		else
		if ( eve.GUIEvent.Caller == gui.Bit32 && eve.GUIEvent.EventType == gui::EGET_CHECKBOX_CHANGED )
		{
			Game->deviceParam.Bits = gui.Bit32->isChecked() ? 32 : 16;
		}
		else
		if ( eve.GUIEvent.Caller == gui.MultiSample && eve.GUIEvent.EventType == gui::EGET_SCROLL_BAR_CHANGED )
		{
			Game->deviceParam.AntiAlias = gui.MultiSample->getPos();
		}
		else
		if ( eve.GUIEvent.Caller == gui.Tesselation && eve.GUIEvent.EventType == gui::EGET_SCROLL_BAR_CHANGED )
		{
			Game->loadParam.patchTesselation = gui.Tesselation->getPos ();
		}
		else
		if ( eve.GUIEvent.Caller == gui.Gamma && eve.GUIEvent.EventType == gui::EGET_SCROLL_BAR_CHANGED )
		{
			Game->GammaValue = gui.Gamma->getPos () * 0.01f;
			Game->Device->setGammaRamp ( Game->GammaValue, Game->GammaValue, Game->GammaValue, 0.f, 0.f );
		}
		else
		if ( eve.GUIEvent.Caller == gui.SetVideoMode && eve.GUIEvent.EventType == gui::EGET_BUTTON_CLICKED )
		{
			Game->retVal = 2;
			Game->Device->closeDevice();
		}
		else
		if ( eve.GUIEvent.Caller == gui.Window && eve.GUIEvent.EventType == gui::EGET_ELEMENT_CLOSED )
		{
			Game->Device->closeDevice();
		}
		else
		if ( eve.GUIEvent.Caller == gui.Collision && eve.GUIEvent.EventType == gui::EGET_CHECKBOX_CHANGED )
		{
			// set fly through active
			Game->flyTroughState ^= 1;
			Player[0].cam()->setAnimateTarget ( Game->flyTroughState == 0 );

			printf ( "collision %d\n", Game->flyTroughState == 0 );
		}
		else
		if ( eve.GUIEvent.Caller == gui.Visible_Map && eve.GUIEvent.EventType == gui::EGET_CHECKBOX_CHANGED )
		{
			bool v = gui.Visible_Map->isChecked();

			if ( MapParent )
			{
				printf ( "static node set visible %d\n",v );
				MapParent->setVisible ( v );
			}
		}
		else
		if ( eve.GUIEvent.Caller == gui.Visible_Shader && eve.GUIEvent.EventType == gui::EGET_CHECKBOX_CHANGED )
		{
			bool v = gui.Visible_Shader->isChecked();

			if ( ShaderParent )
			{
				printf ( "shader node set visible %d\n",v );
				ShaderParent->setVisible ( v );
			}
		}
		else
		if ( eve.GUIEvent.Caller == gui.Visible_Skydome && eve.GUIEvent.EventType == gui::EGET_CHECKBOX_CHANGED )
		{
			if ( SkyNode )
			{
				bool v = !SkyNode->isVisible();
				printf ( "skynode set visible %d\n",v );
				SkyNode->setVisible ( v );
			}
		}
		else
		if ( eve.GUIEvent.Caller == gui.Respawn && eve.GUIEvent.EventType == gui::EGET_BUTTON_CLICKED )
		{
			Player[0].respawn ();
		}

		return false;
	}

	// fire
	if ((eve.EventType == EET_KEY_INPUT_EVENT && eve.KeyInput.Key == KEY_SPACE &&
		eve.KeyInput.PressedDown == false) ||
		(eve.EventType == EET_MOUSE_INPUT_EVENT && eve.MouseInput.Event == EMIE_LMOUSE_LEFT_UP)
		)
	{
		ICameraSceneNode * camera = Game->Device->getSceneManager()->getActiveCamera ();
		if ( camera && camera->isInputReceiverEnabled () )
		{
			useItem( Player + 0 );
		}
	}

	// gui active
	if ((eve.EventType == EET_KEY_INPUT_EVENT && eve.KeyInput.Key == KEY_F1 &&
		eve.KeyInput.PressedDown == false) ||
		(eve.EventType == EET_MOUSE_INPUT_EVENT && eve.MouseInput.Event == EMIE_RMOUSE_LEFT_UP)
		)
	{
		SetGUIActive ( 2 );
	}

	// check if user presses the key
	if ( eve.EventType == EET_KEY_INPUT_EVENT && eve.KeyInput.PressedDown == false)
	{
		// Escape toggles camera Input
		if ( eve.KeyInput.Key == irr::KEY_ESCAPE )
		{
			SetGUIActive ( 3 );
		}
		else
		if (eve.KeyInput.Key == KEY_F11)
		{
			// screenshot are taken without gamma!
			IImage* image = Game->Device->getVideoDriver()->createScreenShot();
			if (image)
			{
				core::vector3df pos;
				core::vector3df rot;
				ICameraSceneNode * cam = Game->Device->getSceneManager()->getActiveCamera ();
				if ( cam )
				{
					pos = cam->getPosition ();
					rot = cam->getRotation ();
				}

				static const c8 *dName[] = { "null", "software", "burning",
					"d3d8", "d3d9", "opengl" };

				snprintf(buf, 256, "%s_%ls_%.0f_%.0f_%.0f_%.0f_%.0f_%.0f.jpg",
						dName[Game->Device->getVideoDriver()->getDriverType()],
						Game->CurrentMapName.c_str(),
						pos.X, pos.Y, pos.Z,
						rot.X, rot.Y, rot.Z
						);
				path filename ( buf );
				filename.replace ( '/', '_' );
				printf ( "screenshot : %s\n", filename.c_str() );
				Game->Device->getVideoDriver()->writeImageToFile(image, filename, 100 );
				image->drop();
			}
		}
		else
		if (eve.KeyInput.Key == KEY_F9)
		{
			s32 value = EDS_OFF;

			Game->debugState = ( Game->debugState + 1 ) & 3;

			switch ( Game->debugState )
			{
				case 1: value = EDS_NORMALS | EDS_MESH_WIRE_OVERLAY | EDS_BBOX_ALL; break;
				case 2: value = EDS_NORMALS | EDS_MESH_WIRE_OVERLAY | EDS_SKELETON; break;
			}
/*
			// set debug map data on/off
			debugState = debugState == EDS_OFF ?
				EDS_NORMALS | EDS_MESH_WIRE_OVERLAY | EDS_BBOX_ALL:
				EDS_OFF;
*/
			if ( ItemParent )
			{
				list<ISceneNode*>::ConstIterator it = ItemParent->getChildren().begin();
				for (; it != ItemParent->getChildren().end(); ++it)
				{
					(*it)->setDebugDataVisible ( value );
				}
			}

			if ( ShaderParent )
			{
				list<ISceneNode*>::ConstIterator it = ShaderParent->getChildren().begin();
				for (; it != ShaderParent->getChildren().end(); ++it)
				{
					(*it)->setDebugDataVisible ( value );
				}
			}

			if ( UnresolvedParent )
			{
				list<ISceneNode*>::ConstIterator it = UnresolvedParent->getChildren().begin();
				for (; it != UnresolvedParent->getChildren().end(); ++it)
				{
					(*it)->setDebugDataVisible ( value );
				}
			}

			if ( FogParent )
			{
				list<ISceneNode*>::ConstIterator it = FogParent->getChildren().begin();
				for (; it != FogParent->getChildren().end(); ++it)
				{
					(*it)->setDebugDataVisible ( value );
				}
			}

			if ( SkyNode )
			{
				SkyNode->setDebugDataVisible ( value );
			}

		}
		else
		if (eve.KeyInput.Key == KEY_F8)
		{
			// set gravity on/off
			Game->gravityState ^= 1;
			Player[0].cam()->setGravity ( getGravity ( Game->gravityState ? "earth" : "none" ) );
			printf ( "gravity %s\n", Game->gravityState ? "earth" : "none" );
		}
		else
		if (eve.KeyInput.Key == KEY_F7)
		{
			// set fly through active
			Game->flyTroughState ^= 1;
			Player[0].cam()->setAnimateTarget ( Game->flyTroughState == 0 );
			if ( gui.Collision )
				gui.Collision->setChecked ( Game->flyTroughState == 0 );

			printf ( "collision %d\n", Game->flyTroughState == 0 );
		}
		else
		if (eve.KeyInput.Key == KEY_F2)
		{
			Player[0].respawn ();
		}
		else
		if (eve.KeyInput.Key == KEY_F3)
		{
			if ( MapParent )
			{
				bool v = !MapParent->isVisible ();
				printf ( "static node set visible %d\n",v );
				MapParent->setVisible ( v );
				if ( gui.Visible_Map )
					gui.Visible_Map->setChecked ( v );
			}
		}
		else
		if (eve.KeyInput.Key == KEY_F4)
		{
			if ( ShaderParent )
			{
				bool v = !ShaderParent->isVisible ();
				printf ( "shader node set visible %d\n",v );
				ShaderParent->setVisible ( v );
				if ( gui.Visible_Shader )
					gui.Visible_Shader->setChecked ( v );
			}
		}
		else
		if (eve.KeyInput.Key == KEY_F5)
		{
			if ( FogParent )
			{
				bool v = !FogParent->isVisible ();
				printf ( "fog node set visible %d\n",v );
				FogParent->setVisible ( v );
				if ( gui.Visible_Fog )
					gui.Visible_Fog->setChecked ( v );
			}

		}
		else
		if (eve.KeyInput.Key == KEY_F6)
		{
			if ( UnresolvedParent )
			{
				bool v = !UnresolvedParent->isVisible ();
				printf ( "unresolved node set visible %d\n",v );
				UnresolvedParent->setVisible ( v );
				if ( gui.Visible_Unresolved )
					gui.Visible_Unresolved->setChecked ( v );
			}
		}
	}

	// check if user presses the key C ( for crouch)
	if ( eve.EventType == EET_KEY_INPUT_EVENT && eve.KeyInput.Key == KEY_KEY_X )
	{
		// crouch
		ISceneNodeAnimatorCollisionResponse *anim = Player[0].cam ();
		if ( anim && 0 == Game->flyTroughState )
		{
			if ( false == eve.KeyInput.PressedDown )
			{
				// stand up
				anim->setEllipsoidRadius (  vector3df(30,45,30) );
				anim->setEllipsoidTranslation ( vector3df(0,40,0));

			}
			else
			{
				// on your knees
				anim->setEllipsoidRadius (  vector3df(30,20,30) );
				anim->setEllipsoidTranslation ( vector3df(0,20,0));
			}
			return true;
		}
	}
	return false;
}
