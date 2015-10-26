#include "EntitySimple.h"
#include "space_world.h"
#include "DotSceneLoader.h"
#include <ctime>
#include <Terrain/include/OgreTerrain.h>
#include <Terrain/include/OgreTerrainGroup.h>

#include "../../kbengine_dll/kbengine_dll.h"

Ogre::String animNames[] =
{"Attack1", "Attack2", "Attack3", "Attack4", "Attack5", "Battleidle1", "Battleidle2",
	"Idle1", "Idle2", "Block", "Die1", "Die2", "Run", "Walk", "Jump1", "Jump2"};

//-------------------------------------------------------------------------------------
EntitySimple::EntitySimple(SpaceWorld* pSpace, KBEngine::ENTITY_ID eid):
KBEntity(pSpace, eid),
mLastAnims(NULL),
mLastAnimName(),
blocktime_(0)
{
	//setupBody(cam->getSceneManager());
	//setupCamera(cam);
	//setupAnimations();
}

//-------------------------------------------------------------------------------------
EntitySimple::~EntitySimple()
{
}

//-------------------------------------------------------------------------------------
void EntitySimple::addTime(Real deltaTime)
{
	if(mState == 1)
	{
		if(mLastAnimName != "Die")
		{
			playAnimation("Die");
		}

		return;
	}

	if(blocktime_ <= 0.f)
	{
		Ogre::Vector3 currpos = getLastPosition();
		if(kbe_playerID() != mID)
		{
			Ogre::Vector3 movement = destPos_ - currpos;
			float speed = mMoveSpeed * deltaTime;

			movement.y = 0.f;

			Real mlen = movement.length();

			if(mlen < speed || (mLastAnimName != "Run" && mlen <= 1.0f))
			{
				if (mLastAnimName == "Run")
				{
					float y = currpos.y;
					currpos = destPos_;
					currpos.y = y;

					playAnimation("Idle");
				}
			}
			else
			{
				movement.normalise();

				// �ƶ�λ��
				movement *= speed;
				currpos += movement;
				
				playAnimation("Run");
			}
		}

		blocktime_ = 0.f;
	
		setPosition(currpos.x, currpos.y, currpos.z);
		KBEntity::addTime(deltaTime);
	}
	else
	{
		blocktime_ -= deltaTime;
		playAnimation("Block");
	}

	if(mLastAnims)
		mLastAnims->addTime(deltaTime);
}

//-------------------------------------------------------------------------------------
void EntitySimple::setupBody(SceneManager* sceneMgr)
{
	if(modelID_ == 1002)
		modelName_ = "dwarf.mesh";
	else if(modelID_ == 1003)
		modelName_ = "Ogre.mesh";
	//else if(modelID_ == 90000001) // Ŀǰ���Ʋ����ɳ�������Sinbad.mesh����ogrehead����
	//	modelName_ = "Sinbad.mesh";
	else
		modelName_ = "ogrehead.mesh";

	KBEntity::setupBody(sceneMgr);
}

//-------------------------------------------------------------------------------------
void EntitySimple::setupAnimations()
{
	if(modelName_ == "ogrehead.mesh")
		return;

	Ogre::AnimationStateSet* aniStetes = mBodyEnt->getAllAnimationStates();
	if(aniStetes != NULL)
	{
		// populate our animation list
		for (int i = 0; i < SIMPLE_NUM_ANIMS; i++)
		{
			if(aniStetes->hasAnimationState(animNames[i]))
			{
				mAnims[i] = mBodyEnt->getAnimationState(animNames[i]);
				mAnims[i]->setLoop(true);
			}
			else
			{
				char c = animNames[i].c_str()[animNames[i].size() - 1];

				if(c > '1' && c <= '9')
				{
					Ogre::String name = animNames[i];

					name.erase(name.size() - 1, 1);
					name += "1";

					mAnims[i] = mBodyEnt->getAnimationState(name);
					mAnims[i]->setLoop(true);
				}
				else
					mAnims[i] = NULL;
			}
		}
	}
	else
	{
		for (int i = 0; i < SIMPLE_NUM_ANIMS; i++)
		{
			mAnims[i] = NULL;
		}
	}

	if(mState != 1)
		playAnimation("Idle");
	else
		playAnimation("Die");
}

//-------------------------------------------------------------------------------------
void EntitySimple::playAnimation(Ogre::String name)
{
	if(modelName_ == "ogrehead.mesh")
		return;

	if(name != "Die" && mState == 1)
		name = "Die";

	AnimationState* astate = NULL;
	bool loopplay = true;

	if(name == "Idle")
	{
		srand((unsigned)time(NULL));

		if(mState == 3)
			astate = mAnims[ANIM_BATTLEIDLE1 + (rand() % 1)];
		else
			astate = mAnims[ANIM_IDLE_1 + (rand() % 1)];

		assert(mState != 1);
	}
	else if(name == "Die")
	{
		if(mLastAnimName == name)
			return;

		srand((unsigned)time(NULL));
		astate = mAnims[ANIM_DIE1 + (rand() % 1)];
		loopplay = false;
	}
	else if(name == "Attack")
	{
		srand((unsigned)time(NULL));
		int attackID = ANIM_ATTACK1 + (rand() % 4);
		astate = mAnims[attackID];
	}
	else
	{
		if(mLastAnimName == name)
			return;

		for (int i = 0; i < SIMPLE_NUM_ANIMS; i++)
		{
			if(name == animNames[i])
			{
				astate = mAnims[i];
				break;
			}
		}
	}
	
	if(astate == NULL)
		return;

	if(mLastAnims)
		mLastAnims->setEnabled(false);

	mLastAnimName = name;
	astate->setLoop(loopplay);
	astate->setEnabled(true);
	mLastAnims = astate;
}

//-------------------------------------------------------------------------------------
void EntitySimple::attack(KBEntity* receiver, uint32 skillID, uint32 damageType, uint32 damage)
{
	playAnimation("Attack");
}

//-------------------------------------------------------------------------------------
void EntitySimple::recvDamage(KBEntity* attacker, uint32 skillID, uint32 damageType, uint32 damage)
{
	if(damage < 3)
		blocktime_ = 0.5f;

	KBEntity::recvDamage(attacker, skillID, damageType, damage);
}

//-------------------------------------------------------------------------------------