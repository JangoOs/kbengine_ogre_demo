#pragma warning(disable:4251) 
#pragma warning(disable:4244) 
#pragma warning(disable:4193)
#pragma warning(disable:4275) 

#include "space_world.h"
#include "space_login.h"
#include "DotSceneLoader.h"
#include "PagedGeometry.h"
#include "GrassLoader.h"
#include "BatchPage.h"
#include "ImpostorPage.h"
#include "TreeLoader3D.h"
#include "OgreApplication.h"
#include "EntityComplex.h"
#include "EntitySimple.h"

#include "../../kbengine_dll/kbengine_dll.h"

// ��¼ѡ��
KBEngine::ENTITY_ID g_tickSelTargetID = -1;

//-------------------------------------------------------------------------------------
SpaceWorld::SpaceWorld(Ogre::Root *pOgreRoot, Ogre::RenderWindow* pRenderWin, 
		OIS::InputManager* pInputMgr, OgreBites::SdkTrayManager* pTrayMgr)
:   Space(pOgreRoot, pRenderWin, pInputMgr, pTrayMgr),
	mLoader(0),
    mTerrainImported(true),
    mSceneProjectDir(Ogre::StringUtil::BLANK),
    mHelpInfo(Ogre::StringUtil::BLANK),
    mFly(false),
	mPlayerPtr(0),
	mTargetPtr(0),
	mMouseTargetPtr(0),
	mEntities(),
	serverClosed_(false),
	showCloseButton_(false),
	createdReliveButton_(false),
	pDecalObj_(NULL),
	pSelDecalObj_(NULL),
	selPos_(),
	showSelPosDecal_(false),
	pLensflare_(0)
{
    mHelpInfo = Ogre::String("Use [W][A][S][D] keys for movement.\nKeys [1]-[9] to switch between cameras.\n[0] toggles SceneNode debug visuals.\n\nPress [C] to toggle clamp to terrain (gravity).\n\n[G] toggles the detail panel.\n[R] cycles polygonModes (Solid/Wireframe/Points).\n[T] cycles various filtering.\n\n\nPress [ESC] to quit.");
}

//-------------------------------------------------------------------------------------
SpaceWorld::~SpaceWorld(void)
{
	if(showCloseButton_)
		mTrayMgr->destroyWidget("backlogin");
	
	if(createdReliveButton_)
		mTrayMgr->destroyWidget("relive");

	if(pLensflare_)
		delete pLensflare_;

	if(pDecalObj_)
		delete pDecalObj_;

	if(pSelDecalObj_)
		delete pSelDecalObj_;

	mSceneMgr->destroyCamera("mainCamera");

	mActiveCamera = NULL;
	delete mLoader;
	
	mMouseTargetPtr = mTargetPtr = mPlayerPtr = NULL;
	mEntities.clear();
}

//-------------------------------------------------------------------------------------
void SpaceWorld::addSpaceGeometryMapping(std::string respath)
{
	mSceneProjectDir = std::string(kbe_getPyUserResPath());

	if(mSceneProjectDir[mSceneProjectDir.size() - 1] != '\\' && mSceneProjectDir[mSceneProjectDir.size() - 1] != '/')
		mSceneProjectDir += "\\";

	mSceneProjectDir += respath + "/Scenes/Scene";

	loadSpaceGeometryMapping();
}

//-------------------------------------------------------------------------------------
void SpaceWorld::loadSpaceGeometryMapping()
{
	if(mSceneProjectDir == Ogre::StringUtil::BLANK)
		return;

	/*
    rapidxml::xml_document<> XMLDoc;    // character type defaults to char
    rapidxml::xml_node<>* XMLRoot;

    std::ifstream fp;
    
    fp.open("scenes.xml", std::ios::in | std::ios::binary);
    Ogre::DataStreamPtr stream(OGRE_NEW Ogre::FileStreamDataStream("scenes.xml", &fp, false));
    char* sampleAppConfig = strdup(stream->getAsString().c_str());
    XMLDoc.parse<0>(sampleAppConfig);
    XMLRoot = XMLDoc.first_node("Scene1");

    mSceneProjectDir = Ogre::String(XMLRoot->first_attribute("projectDir")->value());
    mSceneFile = Ogre::String(XMLRoot->first_attribute("scene")->value());
	*/
	
	// mSceneProjectDir = Ogre::String("../../res/Media/Scenes/Scene1");
	
    // add sample project directory to the resource paths
    Ogre::ResourceGroupManager::getSingleton().addResourceLocation(
        "" + mSceneProjectDir, "FileSystem", "Space");

    Ogre::ResourceGroupManager::getSingleton().addResourceLocation(
        "" + mSceneProjectDir + "/Materials", "FileSystem", "Space");
    Ogre::ResourceGroupManager::getSingleton().addResourceLocation(
        "" + mSceneProjectDir + "/Models", "FileSystem", "Space");
    Ogre::ResourceGroupManager::getSingleton().addResourceLocation(
        "" + mSceneProjectDir + "/Terrain", "FileSystem", "Space");

	mTrayMgr->showLoadingBar(1, 0);
	Ogre::ResourceGroupManager::getSingleton().initialiseResourceGroup("Space");
	mTrayMgr->hideLoadingBar();
	
    mLoader = new DotSceneLoader(SCENE_MASK);
    mLoader->parseDotScene(Ogre::String("Scene.scene"), "Space", mSceneMgr);

    for(unsigned int ij = 0;ij < mLoader->mPGHandles.size();ij++)
    {
        mLoader->mPGHandles[ij]->setCamera(mActiveCamera);
    }
}

//-------------------------------------------------------------------------------------
void SpaceWorld::setupResources(void)
{
}

//-------------------------------------------------------------------------------------
void SpaceWorld::createScene(void)
{
	mSceneMgr = mRoot->createSceneManager(Ogre::ST_GENERIC);
	
	loadSpaceGeometryMapping();
	
	mTrayMgr->showCursor();
	mTrayMgr->showFrameStats(OgreBites::TL_BOTTOMLEFT);
	mTrayMgr->showBackdrop();
	mTrayMgr->showLogo(OgreBites::TL_BOTTOMRIGHT);

	// set shadow properties
	mSceneMgr->setShadowTechnique(SHADOWTYPE_TEXTURE_MODULATIVE);
	mSceneMgr->setShadowColour(ColourValue(0.5, 0.5, 0.5));
	mSceneMgr->setShadowTextureSize(1024);
	mSceneMgr->setShadowTextureCount(1);

	mActiveCamera = mSceneMgr->createCamera("mainCamera");
	// Create one viewport, entire window
	Ogre::Viewport* vp = mWindow->addViewport(mActiveCamera);
	vp->setBackgroundColour(Ogre::ColourValue(0,0,0));

	// Alter the camera aspect ratio to match the viewport
	mActiveCamera->setAspectRatio(
		Ogre::Real(vp->getActualWidth()) / Ogre::Real(vp->getActualHeight()));

    mWindow->getViewport(0)->setCamera(mActiveCamera);

	mActiveCamera->setNearClipDistance(0.1f);
	mActiveCamera->setFarClipDistance(30000);

	mCameraMan = new OgreBites::SdkCameraMan(mActiveCamera);   // create a default camera controller
	mCameraMan->setTopSpeed(7.0f);
    mCameraMan->setCamera(mActiveCamera);
	mCameraMan->setStyle(OgreBites::CS_MANUAL);
	OgreApplication::getSingleton().setCurrCameraMan(mCameraMan);

	mRaySceneQuery = mSceneMgr->createRayQuery(Ray());

	pDecalObj_ = new DecalObject("Examples/Decal", this, 1.0f, 1.5f);
	pSelDecalObj_ = new DecalObject("Examples/Sel_Decal", this, 1.0f, 1.5f);

	pLensflare_ = new LensFlare(Ogre::Vector3(3000, 2000, 0), mActiveCamera, mSceneMgr);
}

//----------------------------------------------------------------------------------------
float SpaceWorld::getPositionHeight(const Ogre::Vector3& pos)
{ 
	if(!mLoader)
		return 0.f;

	return mLoader->getTerrainGroup()->getHeightAtWorldPosition(pos); 
}

//-------------------------------------------------------------------------------------
bool SpaceWorld::frameRenderingQueued(const Ogre::FrameEvent& evt)
{
	if(mLoader)
	{
		if (!mLoader->getTerrainGroup()->isDerivedDataUpdateInProgress())
		{
			if (mTerrainImported)
			{
				mLoader->getTerrainGroup()->saveAllTerrains(true);
				mTerrainImported = false;
			}
		}

		for(unsigned int ij = 0;ij < mLoader->mPGHandles.size();ij++)
		{
			mLoader->mPGHandles[ij]->update();
		}
	
		ENTITIES::iterator iter = mEntities.begin();
		for(; iter != mEntities.end(); iter++)
		{
			if(!iter->second->inWorld())
				continue;

			iter->second->addTime(evt.timeSinceLastFrame);
		}
		
		if(mTargetPtr)
		{
			pSelDecalObj_->update(evt.timeSinceLastFrame);
		}

		if(showSelPosDecal_)
		{
			pDecalObj_->update(evt.timeSinceLastFrame);
		}

		if(mPlayerPtr) 
		{
			// kbe_lock();

			kbe_updateVolatile(g_tickSelTargetID, 
								mPlayerPtr->getPosition().x, 
								mPlayerPtr->getPosition().y, 
								mPlayerPtr->getPosition().z,
								mPlayerPtr->getDirection().x, 
								mPlayerPtr->getDirection().y, 
								mPlayerPtr->getDirection().z);

			g_tickSelTargetID = -1;
			// kbe_unlock();
		}
	}

	// �������ر�
	if(serverClosed_)
	{
		mTrayMgr->createButton(OgreBites::TL_CENTER, "backlogin", "back login", 120);
		serverClosed_ = false;
		showCloseButton_ = true;

		if(createdReliveButton_)
		{
			createdReliveButton_ = false;
			mTrayMgr->destroyWidget("relive");
		}
	}
	else
	{
		if(mPlayerPtr && !showCloseButton_)
		{
			if(!createdReliveButton_)
			{
				// ��������ʾ���ť
				if(mPlayerPtr->getState() == 1)
				{
					mTrayMgr->createButton(OgreBites::TL_CENTER, "relive", "relive", 120);
					createdReliveButton_ = true;
				}
			}
			else
			{
				if(mPlayerPtr->getState() != 1)
				{
					createdReliveButton_ = false;
					mTrayMgr->destroyWidget("relive");
				}
			}
		}
	}

	if(mLoader && pLensflare_)
		pLensflare_->update();

    return true;
}

//-------------------------------------------------------------------------------------
void SpaceWorld::buttonHit(OgreBites::Button* button)
{
	if(button->getCaption() == "back login" && showCloseButton_)
	{
		OgreApplication::getSingleton().changeSpace(new SpaceLogin(mRoot, mWindow, mInputManager, mTrayMgr));
		kbe_fireEvent("reset", NULL);
	}
	else if(button->getCaption() == "relive")
	{
		kbe_fireEvent("relive", NULL);
	}
}

//-------------------------------------------------------------------------------------
bool SpaceWorld::keyPressed( const OIS::KeyEvent &arg )
{
    if (arg.key == OIS::KC_H || arg.key == OIS::KC_F1)   // toggle visibility of help dialog
    {
        if (!mTrayMgr->isDialogVisible()) mTrayMgr->showOkDialog("Help", mHelpInfo);
        else mTrayMgr->closeDialog();
    }
    else if (arg.key == OIS::KC_0)   // toggle scenenode debug renderables
    {
        mSceneMgr->setDisplaySceneNodes(!mSceneMgr->getDisplaySceneNodes());
    }
    else if (arg.key == OIS::KC_F2)   // toggle scenenode debug renderables
    {
		static bool b = false;
		b = !b;

		ENTITIES::iterator iter = mEntities.begin();
		for(; iter != mEntities.end(); iter++)
		{
			iter->second->showBoundingBoxes(b);
		}

    }
    else if (arg.key == OIS::KC_C)   // toggle fly/walk
    {
        mFly = !mFly;
    }

	if(mPlayerPtr) mPlayerPtr->injectKeyDown(arg);

	if(!mLoader)
		return true;

    return true; 
}

//-------------------------------------------------------------------------------------
bool SpaceWorld::keyReleased(const OIS::KeyEvent &arg)
{
	if(mPlayerPtr) mPlayerPtr->injectKeyUp(arg);

	if(!mLoader)
		return true;

    return true;
}

//-------------------------------------------------------------------------------------
bool SpaceWorld::mouseMoved( const OIS::MouseEvent &arg )
{
    if(mPlayerPtr) mPlayerPtr->injectMouseMove(arg);

	if(!mLoader)
		return false;

	Ogre::Ray mouseRay = mActiveCamera->getCameraToViewportRay(arg.state.X.abs / float(arg.state.width),
		arg.state.Y.abs / float(arg.state.height));

	Ogre::Entity* rayResult = NULL;
	Ogre::Vector3 hitPoint;
	if(pickEntity(mRaySceneQuery, mouseRay, &rayResult, ENTITY_MASK, 0, hitPoint, "x", 5000))
	{
		Ogre::String key = rayResult->getName();
		Ogre::StringVector vec = Ogre::StringUtil::split(key, "_");
		if(vec.size() >= 2)
		{
			key = vec[0];
			KBEngine::ENTITY_ID eid = Ogre::StringConverter::parseInt(key);

			ENTITIES::iterator iter = mEntities.find(eid);
			if(iter != mEntities.end())
			{
				if(mMouseTargetPtr != iter->second.get())
				{
					if(mMouseTargetPtr)
						mMouseTargetPtr->setHighlighted(false);

					mMouseTargetPtr = iter->second.get();
					mMouseTargetPtr->setHighlighted(true);
				}
				else
				{
				}
			}
		}
	}
	else
	{
		if(mMouseTargetPtr)
			mMouseTargetPtr->setHighlighted(false);

		mMouseTargetPtr = NULL;
	}

    return false;
}

//-------------------------------------------------------------------------------------
bool SpaceWorld::mousePressed( const OIS::MouseEvent &arg, OIS::MouseButtonID id )
{
	if(mPlayerPtr) mPlayerPtr->injectMouseDown(arg, id);

	if(!mLoader)
		return false;

	if(id == OIS::MB_Left)
	{
		Ogre::Ray mouseRay = mActiveCamera->getCameraToViewportRay(arg.state.X.abs / float(arg.state.width),
			arg.state.Y.abs / float(arg.state.height));

		Ogre::Entity* rayResult = NULL;
		Ogre::Vector3 hitPoint;
		if(pickEntity(mRaySceneQuery, mouseRay, &rayResult, ENTITY_MASK, 0, hitPoint, "x", 5000))
		{
			Ogre::String key = rayResult->getName();
			Ogre::StringVector vec = Ogre::StringUtil::split(key, "_");
			if(vec.size() >= 2)
			{
				key = vec[0];
				KBEngine::ENTITY_ID eid = Ogre::StringConverter::parseInt(key);

				ENTITIES::iterator iter = mEntities.find(eid);
				if(iter != mEntities.end())
				{
					if(eid != mPlayerPtr->id())
					{
						g_tickSelTargetID = eid;
						if(mTargetPtr != iter->second.get())
						{
							mTargetPtr = iter->second.get();
							pSelDecalObj_->moveDecalTo(mTargetPtr->getPosition());
						}
						else
						{
							// �ٴ�ѡ���˸�entity
						}
					}
				}
			}
		}
		else
		{
			Ogre::Ray ray = mTrayMgr->getCursorRay(mActiveCamera);
			Ogre::TerrainGroup::RayResult rayResult = mLoader->getTerrainGroup()->rayIntersects(ray);
			if (rayResult.hit)
			{
				/*
				if(m_player_ != NULL)
				{
					m_player_->moveTo(target, m_player_->getSpeed());
					KBEngine::onMousePressedInWorld(target.x, target.y, target.z);
				}
				*/

				showSelPosDecal_ = true;
				selPos_ = rayResult.position;
				pDecalObj_->moveDecalTo(selPos_);
			}
		}

	}
    return false;
}

//-------------------------------------------------------------------------------------
bool SpaceWorld::mouseReleased( const OIS::MouseEvent &arg, OIS::MouseButtonID id )
{
	if(!mLoader)
		return true;

    return true;
}

//-------------------------------------------------------------------------------------
void SpaceWorld::kbengine_onEvent(const KBEngine::EventData* lpEventData)
{
	switch(lpEventData->id)
	{
	case CLIENT_EVENT_ADDSPACEGEOMAPPING:
		{
			const KBEngine::EventData_AddSpaceGEOMapping* pEventData = static_cast<const KBEngine::EventData_AddSpaceGEOMapping*>(lpEventData);
			KBEngine::SPACE_ID spaceID = pEventData->spaceID;
			addSpaceGeometryMapping(pEventData->respath);
		}
		break;
	case CLIENT_EVENT_CREATEDENTITY:
		{
			const KBEngine::EventData_CreatedEntity* pEventData_createEntity = static_cast<const KBEngine::EventData_CreatedEntity*>(lpEventData);
			KBEngine::ENTITY_ID eid = pEventData_createEntity->entityID;
			KBEntity* pEntity = NULL;
			
			if(kbe_playerID() == eid)
				pEntity = new EntityComplex(this, eid);
			else
				pEntity = new EntitySimple(this, eid);

			mEntities[eid].reset(pEntity);
			//pEntity->visable(false);
		}
		break;
	case CLIENT_EVENT_ENTERWORLD:
		{
			const KBEngine::EventData_EnterWorld* pEventData_EnterWorld = static_cast<const KBEngine::EventData_EnterWorld*>(lpEventData);
			KBEngine::ENTITY_ID eid = pEventData_EnterWorld->entityID;
			
			ENTITIES::iterator iter = mEntities.find(eid);
			if(iter == mEntities.end())
				break;
			
			KBEntity* pEntity = iter->second.get();
			if(kbe_playerID() == eid)
			{
				break;
			}

			pEntity->setup(mSceneMgr);

			pEntity->setPosition(pEventData_EnterWorld->x, pEventData_EnterWorld->y, pEventData_EnterWorld->z);
			pEntity->setDestPosition(pEventData_EnterWorld->x, pEventData_EnterWorld->y, pEventData_EnterWorld->z);
			pEntity->scale(0.3f, 0.3f, 0.3f);
			pEntity->setMoveSpeed(pEventData_EnterWorld->speed);
			pEntity->setDestDirection(pEventData_EnterWorld->yaw, pEventData_EnterWorld->pitch, pEventData_EnterWorld->roll);
			pEntity->setDirection(pEventData_EnterWorld->yaw, pEventData_EnterWorld->pitch, pEventData_EnterWorld->roll);

			pEntity->inWorld(true);

			//pEntity->visable(true);
		}
		break;
	case CLIENT_EVENT_ENTERSPACE:
		{
			const KBEngine::EventData_EnterSpace* pEventData_EnterSpace = static_cast<const KBEngine::EventData_EnterSpace*>(lpEventData);
			KBEngine::ENTITY_ID eid = pEventData_EnterSpace->entityID;
			
			ENTITIES::iterator iter = mEntities.find(eid);
			if(iter == mEntities.end())
				break;
			
			KBEntity* pEntity = iter->second.get();

			pEntity->setup(mSceneMgr);

			pEntity->setPosition(pEventData_EnterSpace->x, pEventData_EnterSpace->y, pEventData_EnterSpace->z);
			pEntity->setDestPosition(pEventData_EnterSpace->x, pEventData_EnterSpace->y, pEventData_EnterSpace->z);
			pEntity->scale(0.3f, 0.3f, 0.3f);
			pEntity->setMoveSpeed(pEventData_EnterSpace->speed);
			pEntity->setDestDirection(pEventData_EnterSpace->yaw, pEventData_EnterSpace->pitch, pEventData_EnterSpace->roll);
			pEntity->setDirection(pEventData_EnterSpace->yaw, pEventData_EnterSpace->pitch, pEventData_EnterSpace->roll);

			if(kbe_playerID() == eid)
			{
				static_cast<EntityComplex*>(pEntity)->setupCamera(mActiveCamera);
				mPlayerPtr = pEntity;
			}
			
			pEntity->inWorld(true);

			//pEntity->visable(true);
		}
		break;
	case CLIENT_EVENT_LEAVEWORLD:
		{
			KBEngine::ENTITY_ID eid = static_cast<const KBEngine::EventData_LeaveWorld*>(lpEventData)->entityID;
			if(kbe_playerID() == eid)
				mPlayerPtr = NULL;
			
			if(mTargetPtr && mTargetPtr->id() == eid)
				mTargetPtr = NULL;
		
			if(mMouseTargetPtr && mMouseTargetPtr->id() == eid)
				mMouseTargetPtr = NULL;
			
			mEntities.erase(eid);
		}
		break;
	case CLIENT_EVENT_LEAVESPACE:
		{
			KBEngine::ENTITY_ID eid = static_cast<const KBEngine::EventData_LeaveSpace*>(lpEventData)->entityID;
			if(kbe_playerID() == eid)
				mPlayerPtr = NULL;
			
			if(mTargetPtr && mTargetPtr->id() == eid)
				mTargetPtr = NULL;
		
			if(mMouseTargetPtr && mMouseTargetPtr->id() == eid)
				mMouseTargetPtr = NULL;
			
			mEntities.erase(eid);
		}
		break;
	case CLIENT_EVENT_POSITION_CHANGED:
		{
			const KBEngine::EventData_PositionChanged* pEventData = static_cast<const KBEngine::EventData_PositionChanged*>(lpEventData);
			KBEngine::ENTITY_ID eid = pEventData->entityID;

			ENTITIES::iterator iter = mEntities.find(eid);
			if(iter == mEntities.end())
				break;
			
			iter->second->setDestPosition(pEventData->x, pEventData->y, pEventData->z);
		}
		break;
	case CLIENT_EVENT_DIRECTION_CHANGED:
		{
			const KBEngine::EventData_DirectionChanged* pEventData = static_cast<const KBEngine::EventData_DirectionChanged*>(lpEventData);
			KBEngine::ENTITY_ID eid = pEventData->entityID;

			ENTITIES::iterator iter = mEntities.find(eid);
			if(iter == mEntities.end())
				break;

			iter->second->setDestDirection(pEventData->yaw, pEventData->pitch, pEventData->roll);
		}
		break;
	case CLIENT_EVENT_POSITION_FORCE:
		{
			const KBEngine::EventData_PositionForce* pEventData = static_cast<const KBEngine::EventData_PositionForce*>(lpEventData);
			KBEngine::ENTITY_ID eid = pEventData->entityID;

			ENTITIES::iterator iter = mEntities.find(eid);
			if(iter == mEntities.end())
				break;
			
			iter->second->setDestPosition(pEventData->x, pEventData->y, pEventData->z);
			iter->second->setPosition(pEventData->x, pEventData->y, pEventData->z);
		}
		break;
	case CLIENT_EVENT_DIRECTION_FORCE:
		{
			const KBEngine::EventData_DirectionForce* pEventData = static_cast<const KBEngine::EventData_DirectionForce*>(lpEventData);
			KBEngine::ENTITY_ID eid = pEventData->entityID;

			ENTITIES::iterator iter = mEntities.find(eid);
			if(iter == mEntities.end())
				break;

			//iter->second->setDirection(pEventData->yaw, pEventData->pitch, pEventData->roll);
		}
		break;
	case CLIENT_EVENT_MOVESPEED_CHANGED:
		{
			const KBEngine::EventData_MoveSpeedChanged* pEventData = static_cast<const KBEngine::EventData_MoveSpeedChanged*>(lpEventData);
			KBEngine::ENTITY_ID eid = pEventData->entityID;
			
			ENTITIES::iterator iter = mEntities.find(eid);
			if(iter == mEntities.end())
				break;

			iter->second->setMoveSpeed(pEventData->speed);
		}
		break;
	case CLIENT_EVENT_SERVER_CLOSED:
		{
			serverClosed_ = true;
		}
		break;
		case CLIENT_EVENT_SCRIPT:
			{
				const KBEngine::EventData_Script* peventdata = static_cast<const KBEngine::EventData_Script*>(lpEventData);
				Json::Reader reader;
				Json::Value root;

				if (!reader.parse(peventdata->datas.c_str(), root))
				{  
					assert(false);
				}

				KBEngine::ENTITY_ID eid = root[Json::Value::UInt(0)].asInt();
				ENTITIES::iterator iter = mEntities.find(eid);
				KBEntity* pEntity = NULL;

				if(iter != mEntities.end())
					pEntity = iter->second.get();

				if(peventdata->name == "set_name")
				{
					if(pEntity == NULL)
						break;

					std::string name = root[1].asString();
					wchar_t wname[1024]; 
					MultiByteToWideChar(CP_UTF8, 0, name.c_str(), -1, wname, 1024);  
					pEntity->setName(wname);																		
				}
				else if(peventdata->name == "set_modelScale")
				{
					if(pEntity == NULL)
						break;

					uint32 scale = root[1].asUInt();	
					pEntity->scale(scale / 100.0);
				}
				else if(peventdata->name == "set_modelID")
				{
					if(pEntity == NULL)
						break;

					uint32 modelID = root[1].asUInt();		
					pEntity->setModelID(modelID);
				}
				else if(peventdata->name == "set_state")
				{
					if(pEntity == NULL)
						break;

					int32 state = root[1].asInt();	
					pEntity->setState(state);
				}
				else if(peventdata->name == "set_HP_Max")
				{
					if(pEntity == NULL)
						break;

					int32 v = root[1].asInt();		
					pEntity->setHPMAX(v);
				}
				else if(peventdata->name == "set_MP_Max")
				{
					if(pEntity == NULL)
						break;

					int32 v = root[1].asInt();		
					pEntity->setMPMAX(v);
				}
				else if(peventdata->name == "recvDamage")
				{
					KBEngine::ENTITY_ID attackerID = root[1].asInt();	
					uint32 skillID = root[2].asUInt();
					uint32 damageType = root[3].asUInt();
					uint32 damage = root[4].asUInt();

					ENTITIES::iterator iter = mEntities.find(attackerID);
						
					KBEntity* attacker = NULL;
					KBEntity* receiver = pEntity;

					if(iter != mEntities.end())
					{
						attacker = iter->second.get();
					}

					if(attacker)
					{
						attacker->attack(receiver, skillID, damageType, damage);
					}

					if(receiver)
					{
						receiver->recvDamage(attacker, skillID, damageType, damage);
					}
				}
			}
			
			break;
	default:
		break;
	};
}
