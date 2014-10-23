#ifndef __KBENGINE_DLL__
#define __KBENGINE_DLL__
#pragma warning(disable : 4217)
#pragma warning(disable : 4049)
#include "stdio.h"
#include "stdlib.h"
#include "cstdkbe/cstdkbe.hpp"
#include "Python.h"

#ifdef KBE_DLL_API
#else
#define KBE_DLL_API  extern "C" _declspec(dllimport)
#endif

namespace KBEngine{
	class EventHandle;
}

inline char* wchar2char(const wchar_t* ts)
{
	int len = (wcslen(ts) + 1) * 4;
	char* ccattr =(char *)malloc(len);
	memset(ccattr, 0, len);
	wcstombs(ccattr, ts, len);
	return ccattr;
};

inline wchar_t* char2wchar(const char* cs)
{
	int len = (strlen(cs) + 1) * 4;
	wchar_t* ccattr =(wchar_t *)malloc(len);
	memset(ccattr, 0, len);
	mbstowcs(ccattr, cs, len);
	return ccattr;
};

/**
	��ʼ������������ģ��
*/
KBE_DLL_API bool kbe_init();
KBE_DLL_API bool kbe_destroy();

/**
	����һ��uint64λ��Ψһֵ(ע�⣺����ǰapp�в�����Ψһ��)
*/
KBE_DLL_API KBEngine::uint64 kbe_genUUID64(); 

/**
	����
*/
KBE_DLL_API void kbe_sleep(KBEngine::uint32 ms); 

/**
	��õ�ǰϵͳʱ�䣬 ����
*/
KBE_DLL_API KBEngine::uint32 kbe_getSystemTime();

/**
	��¼������
*/
KBE_DLL_API bool kbe_login(const char* accountName, const char* passwd, 
						   const char* ip = NULL, KBEngine::uint32 port = 0);

/**
	��������״̬
*/
KBE_DLL_API void kbe_update();

/**
	�������� ��ֹ���߳��·��ʽű��ȳ���
	��Ҫ���python
*/
KBE_DLL_API void kbe_lock();
KBE_DLL_API void kbe_unlock();

/**
	����������Ⱦ���ڴ�����
*/
KBE_DLL_API void kbe_inProcess(bool v);

/**
	��ȡ���һ��ʹ�õ��˺�
*/
KBE_DLL_API const char* kbe_getLastAccountName();

/**
	�õ�player�ĵ�ǰID
*/
KBE_DLL_API KBEngine::ENTITY_ID kbe_playerID();

/**
	�õ�player�ĵ�ǰdbid
*/
KBE_DLL_API KBEngine::DBID kbe_playerDBID();

/**
	����player�������볯��
*/
KBE_DLL_API void kbe_updateVolatile(KBEngine::ENTITY_ID eid, float x, float y, float z, float yaw, float pitch, float roll);

/**
	ע��һ���¼�handle����������������¼�
	Ȼ������¼���������Ӧ�ı���
*/
KBE_DLL_API bool kbe_registerEventHandle(KBEngine::EventHandle* pHandle);
KBE_DLL_API bool kbe_deregisterEventHandle(KBEngine::EventHandle* pHandle);

/**
	���ýű�entity�ķ��� 
*/
KBE_DLL_API void kbe_callEntityMethod(KBEngine::ENTITY_ID entityID, const char *method, 
										   const char* strargs); 


/**
	�����¼����ű�
	���Դ��������¼�
	��������ƶ��¼���
*/
KBE_DLL_API void kbe_fireEvent(const char *eventID, PyObject *args); 


/**
	����û��ű���Դ��Ŀ¼
*/
KBE_DLL_API const char* kbe_getPyUserResPath();

#endif // __KBENGINE_DLL__