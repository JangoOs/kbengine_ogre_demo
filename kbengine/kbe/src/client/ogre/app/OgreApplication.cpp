#pragma warning(disable:4251) 
#include "OgreApplication.h"
#include "DotSceneLoader.h"
#include "PagedGeometry.h"
#include "GrassLoader.h"
#include "BatchPage.h"
#include "ImpostorPage.h"
#include "TreeLoader3D.h"
#include "space_world.h"
#include "space_login.h"
#include "space_avatarselect.h"
#include "common/common.h"
#include "pyscript/pythread_lock.h"

#include "../../kbengine_dll/kbengine_dll.h"

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
HINSTANCE g_hKBEngineDll = NULL;
#endif

namespace KBEngine{
	COMPONENT_TYPE g_componentType = CLIENT_TYPE;
	COMPONENT_ORDER g_componentGlobalOrder = 1;
	COMPONENT_ORDER g_componentGroupOrder = 1;
	COMPONENT_ID g_componentID = 1;
	int32 g_genuuid_sections = -1;
}

template<> OgreApplication* Ogre::Singleton<OgreApplication>::msSingleton = 0;

Space* g_space = NULL;
boost::mutex g_spaceMutex;
volatile bool g_hasEvent = false;

unsigned int   mVertexBufferSize = 0;
unsigned int   mIndexBufferSize = 0;
Ogre::Vector3 *mVertexBuffer = 0;
unsigned long *mIndexBuffer = 0;

//-------------------------------------------------------------------------------------
OgreApplication::OgreApplication(void):
events_()
{
	kbe_registerEventHandle(this);
}

//-------------------------------------------------------------------------------------
OgreApplication::~OgreApplication(void)
{
	kbe_deregisterEventHandle(this);
	mCameraMan = NULL;
	SAFE_RELEASE(g_space);
}

//-------------------------------------------------------------------------------------
void OgreApplication::go(void)
{
    if (!setup())
        return;

	while(!mShutDown)
	{
		mRoot->renderOneFrame();

		// ֪ͨϵͳ������Ϣ
		Ogre::WindowEventUtilities::messagePump();

		if(!g_hasEvent)
			Sleep(1);
	}

    // clean up
    destroyScene();
}

//-------------------------------------------------------------------------------------
bool OgreApplication::setup(void)
{
#ifdef _DEBUG
    mRoot = new Ogre::Root("plugins_d.cfg");
#else
    mRoot = new Ogre::Root();
#endif
    setupResources();

    if (!configure()) return false;

    // Set default mipmap level (NB some APIs ignore this)
    Ogre::TextureManager::getSingleton().setDefaultNumMipmaps(5);

    // Create any resource listeners (for loading screens)
    createResourceListener();
    // Load resources
    loadResources();

    createFrameListener();
	
	mWindow->setDeactivateOnFocusChange(false);
	changeSpace(new SpaceLogin(mRoot, mWindow, mInputManager, mTrayMgr));
    return true;
};

//-------------------------------------------------------------------------------------
void OgreApplication::changeSpace(Space* space)
{
	if(g_space)
		delete g_space;

	g_space = space;
	space->setup();
}

//-------------------------------------------------------------------------------------
void OgreApplication::setupResources(void)
{
    BaseApplication::setupResources();
}

//-------------------------------------------------------------------------------------
bool OgreApplication::frameRenderingQueued(const Ogre::FrameEvent& evt)
{
	if(g_space == NULL)
	{
		return BaseApplication::frameRenderingQueued(evt);
	}
	
	while(true)
	{
		g_spaceMutex.lock();

		if(events_.empty())
		{
			g_spaceMutex.unlock();
			break;
		}

		std::tr1::shared_ptr<const KBEngine::EventData> pEventData = events_.front();
		events_.pop();
		g_spaceMutex.unlock();

		KBEngine::EventID id = pEventData->id;
		
		if(id == CLIENT_EVENT_SERVER_CLOSED)
		{
			//OgreApplication::getSingleton().changeSpace(new SpaceAvatarSelect(mRoot, mWindow, mInputManager, mTrayMgr));
			//break;
		}

		// �����Ҫ�ڱ��̷߳��ʽű�������Ҫ��ס����
		if(id == CLIENT_EVENT_SCRIPT)
		{
			kbe_lock();
		}

		g_space->kbengine_onEvent(pEventData.get());

		if(id == CLIENT_EVENT_SCRIPT)
		{
			kbe_unlock();
		}

		g_hasEvent = false;
	}

	if(!g_space->frameRenderingQueued(evt))
		return true;

    return BaseApplication::frameRenderingQueued(evt);
}

//-------------------------------------------------------------------------------------
bool OgreApplication::keyPressed( const OIS::KeyEvent &arg )
{
    if (mTrayMgr->isDialogVisible()) return true;   // don't process any more keys if dialog is up

	if(g_space)
	{
		if(!g_space->keyPressed(arg))
			return true;
	}

    return BaseApplication::keyPressed( arg );
}

//-------------------------------------------------------------------------------------
bool OgreApplication::keyReleased( const OIS::KeyEvent &arg )
{
    if (mTrayMgr->isDialogVisible()) return true;   // don't process any more keys if dialog is up

	if(g_space)
	{
		if(!g_space->keyReleased(arg))
			return true;
	}

    return BaseApplication::keyReleased(arg);
}

//-------------------------------------------------------------------------------------
bool OgreApplication::mouseMoved( const OIS::MouseEvent &arg )
{
    mTrayMgr->injectMouseMove(arg);
    if (mTrayMgr->isDialogVisible()) return true;   // don't process any more keys if dialog is up

    if(mCameraMan)
       mCameraMan->injectMouseMove(arg);

	if(g_space)
	{
		if(!g_space->mouseMoved(arg))
			return true;
	}

    return true;
}

//-------------------------------------------------------------------------------------
bool OgreApplication::mousePressed( const OIS::MouseEvent &arg, OIS::MouseButtonID id )
{
    mTrayMgr->injectMouseDown(arg, id);
    if (mTrayMgr->isDialogVisible()) return true;   // don't process any more keys if dialog is up

    if(mCameraMan)
       mCameraMan->injectMouseDown(arg, id);

	if(g_space)
	{
		if(!g_space->mousePressed(arg, id))
			return true;
	}

    return true;
}

//-------------------------------------------------------------------------------------
bool OgreApplication::mouseReleased( const OIS::MouseEvent &arg, OIS::MouseButtonID id )
{
    mTrayMgr->injectMouseUp(arg, id);
    if (mTrayMgr->isDialogVisible()) return true;   // don't process any more keys if dialog is up

    if(mCameraMan)
       mCameraMan->injectMouseUp(arg, id);

	if(g_space)
	{
		if(!g_space->mouseReleased(arg, id))
			return true;
	}

    return true;
}

//-------------------------------------------------------------------------------------
void OgreApplication::buttonHit(OgreBites::Button* button)
{
	if(g_space)
	{
		g_space->buttonHit(button);
	}
}

//-------------------------------------------------------------------------------------
void OgreApplication::kbengine_onEvent(const KBEngine::EventData* lpEventData)
{
	KBEngine::EventData* peventdata = KBEngine::copyKBEngineEvent(lpEventData);

	if(peventdata)
	{
		boost::mutex::scoped_lock lock(g_spaceMutex);
		events_.push(std::tr1::shared_ptr<const KBEngine::EventData>(peventdata));
		g_hasEvent = true;
	}
}

//-------------------------------------------------------------------------------------
void OgreApplication::windowClosed(Ogre::RenderWindow* rw)
{
	BaseApplication::windowClosed(rw);
	mShutDown = true;
}

//-------------------------------------------------------------------------------------
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#define WIN32_LEAN_AND_MEAN
#include "windows.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
    INT WINAPI WinMain( HINSTANCE hInst, HINSTANCE, LPSTR strCmdLine, INT )
#else
    int main(int argc, char *argv[])
#endif
    {
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#ifdef _DEBUG
		std::string kbenginedll_name = "kbengine_d.dll";
#else
		std::string kbenginedll_name = "kbengine.dll";
#endif

		g_hKBEngineDll = LoadLibrary(kbenginedll_name.c_str());
		if (g_hKBEngineDll == NULL)
		{
			std::string kbenginedll_name_failed = "load " + kbenginedll_name + " is failed!";
			MessageBox( NULL, kbenginedll_name_failed.c_str(), "An exception has occured!", MB_OK | MB_ICONERROR | MB_TASKMODAL);
			return 0;
		}
#endif

		if(!kbe_init())
		{
			MessageBox( NULL, "kbengine_init() is failed!", "An exception has occured!", MB_OK | MB_ICONERROR | MB_TASKMODAL);
			return 0;
		}

        // Create application object
        OgreApplication* app = new OgreApplication();

        try {
            app->go();
        } catch( Ogre::Exception& e ) {
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
            MessageBox( NULL, e.getFullDescription().c_str(), "An exception has occured!", MB_OK | MB_ICONERROR | MB_TASKMODAL);
#else
            std::cerr << "An exception has occured: " <<
                e.getFullDescription().c_str() << std::endl;
#endif
        }
		
		delete app;
		kbe_destroy();

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
		FreeLibrary(g_hKBEngineDll);
#endif

        return 0;
    }

#ifdef __cplusplus
}
#endif
