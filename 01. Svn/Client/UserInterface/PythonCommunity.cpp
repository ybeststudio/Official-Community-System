#if defined(ENABLE_MESSENGER_RENEWAL)

#include "PythonCommunity.h"
#include "PythonMessenger.h"
#include "PythonNetworkStream.h"
#include "PythonCharacterManager.h"
#include "InstanceBase.h"
#include "../eterBase/Timer.h"

namespace
{
	static DWORD NameKeyToPid(const char* c_szName)
	{
		if (!c_szName || !*c_szName)
			return 0;

		DWORD hash = 2166136261u;
		for (const unsigned char* p = reinterpret_cast<const unsigned char*>(c_szName); *p; ++p)
		{
			hash ^= *p;
			hash *= 16777619u;
		}
		return hash ? hash : 1u;
	}

	static DWORD s_dwLastPartyInviteTime = 0;
	static DWORD s_dwLastConnectionStateTime = 0;
	static DWORD s_dwLastStatusMessageTime = 0;

	enum
	{
		PARTY_INVITE_LIMIT_MS = 2000,
		CONNECTION_STATE_LIMIT_MS = 2000,
		STATUS_MESSAGE_LIMIT_MS = 2000,
		MAX_STATUS_MESSAGE_LEN = 50,
	};
}

CPythonCommunity::CPythonCommunity()
	: m_poCommunityHandler(NULL)
	, m_poMessengerHandler(NULL)
	, m_poGuildHandler(NULL)
	, m_poConfigHandler(NULL)
	, m_dwConfigFlag(532676608u)
	, m_bMainConnectionState(0)
{
}

CPythonCommunity::~CPythonCommunity()
{
	Destroy();
}

void CPythonCommunity::Destroy()
{
	ClearCommunityHandler();
	ClearMessengerHandler();
	ClearGuildHandler();
	ClearConfigHandler();
}

void CPythonCommunity::SetCommunityHandler(PyObject* poHandler)
{
	m_poCommunityHandler = poHandler;
}

void CPythonCommunity::SetMessengerHandler(PyObject* poHandler)
{
	m_poMessengerHandler = poHandler;
}

void CPythonCommunity::SetGuildHandler(PyObject* poHandler)
{
	m_poGuildHandler = poHandler;
}

void CPythonCommunity::SetConfigHandler(PyObject* poHandler)
{
	m_poConfigHandler = poHandler;
}

void CPythonCommunity::ClearCommunityHandler()
{
	m_poCommunityHandler = NULL;
}

void CPythonCommunity::ClearMessengerHandler()
{
	m_poMessengerHandler = NULL;
}

void CPythonCommunity::ClearGuildHandler()
{
	m_poGuildHandler = NULL;
}

void CPythonCommunity::ClearConfigHandler()
{
	m_poConfigHandler = NULL;
}

void CPythonCommunity::OnFriendOnline(const char* c_szName
#if defined(ENABLE_MESSENGER_DETAILS)
	, DWORD dwLastPlayTime
#if defined(ENABLE_MULTI_LANGUAGE_SYSTEM)
	, const char* c_szCountry
#endif
#endif
)
{
	if (!m_poMessengerHandler || !c_szName)
		return;

	const DWORD pid = NameKeyToPid(c_szName);

#if defined(ENABLE_MESSENGER_DETAILS)
#if defined(ENABLE_MULTI_LANGUAGE_SYSTEM)
	PyCallClassMemberFunc(m_poMessengerHandler, "AddOnlineFriendFromBridge", Py_BuildValue("(isIs)", pid, c_szName, dwLastPlayTime, c_szCountry ? c_szCountry : ""));
#else
	PyCallClassMemberFunc(m_poMessengerHandler, "AddOnlineFriendFromBridge", Py_BuildValue("(isi)", pid, c_szName, dwLastPlayTime));
#endif
#else
	PyCallClassMemberFunc(m_poMessengerHandler, "AddOnlineFriendFromBridge", Py_BuildValue("(is)", pid, c_szName));
#endif
}

void CPythonCommunity::OnFriendOffline(const char* c_szName
#if defined(ENABLE_MESSENGER_DETAILS)
	, DWORD dwLastPlayTime
#if defined(ENABLE_MULTI_LANGUAGE_SYSTEM)
	, const char* c_szCountry
#endif
#endif
)
{
	if (!m_poMessengerHandler || !c_szName)
		return;

	const DWORD pid = NameKeyToPid(c_szName);

#if defined(ENABLE_MESSENGER_DETAILS)
#if defined(ENABLE_MULTI_LANGUAGE_SYSTEM)
	PyCallClassMemberFunc(m_poMessengerHandler, "LoginFriendFromBridge", Py_BuildValue("(isIs)", pid, c_szName, dwLastPlayTime, c_szCountry ? c_szCountry : ""));
#else
	PyCallClassMemberFunc(m_poMessengerHandler, "LoginFriendFromBridge", Py_BuildValue("(isi)", pid, c_szName, dwLastPlayTime));
#endif
#else
	PyCallClassMemberFunc(m_poMessengerHandler, "LoginFriendFromBridge", Py_BuildValue("(is)", pid, c_szName));
#endif
}

void CPythonCommunity::OnFriendRemove(const char* c_szName)
{
	if (!m_poMessengerHandler || !c_szName)
		return;

	PyCallClassMemberFunc(m_poMessengerHandler, "DeleteFriendMember", Py_BuildValue("(i)", NameKeyToPid(c_szName)));
}

#if defined(ENABLE_MESSENGER_RENEWAL)
void CPythonCommunity::OnFriendRenewalDetails(const char* c_szName, BYTE bLevel, BYTE bIsConqueror, BYTE bChannel, long lMapIndex)
{
	if (!m_poMessengerHandler || !c_szName)
		return;

	const DWORD pid = NameKeyToPid(c_szName);

	if (bLevel > 0)
		PyCallClassMemberFunc(m_poMessengerHandler, "SetFriendLevelInfo", Py_BuildValue("(iii)", pid, bIsConqueror, bLevel));

	if (bChannel > 0 || lMapIndex != 0)
		PyCallClassMemberFunc(m_poMessengerHandler, "SetFriendLocationInfo", Py_BuildValue("(iii)", pid, bChannel, lMapIndex));
}

void CPythonCommunity::OnFriendConnectionState(const char* c_szName, BYTE bConnectionState)
{
	if (!m_poMessengerHandler || !c_szName)
		return;

	const DWORD pid = NameKeyToPid(c_szName);
	PyCallClassMemberFunc(m_poMessengerHandler, "ChangeFriendConnectionState",
		Py_BuildValue("(ii)", pid, static_cast<int>(bConnectionState)));
}

void CPythonCommunity::OnFriendStatusMessage(const char* c_szName, const char* c_szMessage)
{
	if (!m_poMessengerHandler || !c_szName)
		return;

	const DWORD pid = NameKeyToPid(c_szName);
	PyCallClassMemberFunc(m_poMessengerHandler, "SetFriendStatusMessage",
		Py_BuildValue("(is)", pid, c_szMessage ? c_szMessage : ""));
}

void CPythonCommunity::NotifyMyStatusMessage(const char* c_szMessage)
{
	if (!m_poCommunityHandler)
		return;

	PyCallClassMemberFunc(m_poCommunityHandler, "LoadMyStatusMessage",
		Py_BuildValue("(s)", c_szMessage ? c_szMessage : ""));
}

#if defined(ENABLE_COMMUNITY_GUILD_RENEWAL)
void CPythonCommunity::OnGuildMemberLocation(DWORD dwPID, BYTE bChannel, long lMapIndex)
{
	if (!m_poMessengerHandler)
		return;

	if (bChannel == 0 && lMapIndex == 0)
		return;

	PyCallClassMemberFunc(m_poMessengerHandler, "SetGuildMemberLocationInfo",
		Py_BuildValue("(iii)", dwPID, static_cast<int>(bChannel), lMapIndex));
}
#endif
#endif

#if defined(ENABLE_MESSENGER_BLOCK)
void CPythonCommunity::OnBlockOnline(const char* c_szName)
{
	if (!m_poMessengerHandler || !c_szName)
		return;

	PyCallClassMemberFunc(m_poMessengerHandler, "AddBlockFromBridge", Py_BuildValue("(is)", NameKeyToPid(c_szName), c_szName));
}

void CPythonCommunity::OnBlockOffline(const char* c_szName)
{
	if (!m_poMessengerHandler || !c_szName)
		return;

	PyCallClassMemberFunc(m_poMessengerHandler, "AddBlockFromBridge", Py_BuildValue("(is)", NameKeyToPid(c_szName), c_szName));
}

void CPythonCommunity::OnBlockRemove(const char* c_szName)
{
	if (!m_poMessengerHandler || !c_szName)
		return;

	PyCallClassMemberFunc(m_poMessengerHandler, "DeleteBlockMember", Py_BuildValue("(i)", NameKeyToPid(c_szName)));
}
#endif

PyObject* communitySetCommunityHandler(PyObject* poSelf, PyObject* poArgs)
{
	PyObject* poHandler = NULL;
	if (!PyTuple_GetObject(poArgs, 0, &poHandler))
		return Py_BuildException();

	CPythonCommunity::Instance().SetCommunityHandler(poHandler);
	return Py_BuildNone();
}

PyObject* communitySetMessengerHandler(PyObject* poSelf, PyObject* poArgs)
{
	PyObject* poHandler = NULL;
	if (!PyTuple_GetObject(poArgs, 0, &poHandler))
		return Py_BuildException();

	CPythonCommunity::Instance().SetMessengerHandler(poHandler);
	return Py_BuildNone();
}

PyObject* communitySetGuildHandler(PyObject* poSelf, PyObject* poArgs)
{
	PyObject* poHandler = NULL;
	if (!PyTuple_GetObject(poArgs, 0, &poHandler))
		return Py_BuildException();

	CPythonCommunity::Instance().SetGuildHandler(poHandler);
	return Py_BuildNone();
}

PyObject* communitySetConfigHandler(PyObject* poSelf, PyObject* poArgs)
{
	PyObject* poHandler = NULL;
	if (!PyTuple_GetObject(poArgs, 0, &poHandler))
		return Py_BuildException();

	CPythonCommunity::Instance().SetConfigHandler(poHandler);
	return Py_BuildNone();
}

PyObject* communityClearCommunityHandler(PyObject* poSelf, PyObject* poArgs)
{
	CPythonCommunity::Instance().ClearCommunityHandler();
	return Py_BuildNone();
}

PyObject* communityClearMessengerHandler(PyObject* poSelf, PyObject* poArgs)
{
	CPythonCommunity::Instance().ClearMessengerHandler();
	return Py_BuildNone();
}

PyObject* communityClearGuildHandler(PyObject* poSelf, PyObject* poArgs)
{
	CPythonCommunity::Instance().ClearGuildHandler();
	return Py_BuildNone();
}

PyObject* communityClearConfigHandler(PyObject* poSelf, PyObject* poArgs)
{
	CPythonCommunity::Instance().ClearConfigHandler();
	return Py_BuildNone();
}

PyObject* communityIsFriendByName(PyObject* poSelf, PyObject* poArgs)
{
	char* szName = NULL;
	if (!PyTuple_GetString(poArgs, 0, &szName))
		return Py_BuildException();

	return Py_BuildValue("i", CPythonMessenger::Instance().IsFriendByName(szName));
}

PyObject* communityGetFriendCount(PyObject* poSelf, PyObject* poArgs)
{
	const CPythonMessenger& messenger = CPythonMessenger::Instance();
	return Py_BuildValue("i", static_cast<int>(messenger.m_FriendNameMap.size()));
}

PyObject* communityGetFriendsNameByTuple(PyObject* poSelf, PyObject* poArgs)
{
	const CPythonMessenger& messenger = CPythonMessenger::Instance();
	PyObject* poTuple = PyTuple_New(messenger.m_FriendNameMap.size());
	int i = 0;
	for (const std::string& sFriendName : messenger.m_FriendNameMap)
		PyTuple_SetItem(poTuple, i++, PyString_FromString(sFriendName.c_str()));
	return poTuple;
}

#if defined(ENABLE_MESSENGER_BLOCK)
PyObject* communityGetBlocksNameByTuple(PyObject* poSelf, PyObject* poArgs)
{
	const CPythonMessenger& messenger = CPythonMessenger::Instance();
	const CPythonMessenger::TBlockNameMap& blockMap = messenger.GetBlockNameMap();
	PyObject* poTuple = PyTuple_New(blockMap.size());
	int i = 0;
	for (const std::string& sBlockName : blockMap)
		PyTuple_SetItem(poTuple, i++, PyString_FromString(sBlockName.c_str()));
	return poTuple;
}
#endif

PyObject* communityRequestMessengerInfo(PyObject* poSelf, PyObject* poArgs)
{
	CPythonNetworkStream::Instance().RequestMessengerRefresh();
	return Py_BuildNone();
}

PyObject* communityEnterGame(PyObject* poSelf, PyObject* poArgs)
{
	CPythonNetworkStream::Instance().RequestMessengerRefresh();
	return Py_BuildNone();
}

PyObject* communitySendDeleteMember(PyObject* poSelf, PyObject* poArgs)
{
	char* szName = NULL;
	if (!PyTuple_GetString(poArgs, 0, &szName))
		return Py_BuildException();

	if (szName && *szName)
		CPythonMessenger::Instance().RemoveFriend(szName);

	CPythonNetworkStream& rkNet = CPythonNetworkStream::Instance();
	rkNet.SendMessengerRemovePacket(szName, szName);
	return Py_BuildNone();
}

PyObject* communitySendAddBlock(PyObject* poSelf, PyObject* poArgs)
{
	char* szName = NULL;
	if (!PyTuple_GetString(poArgs, 0, &szName))
		return Py_BuildException();

	CPythonNetworkStream& rkNet = CPythonNetworkStream::Instance();
	rkNet.SendMessengerBlockAddByNamePacket(szName);
	return Py_BuildNone();
}

PyObject* communitySendRequestFriend(PyObject* poSelf, PyObject* poArgs)
{
	char* szName = NULL;
	if (!PyTuple_GetString(poArgs, 0, &szName))
		return Py_BuildException();

	CPythonNetworkStream& rkNet = CPythonNetworkStream::Instance();
	rkNet.SendMessengerAddByNamePacket(szName);
	return Py_BuildNone();
}

PyObject* communityCanPartyInviteTime(PyObject* poSelf, PyObject* poArgs)
{
	const DWORD dwNow = ELTimer_GetMSec();
	return Py_BuildValue("i", (dwNow - s_dwLastPartyInviteTime) >= PARTY_INVITE_LIMIT_MS ? 1 : 0);
}

PyObject* communitySendPartyInvite(PyObject* poSelf, PyObject* poArgs)
{
	char* szName = NULL;
	if (!PyTuple_GetString(poArgs, 0, &szName) || !szName || !*szName)
		return Py_BuildNone();

	const DWORD dwNow = ELTimer_GetMSec();
	if (dwNow - s_dwLastPartyInviteTime < PARTY_INVITE_LIMIT_MS)
		return Py_BuildNone();

	CInstanceBase* pInstance = CPythonCharacterManager::Instance().GetInstancePtrByName(szName);
	if (!pInstance || !pInstance->IsPC())
		return Py_BuildNone();

	if (CPythonNetworkStream::Instance().SendPartyInvitePacket(pInstance->GetVirtualID()))
		s_dwLastPartyInviteTime = dwNow;

	return Py_BuildNone();
}

PyObject* communityCanChangeMyConnectionStateTime(PyObject* poSelf, PyObject* poArgs)
{
	const DWORD dwNow = ELTimer_GetMSec();
	return Py_BuildValue("i", (dwNow - s_dwLastConnectionStateTime) >= CONNECTION_STATE_LIMIT_MS ? 1 : 0);
}

PyObject* communitySendChangeConnectionState(PyObject* poSelf, PyObject* poArgs)
{
	int iState = 0;
	if (!PyTuple_GetInteger(poArgs, 0, &iState))
		return Py_BuildException();

	if (iState < 0 || iState > MESSENGER_CONNECTION_STATE_DISCONNECT)
		return Py_BuildNone();

	const DWORD dwNow = ELTimer_GetMSec();
	if (dwNow - s_dwLastConnectionStateTime < CONNECTION_STATE_LIMIT_MS)
		return Py_BuildNone();

	if (!CPythonNetworkStream::Instance().SendMessengerSetConnectionStatePacket(static_cast<BYTE>(iState)))
		return Py_BuildNone();

	s_dwLastConnectionStateTime = dwNow;
	CPythonCommunity::Instance().SetMainConnectionState(static_cast<BYTE>(iState));
	return Py_BuildNone();
}

PyObject* communityForceSendChangeConnectionState(PyObject* poSelf, PyObject* poArgs)
{
	int iState = 0;
	if (!PyTuple_GetInteger(poArgs, 0, &iState))
		return Py_BuildException();

	if (iState < 0 || iState > MESSENGER_CONNECTION_STATE_DISCONNECT)
		return Py_BuildNone();

	if (!CPythonNetworkStream::Instance().SendMessengerSetConnectionStatePacket(static_cast<BYTE>(iState)))
		return Py_BuildNone();

	CPythonCommunity::Instance().SetMainConnectionState(static_cast<BYTE>(iState));
	return Py_BuildNone();
}

PyObject* communityCanChangeMyStatusMessageTime(PyObject* poSelf, PyObject* poArgs)
{
	const DWORD dwNow = ELTimer_GetMSec();
	return Py_BuildValue("i", (dwNow - s_dwLastStatusMessageTime) >= STATUS_MESSAGE_LIMIT_MS ? 1 : 0);
}

PyObject* communitySendRegisterMyStatusMessage(PyObject* poSelf, PyObject* poArgs)
{
	char* szMessage = NULL;
	if (!PyTuple_GetString(poArgs, 0, &szMessage) || !szMessage)
		return Py_BuildNone();

	size_t len = strlen(szMessage);
	if (len == 0 || len > MAX_STATUS_MESSAGE_LEN)
		return Py_BuildNone();

	const DWORD dwNow = ELTimer_GetMSec();
	if (dwNow - s_dwLastStatusMessageTime < STATUS_MESSAGE_LIMIT_MS)
		return Py_BuildNone();

	if (!CPythonNetworkStream::Instance().SendMessengerSetStatusMessagePacket(szMessage))
		return Py_BuildNone();

	s_dwLastStatusMessageTime = dwNow;
	return Py_BuildNone();
}

PyObject* communitySendDeleteMyStatusMessage(PyObject* poSelf, PyObject* poArgs)
{
	const DWORD dwNow = ELTimer_GetMSec();
	if (dwNow - s_dwLastStatusMessageTime < STATUS_MESSAGE_LIMIT_MS)
		return Py_BuildNone();

	if (!CPythonNetworkStream::Instance().SendMessengerDeleteStatusMessagePacket())
		return Py_BuildNone();

	s_dwLastStatusMessageTime = dwNow;
	return Py_BuildNone();
}

PyObject* communityGetMainCharacterConnectionState(PyObject* poSelf, PyObject* poArgs)
{
	CPythonCommunity& rkCommunity = CPythonCommunity::Instance();
	return Py_BuildValue("i", static_cast<int>(rkCommunity.GetMainConnectionState()));
}

PyObject* communitySetMainCharacterConnectionState(PyObject* poSelf, PyObject* poArgs)
{
	int iState = 0;
	if (!PyTuple_GetInteger(poArgs, 0, &iState))
		return Py_BuildException();

	CPythonCommunity::Instance().SetMainConnectionState(static_cast<BYTE>(iState));
	return Py_BuildNone();
}

PyObject* communityGetLastSavedConfigFlag(PyObject* poSelf, PyObject* poArgs)
{
	return Py_BuildValue("i", 532676608);
}

PyObject* communitySetMyConfigFlag(PyObject* poSelf, PyObject* poArgs)
{
	int iFlag = 0;
	if (!PyTuple_GetInteger(poArgs, 0, &iFlag))
		return Py_BuildException();
	return Py_BuildNone();
}

PyObject* communityIsSetConfigFlag(PyObject* poSelf, PyObject* poArgs)
{
	return Py_BuildValue("i", 0);
}

PyObject* communitySendSaveConfig(PyObject* poSelf, PyObject* poArgs)
{
	return Py_BuildNone();
}

PyObject* communitySendInitConfig(PyObject* poSelf, PyObject* poArgs)
{
	return Py_BuildNone();
}

void initCommunity()
{
	static PyMethodDef s_methods[] =
	{
		{ "SetCommunityHandler", communitySetCommunityHandler, METH_VARARGS },
		{ "SetMessengerHandler", communitySetMessengerHandler, METH_VARARGS },
		{ "SetGuildHandler", communitySetGuildHandler, METH_VARARGS },
		{ "SetConfigHandler", communitySetConfigHandler, METH_VARARGS },
		{ "ClearCommunityHandler", communityClearCommunityHandler, METH_VARARGS },
		{ "ClearMessengerHandler", communityClearMessengerHandler, METH_VARARGS },
		{ "ClearGuildHandler", communityClearGuildHandler, METH_VARARGS },
		{ "ClearConfigHandler", communityClearConfigHandler, METH_VARARGS },
		{ "IsFriendByName", communityIsFriendByName, METH_VARARGS },
		{ "GetFriendCount", communityGetFriendCount, METH_VARARGS },
		{ "GetFriendsNameByTuple", communityGetFriendsNameByTuple, METH_VARARGS },
#if defined(ENABLE_MESSENGER_BLOCK)
		{ "GetBlocksNameByTuple", communityGetBlocksNameByTuple, METH_VARARGS },
#endif
		{ "RequestMessengerInfo", communityRequestMessengerInfo, METH_VARARGS },
		{ "EnterGame", communityEnterGame, METH_VARARGS },
		{ "SendDeleteMember", communitySendDeleteMember, METH_VARARGS },
		{ "SendAddBlock", communitySendAddBlock, METH_VARARGS },
		{ "SendRequestFriend", communitySendRequestFriend, METH_VARARGS },
		{ "CanPartyInviteTime", communityCanPartyInviteTime, METH_VARARGS },
		{ "SendPartyInvite", communitySendPartyInvite, METH_VARARGS },
		{ "CanChangeMyConnectionStateTime", communityCanChangeMyConnectionStateTime, METH_VARARGS },
		{ "SendChangeConnectionState", communitySendChangeConnectionState, METH_VARARGS },
		{ "ForceSendChangeConnectionState", communityForceSendChangeConnectionState, METH_VARARGS },
		{ "CanChangeMyStatusMessageTime", communityCanChangeMyStatusMessageTime, METH_VARARGS },
		{ "SendRegisterMyStatusMessage", communitySendRegisterMyStatusMessage, METH_VARARGS },
		{ "SendDeleteMyStatusMessage", communitySendDeleteMyStatusMessage, METH_VARARGS },
		{ "GetMainCharacterConnectionState", communityGetMainCharacterConnectionState, METH_VARARGS },
		{ "SetMainCharacterConnectionState", communitySetMainCharacterConnectionState, METH_VARARGS },
		{ "GetLastSavedConfigFlag", communityGetLastSavedConfigFlag, METH_VARARGS },
		{ "SetMyConfigFlag", communitySetMyConfigFlag, METH_VARARGS },
		{ "IsSetConfigFlag", communityIsSetConfigFlag, METH_VARARGS },
		{ "SendSaveConfig", communitySendSaveConfig, METH_VARARGS },
		{ "SendInitConfig", communitySendInitConfig, METH_VARARGS },
		{ NULL, NULL, NULL },
	};

	PyObject* poModule = Py_InitModule("community", s_methods);

	PyModule_AddIntConstant(poModule, "CONNECT", 0);
	PyModule_AddIntConstant(poModule, "LEFT_SEAT", 1);
	PyModule_AddIntConstant(poModule, "SHOP_OPEN", 3);
	PyModule_AddIntConstant(poModule, "DISCONNECT", 4);
	PyModule_AddIntConstant(poModule, "CONNECT_STATE_NONE", 5);
	PyModule_AddIntConstant(poModule, "AUTO_HUNT", 2);

	PyModule_AddIntConstant(poModule, "MEMBER_TYPE_FAMILY", 0);
	PyModule_AddIntConstant(poModule, "MEMBER_TYPE_FRIEND", 1);
	PyModule_AddIntConstant(poModule, "MEMBER_TYPE_INACTIVE_FRIEND", 2);
	PyModule_AddIntConstant(poModule, "MEMBER_TYPE_BLOCK", 3);
	PyModule_AddIntConstant(poModule, "MEMBER_TYPE_REQUEST", 4);
	PyModule_AddIntConstant(poModule, "MEMBER_TYPE_GUILD", 5);

	PyModule_AddIntConstant(poModule, "MAX_FRIEND_COUNT", 50);
	PyModule_AddIntConstant(poModule, "MAX_BLOCK_COUNT", 50);
	PyModule_AddIntConstant(poModule, "MAX_STATUS_MESSAGE_LENGTH", 50);
	PyModule_AddIntConstant(poModule, "CONFIG_DEFAULT", 532676608);
	PyModule_AddIntConstant(poModule, "PARTY_INVITE_LIMIT_TIME_TO_SECOND", 2);

	PyModule_AddIntConstant(poModule, "CHAT_WITH_NAME_ADD_FRIEND", 0);
	PyModule_AddIntConstant(poModule, "CHAT_WITH_NAME_ADD_BLOCK", 1);
	PyModule_AddIntConstant(poModule, "CHAT_WITH_NAME_ADD_REQUEST", 2);
}

#endif
