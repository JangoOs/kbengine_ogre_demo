
#ifndef __OgreApplication_h_
#define __OgreApplication_h_

#include "BaseApplication.h"
#include <Terrain/OgreTerrain.h>
#include <Terrain/OgreTerrainGroup.h>
#include <OgreImage.h>
#include "client_lib/event.h"
#include "json/json.h"

class DotSceneLoader;
class Space;

namespace Forests
{
    class PagedGeometry;
    class GrassLoader;
    class GrassLayer;
}

class OgreApplication : public BaseApplication, public Ogre::Singleton<OgreApplication>, public KBEngine::EventHandle
{
public:
    OgreApplication(void);
    virtual ~OgreApplication(void);

	virtual void go(void);

	void setCurrCameraMan(OgreBites::SdkCameraMan* pCameraMan){ mCameraMan = pCameraMan; }

	virtual void buttonHit(OgreBites::Button* button);

	void kbengine_onEvent(const KBEngine::EventData* lpEventData);

	void changeSpace(Space* space);

	virtual void windowClosed(Ogre::RenderWindow* rw);

protected:
	virtual bool setup();
    virtual void setupResources();
	virtual void createScene(void){}
    virtual bool keyPressed(const OIS::KeyEvent &arg);
	virtual bool keyReleased(const OIS::KeyEvent &arg);
    virtual bool mouseMoved( const OIS::MouseEvent &arg );
    virtual bool mousePressed( const OIS::MouseEvent &arg, OIS::MouseButtonID id );
    virtual bool mouseReleased( const OIS::MouseEvent &arg, OIS::MouseButtonID id );
    virtual bool frameRenderingQueued(const Ogre::FrameEvent& evt);
private:
	std::queue< std::tr1::shared_ptr<const KBEngine::EventData> > events_;
};

#endif // #ifndef __OgreApplication_h_
