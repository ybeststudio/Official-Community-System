/**
* Filename: messenger_manager.h
* Author: Owsap
**/

#ifndef __INC_MESSENGER_MANAGER_H__
#define __INC_MESSENGER_MANAGER_H__

#include "db.h"
#include "buffer_manager.h"

#if defined(__MESSENGER_RENEWAL__)
struct packet_messenger_list;
#endif

class CMessengerManager : public singleton<CMessengerManager>
{
public:
	CMessengerManager() {};
	virtual ~CMessengerManager() {};

public:
	void Initialize();
	void Destroy();

	void P2PLogin(const std::string& c_strPlayer);
	void P2PLogout(const std::string& c_strPlayer);

	void Login(const std::string& c_strPlayer);
	void Logout(const std::string& c_strPlayer);

	void RequestToAdd(const LPCHARACTER c_lpChar, const LPCHARACTER c_lpCharTarget);
	void AuthToAdd(const std::string& c_strAccount, const std::string& c_strCompanion, bool bDeny);

	void __AddToList(const std::string& c_strAccount, const std::string& c_strCompanion); // ???? m_Relation, m_InverseRelation ??????? ????
	void AddToList(const std::string& c_strAccount, const std::string& c_strCompanion);

	void __RemoveFromList(const std::string& c_strAccount, const std::string& c_strCompanion); // ???? m_Relation, m_InverseRelation ??????? ????
	void RemoveFromList(const std::string& c_strAccount, const std::string& c_strCompanion);
	void RemoveAllList(const std::string& c_strAccount);

	bool IsInList(const std::string& c_strAccount, const std::string& c_strCompanion);
	bool IsFriend(const char* c_strAccount, const char* c_szName);

#if defined(__MESSENGER_RENEWAL__)
	void SetConnectionState(LPCHARACTER c_lpChar, BYTE bConnectionState);
	void SetStatusMessage(LPCHARACTER c_lpChar, const char* c_szMessage);
	void DeleteStatusMessage(LPCHARACTER c_lpChar);
#endif

#if defined(__MESSENGER_BLOCK_SYSTEM__)
	void __AddToBlockList(const std::string& c_strAccount, const std::string& c_strCompanion);
	void AddToBlockList(const std::string& c_strAccount, const std::string& c_strCompanion);

	void __RemoveFromBlockList(const std::string& c_strAccount, const std::string& c_strCompanion);
	void RemoveFromBlockList(const std::string& c_strAccount, const std::string& c_strCompanion);
	void RemoveAllBlockList(const std::string& c_strAccount);

	bool IsBlocked(const char* c_strAccount, const char* c_szName);
#endif

private:
	void __SendList(const std::string& c_strAccount);
	void __SendLogin(const std::string& c_strAccount, const std::string& c_strCompanion);
	void __SendLogout(const std::string& c_strAccount, const std::string& c_strCompanion);
#if defined(__MESSENGER_RENEWAL__)
	void __FillRenewalDetails(packet_messenger_list& rListPacket, const char* c_szCompanionName);
	void __ApplyCompanionConnectionState(packet_messenger_list& rListPacket, const char* c_szCompanionName, bool bCompanionOnlineInSet);
	void __SendConnectionState(const std::string& c_strAccount, const std::string& c_strCompanion, BYTE bConnectionState);
	void __BroadcastConnectionState(const std::string& c_strCompanion, BYTE bConnectionState);
	void __FillStatusMessage(packet_messenger_list& rListPacket, const char* c_szCompanionName);
	void __SendMyStatusMessage(LPCHARACTER c_lpChar);
	void __SendStatusMessage(const std::string& c_strAccount, const char* c_szCompanionName, const char* c_szMessage);
	void __BroadcastStatusMessage(const std::string& c_strCompanion, const char* c_szMessage);
	void __LoadMyStatusMessage(SQLMsg* pMsg);
	void __SaveStatusMessageToDB(const char* c_szName, const char* c_szMessage);
	const char* __GetStatusMessage(const char* c_szName);
#endif

	enum EListRows
	{
#if defined(__MESSENGER_DETAILS__)
		LIST_ROW_ACCOUNT,
		LIST_ROW_ACCOUNT_LAST_PLAY,
#if defined(__MULTI_LANGUAGE_SYSTEM__)
		LIST_ROW_ACCOUNT_LANGUAGE,
#endif
		LIST_ROW_COMPANION,
		LIST_ROW_COMPANION_LAST_PLAY,
#if defined(__MULTI_LANGUAGE_SYSTEM__)
		LIST_ROW_COMPANION_LANGUAGE,
#endif
#if defined(__MESSENGER_RENEWAL__)
		LIST_ROW_COMPANION_STATUS_MESSAGE,
#endif
#else
		LIST_ROW_ACCOUNT,
		LIST_ROW_COMPANION,
#endif
	};
	void __LoadList(SQLMsg* pMsg);

#if defined(__MESSENGER_BLOCK_SYSTEM__)
	void __SendBlockList(const std::string& c_strAccount);
	void __SendBlockLogin(const std::string& c_strAccount, const std::string& c_strCompanion);
	void __SendBlockLogout(const std::string& c_strAccount, const std::string& c_strCompanion);

	enum EBlockListRows { BLOCK_LIST_ACCOUNT, BLOCK_LIST_COMPANION };
	void __LoadBlockList(SQLMsg* pMsg);
#endif

#if defined(__MESSENGER_GM__)
	void __SendGMList(const std::string& c_strAccount);
	void __SendGMLogin(const std::string& c_strAccount, const std::string& c_strCompanion);
	void __SendGMLogout(const std::string& c_strAccount, const std::string& c_strCompanion);

	enum EGMListRows : BYTE
	{
#if defined(__MESSENGER_DETAILS__)
		GM_LIST_ROW_ACCOUNT,
		GM_LIST_ROW_COMPANION,
		GM_LIST_ROW_COMPANION_LAST_PLAY,
#if defined(__MULTI_LANGUAGE_SYSTEM__)
		GM_LIST_ROW_COMPANION_LANGUAGE,
#endif
#else
		GM_LIST_ROW_ACCOUNT,
		GM_LIST_ROW_COMPANION,
#endif
	};
	void __LoadGMList(SQLMsg* pMsg);
#endif

	void __Process(const LPDESC c_lpDesc, const BYTE c_bSubHeader, std::unique_ptr<TEMP_BUFFER> pTempBuffer);

#if defined(__MESSENGER_DETAILS__)
	using CompanionData = struct SCompanionData
	{
		DWORD dwLastPlayTime;
#if defined(__MULTI_LANGUAGE_SYSTEM__)
		char szCountry[COUNTRY_NAME_MAX_LEN + 1];
#endif
	};
	using RelationData = std::pair<std::string, CompanionData>;
	using Relations = std::vector<RelationData>;
#else
	using Relations = std::set<std::string>;
#endif
	using RelationMap = std::map<std::string, Relations>;

#if defined(__MESSENGER_BLOCK_SYSTEM__)
	using BlockedRelations = std::set<std::string>;
	using BlockRelationMap = std::map<::std::string, BlockedRelations>;
#endif

private:
	std::set<std::string> m_set_strOnlineAccount;
	RelationMap m_map_strRelation;
	RelationMap m_map_strInverseRelation;
	std::set<DWORD> m_set_dwRequestToAdd;

#if defined(__MESSENGER_BLOCK_SYSTEM__)
	BlockRelationMap m_map_strBlockRelation;
	BlockRelationMap m_map_strInverseBlockRelation;
#endif

#if defined(__MESSENGER_GM__)
	RelationMap m_map_strGMRelation;
	RelationMap m_map_strGMInverseRelation;
#endif

#if defined(__MESSENGER_RENEWAL__)
	std::map<std::string, std::string> m_map_strStatusMessage;
#endif
};

#endif // __INC_MESSENGER_MANAGER_H__
