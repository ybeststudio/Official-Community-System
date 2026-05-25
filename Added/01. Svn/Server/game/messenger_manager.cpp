/**
* Filename: messenger_manager.cpp
* Author: Owsap
**/

#include "stdafx.h"
#include "messenger_manager.h"
#include "char.h"
#include "char_manager.h"
#include "desc_client.h"
#include "p2p.h"
#include "questmanager.h"
#include "crc32.h"
#include "config.h"
#include "gm.h"

#if defined(__MESSENGER_RENEWAL__)
void CMessengerManager::__ApplyCompanionConnectionState(packet_messenger_list& rListPacket, const char* c_szCompanionName, bool bCompanionOnlineInSet)
{
	rListPacket.bConnectionState = MESSENGER_CONNECTION_STATE_CONNECT;

	if (!bCompanionOnlineInSet)
	{
		rListPacket.bConnected = MESSENGER_CONNECTED_STATE_OFFLINE;
		rListPacket.bConnectionState = MESSENGER_CONNECTION_STATE_DISCONNECT;
		return;
	}

	const LPCHARACTER pkCompanion = CHARACTER_MANAGER::instance().FindPC(c_szCompanionName);
	if (pkCompanion)
		rListPacket.bConnectionState = pkCompanion->GetMessengerConnectionState();

	if (rListPacket.bConnectionState == MESSENGER_CONNECTION_STATE_DISCONNECT)
		rListPacket.bConnected = MESSENGER_CONNECTED_STATE_OFFLINE;
	else
		rListPacket.bConnected = MESSENGER_CONNECTED_STATE_ONLINE;
}

void CMessengerManager::__SendConnectionState(const std::string& c_strAccount, const std::string& c_strCompanion, BYTE bConnectionState)
{
	const LPCHARACTER c_lpChar = CHARACTER_MANAGER::instance().FindPC(c_strAccount.c_str());
	const LPDESC c_lpDesc = c_lpChar ? c_lpChar->GetDesc() : nullptr;
	if (c_lpDesc == nullptr)
		return;

	if (c_lpDesc->GetCharacter() == nullptr)
		return;

	std::unique_ptr<TEMP_BUFFER> TempBuffer = std::make_unique<TEMP_BUFFER>();
	{
		TPacketGCMessengerList ListPacket = {};
		strlcpy(ListPacket.szName, c_strCompanion.c_str(), sizeof(ListPacket.szName));
		ListPacket.bConnectionState = bConnectionState;
		if (bConnectionState == MESSENGER_CONNECTION_STATE_DISCONNECT)
			ListPacket.bConnected = MESSENGER_CONNECTED_STATE_OFFLINE;
		else
			ListPacket.bConnected = MESSENGER_CONNECTED_STATE_ONLINE;
		__FillRenewalDetails(ListPacket, c_strCompanion.c_str());
		__FillStatusMessage(ListPacket, c_strCompanion.c_str());
		TempBuffer->write(&ListPacket, sizeof(ListPacket));
	}
	__Process(c_lpDesc, MESSENGER_SUBHEADER_GC_CONNECTION_STATE, std::move(TempBuffer));
}

void CMessengerManager::__BroadcastConnectionState(const std::string& c_strCompanion, BYTE bConnectionState)
{
	if (m_map_strInverseRelation.find(c_strCompanion) == m_map_strInverseRelation.end())
		return;

#if defined(__MESSENGER_DETAILS__)
	for (const RelationData& it : m_map_strInverseRelation[c_strCompanion])
		__SendConnectionState(it.first, c_strCompanion, bConnectionState);
#else
	for (const std::string& it : m_map_strInverseRelation[c_strCompanion])
		__SendConnectionState(it, c_strCompanion, bConnectionState);
#endif
}

void CMessengerManager::SetConnectionState(LPCHARACTER c_lpChar, BYTE bConnectionState)
{
	if (c_lpChar == nullptr)
		return;

	if (bConnectionState > MESSENGER_CONNECTION_STATE_DISCONNECT)
		return;

	const BYTE bOldState = c_lpChar->GetMessengerConnectionState();
	if (bOldState == bConnectionState)
		return;

	c_lpChar->SetMessengerConnectionState(bConnectionState);

	const std::string strPlayerName = c_lpChar->GetName();
	const bool bWasVisible = (bOldState != MESSENGER_CONNECTION_STATE_DISCONNECT);
	const bool bNowVisible = (bConnectionState != MESSENGER_CONNECTION_STATE_DISCONNECT);

	if (bWasVisible && !bNowVisible)
	{
#if defined(__MESSENGER_DETAILS__)
		for (const RelationData& it : m_map_strInverseRelation[strPlayerName])
			__SendLogout(it.first, strPlayerName);
#else
		for (const std::string& it : m_map_strInverseRelation[strPlayerName])
			__SendLogout(it, strPlayerName);
#endif
	}
	else if (!bWasVisible && bNowVisible)
	{
#if defined(__MESSENGER_DETAILS__)
		for (const RelationData& it : m_map_strInverseRelation[strPlayerName])
			__SendLogin(it.first, strPlayerName);
#else
		for (const std::string& it : m_map_strInverseRelation[strPlayerName])
			__SendLogin(it, strPlayerName);
#endif
	}
	else if (bNowVisible)
	{
		__BroadcastConnectionState(strPlayerName, bConnectionState);
	}
}

void CMessengerManager::__FillRenewalDetails(packet_messenger_list& rListPacket, const char* c_szCompanionName)
{
	rListPacket.bLevel = 0;
	rListPacket.bIsConqueror = 0;
	rListPacket.bChannel = 0;
	rListPacket.lMapIndex = 0;

	if (!(rListPacket.bConnected & MESSENGER_CONNECTED_STATE_ONLINE))
		return;

	if (!c_szCompanionName || !*c_szCompanionName)
		return;

	const LPCHARACTER pkCompanion = CHARACTER_MANAGER::instance().FindPC(c_szCompanionName);
	if (!pkCompanion)
		return;

#if defined(__CONQUEROR_LEVEL__)
	if (pkCompanion->GetConquerorLevel() > 0)
	{
		rListPacket.bIsConqueror = 1;
		rListPacket.bLevel = pkCompanion->GetConquerorLevel();
	}
	else
#endif
	{
		rListPacket.bLevel = pkCompanion->GetLevel();
	}

	rListPacket.bChannel = g_bChannel;
	rListPacket.lMapIndex = pkCompanion->GetMapIndex();
}

const char* CMessengerManager::__GetStatusMessage(const char* c_szName)
{
	if (!c_szName || !*c_szName)
		return "";

	const LPCHARACTER pkChar = CHARACTER_MANAGER::instance().FindPC(c_szName);
	if (pkChar)
		return pkChar->GetStatusMessage();

	const auto it = m_map_strStatusMessage.find(c_szName);
	if (it != m_map_strStatusMessage.end())
		return it->second.c_str();

	return "";
}

void CMessengerManager::__FillStatusMessage(packet_messenger_list& rListPacket, const char* c_szCompanionName)
{
	rListPacket.szStatusMessage[0] = '\0';
	if (!c_szCompanionName || !*c_szCompanionName)
		return;

	const char* c_pszMessage = __GetStatusMessage(c_szCompanionName);
	if (c_pszMessage && *c_pszMessage)
		strlcpy(rListPacket.szStatusMessage, c_pszMessage, sizeof(rListPacket.szStatusMessage));
}

void CMessengerManager::__SaveStatusMessageToDB(const char* c_szName, const char* c_szMessage)
{
	if (!c_szName || !*c_szName)
		return;

	char szEscapedName[CHARACTER_NAME_MAX_LEN * 2 + 1] = {};
	char szEscapedMessage[MESSENGER_STATUS_MESSAGE_MAX_LEN * 2 + 1] = {};

	DBManager::instance().EscapeString(szEscapedName, sizeof(szEscapedName), c_szName, strlen(c_szName));
	DBManager::instance().EscapeString(szEscapedMessage, sizeof(szEscapedMessage),
		c_szMessage ? c_szMessage : "", c_szMessage ? strlen(c_szMessage) : 0);

	DBManager::instance().Query(
		"UPDATE `player%s` SET `status_message`='%s' WHERE `name`='%s' LIMIT 1",
		get_table_postfix(), szEscapedMessage, szEscapedName);
}

void CMessengerManager::__SendMyStatusMessage(LPCHARACTER c_lpChar)
{
	if (!c_lpChar)
		return;

	const LPDESC c_lpDesc = c_lpChar->GetDesc();
	if (!c_lpDesc)
		return;

	TPacketGCMessengerMyStatusMessage StatusPacket = {};
	strlcpy(StatusPacket.szStatusMessage, c_lpChar->GetStatusMessage(), sizeof(StatusPacket.szStatusMessage));

	TPacketGCMessenger Packet = {};
	Packet.bHeader = HEADER_GC_MESSENGER;
	Packet.bSubHeader = MESSENGER_SUBHEADER_GC_MY_STATUS_MESSAGE;
	Packet.wSize = sizeof(Packet) + sizeof(StatusPacket);

	c_lpDesc->BufferedPacket(&Packet, sizeof(Packet));
	c_lpDesc->Packet(&StatusPacket, sizeof(StatusPacket));
}

void CMessengerManager::__SendStatusMessage(const std::string& c_strAccount, const char* c_szCompanionName, const char* c_szMessage)
{
	const LPCHARACTER c_lpChar = CHARACTER_MANAGER::instance().FindPC(c_strAccount.c_str());
	if (!c_lpChar)
		return;

	const LPDESC c_lpDesc = c_lpChar->GetDesc();
	if (!c_lpDesc)
		return;

	TPacketGCMessengerStatusMessage StatusPacket = {};
	strlcpy(StatusPacket.szName, c_szCompanionName ? c_szCompanionName : "", sizeof(StatusPacket.szName));
	strlcpy(StatusPacket.szStatusMessage, c_szMessage ? c_szMessage : "", sizeof(StatusPacket.szStatusMessage));

	TPacketGCMessenger Packet = {};
	Packet.bHeader = HEADER_GC_MESSENGER;
	Packet.bSubHeader = MESSENGER_SUBHEADER_GC_STATUS_MESSAGE;
	Packet.wSize = sizeof(Packet) + sizeof(StatusPacket);

	c_lpDesc->BufferedPacket(&Packet, sizeof(Packet));
	c_lpDesc->Packet(&StatusPacket, sizeof(StatusPacket));
}

void CMessengerManager::__BroadcastStatusMessage(const std::string& c_strCompanion, const char* c_szMessage)
{
#if defined(__MESSENGER_DETAILS__)
	for (const RelationData& it : m_map_strInverseRelation[c_strCompanion])
		__SendStatusMessage(it.first, c_strCompanion.c_str(), c_szMessage);
#else
	for (const std::string& it : m_map_strInverseRelation[c_strCompanion])
		__SendStatusMessage(it, c_strCompanion.c_str(), c_szMessage);
#endif
}

void CMessengerManager::__LoadMyStatusMessage(SQLMsg* pMsg)
{
	if (!pMsg || !pMsg->Get() || pMsg->Get()->uiNumRows == 0)
		return;

	MYSQL_ROW row = mysql_fetch_row(pMsg->Get()->pSQLResult);
	if (!row)
		return;

	const char* c_pszMessage = row[0] ? row[0] : "";
	const char* c_pszAccount = row[1] ? row[1] : "";

	if (!c_pszAccount[0])
		return;

	m_map_strStatusMessage[c_pszAccount] = c_pszMessage;

	const LPCHARACTER pkChar = CHARACTER_MANAGER::instance().FindPC(c_pszAccount);
	if (pkChar)
	{
		pkChar->SetStatusMessage(c_pszMessage);
		__SendMyStatusMessage(pkChar);
	}
}

void CMessengerManager::SetStatusMessage(LPCHARACTER c_lpChar, const char* c_szMessage)
{
	if (!c_lpChar)
		return;

	if (!c_szMessage)
		c_szMessage = "";

	size_t len = strnlen(c_szMessage, MESSENGER_STATUS_MESSAGE_MAX_LEN + 1);
	if (len > MESSENGER_STATUS_MESSAGE_MAX_LEN)
		return;

	c_lpChar->SetStatusMessage(c_szMessage);
	m_map_strStatusMessage[c_lpChar->GetName()] = c_szMessage;
	__SaveStatusMessageToDB(c_lpChar->GetName(), c_szMessage);
	__SendMyStatusMessage(c_lpChar);
	__BroadcastStatusMessage(c_lpChar->GetName(), c_szMessage);
}

void CMessengerManager::DeleteStatusMessage(LPCHARACTER c_lpChar)
{
	if (!c_lpChar)
		return;

	SetStatusMessage(c_lpChar, "");
}
#endif

void CMessengerManager::Initialize()
{
}

void CMessengerManager::Destroy()
{
}

void CMessengerManager::P2PLogin(const std::string& c_strAccount)
{
	Login(c_strAccount);
}

void CMessengerManager::P2PLogout(const std::string& c_strAccount)
{
	Logout(c_strAccount);
}

void CMessengerManager::Login(const std::string& c_strAccount)
{
	if (m_set_strOnlineAccount.find(c_strAccount) != m_set_strOnlineAccount.end())
		return;

	DBManager::instance().FuncQuery(std::bind(&CMessengerManager::__LoadList, this, std::placeholders::_1),
#if defined(__MESSENGER_DETAILS__)
		"SELECT "
		"`messenger_list`.`account`, UNIX_TIMESTAMP(`player`.`last_play`) AS `last_play`"
#if defined(__MULTI_LANGUAGE_SYSTEM__)
		", `account`.`country` AS `country`"
#endif
		", `messenger_list`.`companion`, UNIX_TIMESTAMP(`companion_player`.`last_play`) AS `companion_last_play`"
#if defined(__MULTI_LANGUAGE_SYSTEM__)
		", `companion_account`.`country` AS `companion_country`"
#endif
#if defined(__MESSENGER_RENEWAL__)
		", IFNULL(`companion_player`.`status_message`, '') AS `companion_status_message`"
#endif
		" FROM `player`.`messenger_list`"
		" LEFT JOIN (SELECT `account_id`, `name`, `last_play` FROM `player`) AS `player` ON `player`.`name` = `messenger_list`.`account`"
		" LEFT JOIN (SELECT `id`, `country` FROM `account`.`account`) AS `account` ON `account`.`id` = `player`.`account_id`"
		" LEFT JOIN (SELECT `account_id`, `name`, `last_play`"
#if defined(__MESSENGER_RENEWAL__)
		", `status_message`"
#endif
		" FROM `player`) AS `companion_player` ON `companion_player`.`name` = `messenger_list`.`companion`"
		" LEFT JOIN (SELECT `id`, `country` FROM `account`.`account`) AS `companion_account` ON `companion_account`.`id` = `companion_player`.`account_id`"
		" WHERE `messenger_list`.`account` = '%s'", c_strAccount.c_str());
#else
#if defined(__MESSENGER_RENEWAL__)
		"SELECT `messenger_list`.`account`, `messenger_list`.`companion`, IFNULL(`player`.`status_message`, '') "
		"FROM `messenger_list` "
		"LEFT JOIN `player%s` AS `player` ON `player`.`name` = `messenger_list`.`companion` "
		"WHERE `messenger_list`.`account` = '%s'", get_table_postfix(), c_strAccount.c_str());
#else
		"SELECT `account`, `companion` FROM `messenger_list` WHERE `account` = '%s'", c_strAccount.c_str());
#endif
#endif

#if defined(__MESSENGER_RENEWAL__)
	{
		char szEscapedAccount[CHARACTER_NAME_MAX_LEN * 2 + 1] = {};
		DBManager::instance().EscapeString(szEscapedAccount, sizeof(szEscapedAccount),
			c_strAccount.c_str(), c_strAccount.length());
		DBManager::instance().FuncQuery(std::bind(&CMessengerManager::__LoadMyStatusMessage, this, std::placeholders::_1),
			"SELECT IFNULL(`status_message`, ''), `name` FROM `player%s` WHERE `name`='%s' LIMIT 1",
			get_table_postfix(), szEscapedAccount);
	}
#endif

#if defined(__MESSENGER_BLOCK_SYSTEM__)
	DBManager::instance().FuncQuery(std::bind(&CMessengerManager::__LoadBlockList, this, std::placeholders::_1),
		"SELECT `account`, `companion` FROM `messenger_block_list` WHERE `account` = '%s'", c_strAccount.c_str());
#endif

#if defined(__MESSENGER_GM__)
	DBManager::instance().FuncQuery(std::bind(&CMessengerManager::__LoadGMList, this, std::placeholders::_1),
#if defined(__MESSENGER_DETAILS__)
		"SELECT '%s', `gmlist`.`mName`, UNIX_TIMESTAMP(`player`.`last_play`) AS `last_play`"
#if defined(__MULTI_LANGUAGE_SYSTEM__)
		", `account`.`country` AS `country`"
#endif
		" FROM `common`.`gmlist`"
		" LEFT JOIN (SELECT `account_id`, `name`, `last_play` FROM `player`) AS `player` ON `player`.`name` = `gmlist`.`mName`"
		" LEFT JOIN (SELECT `id`, `country` FROM `account`.`account`) AS `account` ON `account`.`id` = `player`.`account_id`"
		" WHERE `gmlist`.`mAuthority` != 'PLAYER'", c_strAccount.c_str());
#else
		"SELECT '%s', `gmlist`.`mName` FROM `common`.`gmlist` WHERE `gmlist`.`mAuthority` != 'PLAYER'", c_strAccount.c_str());
#endif
#endif

	m_set_strOnlineAccount.emplace(c_strAccount);
}

void CMessengerManager::Logout(const std::string& c_strAccount)
{
	if (m_set_strOnlineAccount.find(c_strAccount) == m_set_strOnlineAccount.end())
		return;

	m_set_strOnlineAccount.erase(c_strAccount);

#if defined(__MESSENGER_DETAILS__)
	for (const RelationData& it : m_map_strInverseRelation[c_strAccount])
		__SendLogout(it.first, c_strAccount);

	for (auto& [key, vec] : m_map_strInverseRelation)
	{
		vec.erase(std::remove_if(vec.begin(), vec.end(),
			[&](const RelationData& c_rData) { return c_rData.first == c_strAccount; }),
			vec.end());
	}
#else
	for (const std::string& it : m_map_strInverseRelation[c_strAccount])
		__SendLogout(it, c_strAccount);

	RelationMap::iterator it = m_map_strInverseRelation.begin();
	while (it != m_map_strInverseRelation.end())
	{
		it->second.erase(c_strAccount);
		++it;
	}
#endif
	m_map_strRelation.erase(c_strAccount);

#if defined(__MESSENGER_BLOCK_SYSTEM__)
	for (const std::string& it : m_map_strInverseBlockRelation[c_strAccount])
		__SendBlockLogout(it, c_strAccount);

	BlockRelationMap::iterator it = m_map_strInverseBlockRelation.begin();
	while (it != m_map_strInverseBlockRelation.end())
	{
		it->second.erase(c_strAccount);
		++it;
	}

	m_map_strBlockRelation.erase(c_strAccount);
#endif

#if defined(__MESSENGER_GM__)
#if defined(__MESSENGER_DETAILS__)
	for (const RelationData& it : m_map_strGMInverseRelation[c_strAccount])
		__SendGMLogout(it.first, c_strAccount);

	for (auto& [key, vec] : m_map_strGMInverseRelation)
	{
		vec.erase(std::remove_if(vec.begin(), vec.end(),
			[&](const RelationData& c_rData) { return c_rData.first == c_strAccount; }),
			vec.end());
}
#else
	for (const std::string& it : m_map_strGMInverseRelation[c_strAccount])
		__SendGMLogout(it, c_strAccount);

	RelationMap::iterator it = m_map_strGMInverseRelation.begin();
	while (it != m_map_strGMInverseRelation.end())
	{
		it->second.erase(c_strAccount);
		++it;
	}
#endif
	m_map_strGMRelation.erase(c_strAccount);
#endif
}

void CMessengerManager::RequestToAdd(const LPCHARACTER c_lpChar, const LPCHARACTER c_lpCharTarget)
{
	if (c_lpChar == nullptr || c_lpCharTarget == nullptr)
		return;

	if (c_lpChar->IsPC() == false || c_lpCharTarget->IsPC() == false)
		return;

	if (quest::CQuestManager::instance().GetPCForce(c_lpChar->GetPlayerID())->IsRunning() == true)
	{
		c_lpChar->ChatPacket(CHAT_TYPE_INFO, LC_STRING("?????? ??? ????? ???? ?? ???? ????????."));
		return;
	}

	if (quest::CQuestManager::instance().GetPCForce(c_lpCharTarget->GetPlayerID())->IsRunning() == true)
		return;

	DWORD dwCRC32CharName = GetCRC32(c_lpChar->GetName(), strlen(c_lpChar->GetName()));
	DWORD dwCRC32CharTargetName = GetCRC32(c_lpCharTarget->GetName(), strlen(c_lpCharTarget->GetName()));

	char szBuf[64] = {};
	snprintf(szBuf, sizeof(szBuf), "%u:%u", dwCRC32CharName, dwCRC32CharTargetName);
	DWORD dwComplex = GetCRC32(szBuf, strlen(szBuf));

	m_set_dwRequestToAdd.emplace(dwComplex);

#if defined(__MESSENGER_RENEWAL__)
	BYTE bLevel = c_lpChar->GetLevel();
#if defined(__CONQUEROR_LEVEL__)
	if (c_lpChar->GetConquerorLevel() > 0)
		bLevel = c_lpChar->GetConquerorLevel();
#endif
	c_lpCharTarget->ChatPacket(CHAT_TYPE_COMMAND, "messenger_auth %s %u %u %ld",
		c_lpChar->GetName(), static_cast<unsigned>(bLevel), static_cast<unsigned>(g_bChannel), c_lpChar->GetMapIndex());
#else
	c_lpCharTarget->ChatPacket(CHAT_TYPE_COMMAND, "messenger_auth %s", c_lpChar->GetName());
#endif
}

void CMessengerManager::AuthToAdd(const std::string& c_strAccount, const std::string& c_strCompanion, bool bDeny)
{
	DWORD dwCRC32CharTargetName = GetCRC32(c_strCompanion.c_str(), c_strCompanion.length());
	DWORD dwCRC32CharName = GetCRC32(c_strAccount.c_str(), c_strAccount.length());

	char szBuf[64] = {};
	snprintf(szBuf, sizeof(szBuf), "%u:%u", dwCRC32CharTargetName, dwCRC32CharName);
	DWORD dwComplex = GetCRC32(szBuf, strlen(szBuf));

	if (m_set_dwRequestToAdd.find(dwComplex) == m_set_dwRequestToAdd.end())
	{
		sys_log(0, "MessengerManager::AuthToAdd : request not exist %s -> %s", c_strCompanion.c_str(), c_strAccount.c_str());
		return;
	}

	m_set_dwRequestToAdd.erase(dwComplex);

	if (!bDeny)
	{
		AddToList(c_strCompanion, c_strAccount);
		AddToList(c_strAccount, c_strCompanion);
	}
}

void CMessengerManager::__AddToList(const std::string& c_strAccount, const std::string& c_strCompanion)
{
#if !defined(__MESSENGER_DETAILS__)
	m_map_strRelation[c_strAccount].emplace(c_strCompanion);
	m_map_strInverseRelation[c_strCompanion].emplace(c_strAccount);
#endif

	const LPCHARACTER pkChar = CHARACTER_MANAGER::instance().FindPC(c_strAccount.c_str());
	const LPDESC pkDesc = pkChar ? pkChar->GetDesc() : nullptr;
	if (pkChar && pkDesc)
		pkChar->ChatPacket(CHAT_TYPE_INFO, LC_STRING("<?????> %s ???? ????? ???????????.", c_strCompanion.c_str()));

	const LPCHARACTER pkCharCompanion = CHARACTER_MANAGER::instance().FindPC(c_strCompanion.c_str());

#if defined(__MESSENGER_DETAILS__)
	CompanionData sAccountRowData{};
	sAccountRowData.dwLastPlayTime = pkChar ? pkChar->GetLastPlay() : 0;
#if defined(__MULTI_LANGUAGE_SYSTEM__)
	strlcpy(sAccountRowData.szCountry, pkChar ? pkChar->GetCountry() : "", sizeof(sAccountRowData.szCountry));
#endif

	CompanionData sCompanionData{};
	sCompanionData.dwLastPlayTime = pkCharCompanion ? pkCharCompanion->GetLastPlay() : 0;
#if defined(__MULTI_LANGUAGE_SYSTEM__)
	strlcpy(sCompanionData.szCountry, pkCharCompanion ? pkCharCompanion->GetCountry() : "", sizeof(sCompanionData.szCountry));
#endif

	m_map_strRelation[c_strAccount].emplace_back(c_strCompanion, sCompanionData);
	m_map_strInverseRelation[c_strCompanion].emplace_back(c_strAccount, sCompanionData);
#endif

	if (pkCharCompanion)
		__SendLogin(c_strAccount, c_strCompanion);
	else
		__SendLogout(c_strAccount, c_strCompanion);
}

void CMessengerManager::AddToList(const std::string& c_strAccount, const std::string& c_strCompanion)
{
	if (c_strCompanion.empty())
		return;

#if defined(__MESSENGER_DETAILS__)
	if (std::any_of(m_map_strRelation[c_strAccount].begin(), m_map_strRelation[c_strAccount].end(),
		[&](const RelationData& elem) { return elem.first == c_strCompanion; }))
		return;
#else
	if (m_map_strRelation[c_strAccount].find(c_strCompanion) != m_map_strRelation[c_strAccount].end())
		return;
#endif

	sys_log(0, "Messenger Add %s %s", c_strAccount.c_str(), c_strCompanion.c_str());
	DBManager::instance().Query("INSERT INTO messenger_list%s VALUES ('%s', '%s')",
		get_table_postfix(), c_strAccount.c_str(), c_strCompanion.c_str());

	__AddToList(c_strAccount, c_strCompanion);

	TPacketGGMessenger P2PPacket = {};
	P2PPacket.bHeader = HEADER_GG_MESSENGER_ADD;
	strlcpy(P2PPacket.szAccount, c_strAccount.c_str(), sizeof(P2PPacket.szAccount));
	strlcpy(P2PPacket.szCompanion, c_strCompanion.c_str(), sizeof(P2PPacket.szCompanion));
	P2P_MANAGER::instance().Send(&P2PPacket, sizeof(TPacketGGMessenger));
}

void CMessengerManager::__RemoveFromList(const std::string& c_strAccount, const std::string& c_strCompanion)
{
#if defined(__MESSENGER_DETAILS__)
	Relations& vRelation = m_map_strRelation[c_strAccount];
	m_map_strRelation[c_strAccount].erase(std::remove_if(vRelation.begin(), vRelation.end(),
		[&](const RelationData& c_rData) { return c_rData.first == c_strCompanion; }),
		vRelation.end());
	
	Relations& vInverseRelation = m_map_strInverseRelation[c_strCompanion];
	m_map_strInverseRelation[c_strCompanion].erase(std::remove_if(vInverseRelation.begin(), vInverseRelation.end(),
		[&](const RelationData& data) { return data.first == c_strAccount; }),
		vInverseRelation.end());
#else
	m_map_strRelation[c_strAccount].erase(c_strCompanion);
	m_map_strInverseRelation[c_strCompanion].erase(c_strAccount);
#endif

	const LPCHARACTER c_lpChar= CHARACTER_MANAGER::instance().FindPC(c_strAccount.c_str());
	const LPDESC c_lpDesc = c_lpChar ? c_lpChar->GetDesc() : nullptr;
	if (c_lpDesc)
		c_lpChar->ChatPacket(CHAT_TYPE_INFO, LC_STRING("<?????> %s ???? ????????? ????????????.", c_strCompanion.c_str()));
}

void CMessengerManager::RemoveFromList(const std::string& c_strAccount, const std::string& c_strCompanion)
{
	if (c_strCompanion.empty())
		return;

	if (!IsInList(c_strAccount, c_strCompanion))
	{
		const LPCHARACTER c_lpChar = CHARACTER_MANAGER::instance().FindPC(c_strAccount.c_str());
		const LPDESC c_lpDesc = c_lpChar ? c_lpChar->GetDesc() : nullptr;
		if (c_lpDesc)
			c_lpChar->ChatPacket(CHAT_TYPE_INFO, LC_STRING("%s ???? ????? ???? ??????.", c_strCompanion.c_str()));

		return;
	}

	char szCompanion[CHARACTER_NAME_MAX_LEN + 1] = {};
	DBManager::instance().EscapeString(szCompanion, sizeof(szCompanion), c_strCompanion.c_str(), c_strCompanion.length());
	DBManager::instance().Query("DELETE FROM messenger_list%s WHERE `account` = '%s' AND `companion` = '%s'",
		get_table_postfix(), c_strAccount.c_str(), szCompanion);

	__RemoveFromList(c_strAccount, c_strCompanion);

	sys_log(1, "Messenger Remove %s %s", c_strAccount.c_str(), c_strCompanion.c_str());

	TPacketGGMessenger P2PPacket = {};
	P2PPacket.bHeader = HEADER_GG_MESSENGER_REMOVE;
	strlcpy(P2PPacket.szAccount, c_strAccount.c_str(), sizeof(P2PPacket.szAccount));
	strlcpy(P2PPacket.szCompanion, c_strCompanion.c_str(), sizeof(P2PPacket.szCompanion));
	P2P_MANAGER::instance().Send(&P2PPacket, sizeof(TPacketGGMessenger));
}

void CMessengerManager::RemoveAllList(const std::string& c_strAccount)
{
	/* SQL Data ???? */
	DBManager::instance().Query("DELETE FROM messenger_list%s WHERE account = '%s' OR companion = '%s'",
		get_table_postfix(), c_strAccount.c_str(), c_strAccount.c_str());

	/* ???? ????????? ????? ???? */
#if defined(__MESSENGER_DETAILS__)
	for (const RelationData& it : m_map_strRelation[c_strAccount])
		RemoveFromList(c_strAccount, it.first);
#else
	for (const std::string& it : m_map_strRelation[c_strAccount])
		RemoveFromList(c_strAccount, it);
#endif

	/* ?????? ????? ???? */
	m_map_strRelation[c_strAccount].clear();
}

bool CMessengerManager::IsInList(const std::string& c_strAccount, const std::string& c_strCompanion)
{
	if (m_map_strRelation.find(c_strAccount) == m_map_strRelation.end())
		return false;

	if (m_map_strRelation[c_strAccount].empty())
		return false;

#if defined(__MESSENGER_DETAILS__)
	return std::any_of(m_map_strRelation[c_strAccount].begin(), m_map_strRelation[c_strAccount].end(),
		[&](const RelationData& c_rData) { return c_rData.first == c_strCompanion; });
#else
	return m_map_strRelation[c_strAccount].find(c_strCompanion) != m_map_strRelation[c_strAccount].end();
#endif
}

bool CMessengerManager::IsFriend(const char* c_szAccount, const char* c_szName)
{
	if (m_map_strRelation.empty())
		return false;

#if defined(__MESSENGER_DETAILS__)
	for (const RelationData& it : m_map_strRelation[c_szAccount])
	{
		if (it.first.compare(c_szName) == 0)
			return true;
	}
#else
	for (const std::string& it : m_map_strRelation[c_szAccount])
	{
		if (it.compare(c_szName) == 0)
			return true;
	}
#endif

	return false;
}

#if defined(__MESSENGER_BLOCK_SYSTEM__)
void CMessengerManager::__AddToBlockList(const std::string& c_strAccount, const std::string& c_strCompanion)
{
	m_map_strBlockRelation[c_strAccount].emplace(c_strCompanion);
	m_map_strInverseBlockRelation[c_strCompanion].emplace(c_strAccount);

	const LPCHARACTER c_lpChar = CHARACTER_MANAGER::instance().FindPC(c_strAccount.c_str());
	const LPDESC c_lpDesc = c_lpChar ? c_lpChar->GetDesc() : nullptr;
	if (c_lpDesc)
		c_lpChar->ChatPacket(CHAT_TYPE_INFO, LC_STRING("%s is now blocked.", c_strCompanion.c_str()));

	const LPCHARACTER c_lpCharBlocked = CHARACTER_MANAGER::instance().FindPC(c_strCompanion.c_str());
	if (c_lpCharBlocked)
		__SendBlockLogin(c_strAccount, c_strCompanion);
	else
		__SendBlockLogout(c_strAccount, c_strCompanion);
}

void CMessengerManager::AddToBlockList(const std::string& c_strAccount, const std::string& c_strCompanion)
{
	if (c_strCompanion.size() == 0)
		return;

	if (m_map_strBlockRelation[c_strAccount].find(c_strCompanion) != m_map_strBlockRelation[c_strAccount].end())
		return;

	DBManager::instance().Query("INSERT INTO messenger_block_list%s VALUES ('%s', '%s', NOW())",
		get_table_postfix(), c_strAccount.c_str(), c_strCompanion.c_str());

	__AddToBlockList(c_strAccount, c_strCompanion);

	TPacketGGMessenger P2PPacket = {};
	P2PPacket.bHeader = HEADER_GG_MESSENGER_BLOCK_ADD;
	strlcpy(P2PPacket.szAccount, c_strAccount.c_str(), sizeof(P2PPacket.szAccount));
	strlcpy(P2PPacket.szCompanion, c_strCompanion.c_str(), sizeof(P2PPacket.szCompanion));
	P2P_MANAGER::instance().Send(&P2PPacket, sizeof(TPacketGGMessenger));
}

void CMessengerManager::__RemoveFromBlockList(const std::string& c_strAccount, const std::string& c_strCompanion)
{
	m_map_strBlockRelation[c_strAccount].erase(c_strCompanion);
	m_map_strInverseBlockRelation[c_strCompanion].erase(c_strAccount);

	const LPCHARACTER c_lpChar = CHARACTER_MANAGER::instance().FindPC(c_strAccount.c_str());
	const LPDESC c_lpDesc = c_lpChar ? c_lpChar->GetDesc() : nullptr;
	if (c_lpDesc)
		c_lpChar->ChatPacket(CHAT_TYPE_INFO, LC_STRING("%s is no longer blocked.", c_strCompanion.c_str()));
}

void CMessengerManager::RemoveFromBlockList(const std::string& c_strAccount, const std::string& c_strCompanion)
{
	if (c_strCompanion.size() == 0)
		return;

	char szCompanion[CHARACTER_NAME_MAX_LEN + 1] = {};
	DBManager::instance().EscapeString(szCompanion, sizeof(szCompanion), c_strCompanion.c_str(), c_strCompanion.length());
	DBManager::instance().Query("DELETE FROM messenger_block_list%s WHERE `account` = '%s' AND `companion` = '%s'",
		get_table_postfix(), c_strAccount.c_str(), szCompanion);

	__RemoveFromBlockList(c_strAccount, c_strCompanion);

	TPacketGGMessenger P2PPacket = {};
	P2PPacket.bHeader = HEADER_GG_MESSENGER_BLOCK_REMOVE;
	strlcpy(P2PPacket.szAccount, c_strAccount.c_str(), sizeof(P2PPacket.szAccount));
	strlcpy(P2PPacket.szCompanion, c_strCompanion.c_str(), sizeof(P2PPacket.szCompanion));
	P2P_MANAGER::instance().Send(&P2PPacket, sizeof(TPacketGGMessenger));
}

void CMessengerManager::RemoveAllBlockList(const std::string& c_strAccount)
{
	/* SQL Data ???? */
	DBManager::instance().Query("DELETE FROM messenger_list%s WHERE account = '%s' OR companion = '%s'",
		get_table_postfix(), c_strAccount.c_str(), c_strAccount.c_str());

	/* ???? ????????? ????? ???? */
	for (const std::string& it : m_map_strBlockRelation[c_strAccount])
		RemoveFromBlockList(c_strAccount, it);

	/* ?????? ????? ???? */
	m_map_strBlockRelation[c_strAccount].clear();
}

bool CMessengerManager::IsBlocked(const char* c_szAccount, const char* c_szName)
{
	if (m_map_strBlockRelation.empty())
		return false;

	for (const std::string& it : m_map_strBlockRelation[c_szAccount])
	{
		if (it.compare(c_szName) == 0)
			return true;
	}

	return false;
}
#endif

void CMessengerManager::__SendList(const std::string& c_strAccount)
{
	const LPCHARACTER c_lpChar = CHARACTER_MANAGER::instance().FindPC(c_strAccount.c_str());
	if (c_lpChar == nullptr)
		return;

	const LPDESC c_lpDesc = c_lpChar->GetDesc();
	if (c_lpDesc == nullptr)
		return;

	if (m_map_strRelation.find(c_strAccount) == m_map_strRelation.end())
		return;

	if (m_map_strRelation[c_strAccount].empty())
		return;

	std::unique_ptr<TEMP_BUFFER> TempBuffer = std::make_unique<TEMP_BUFFER>(128 * 1024);
#if defined(__MESSENGER_DETAILS__)
	for (const RelationData& it : m_map_strRelation[c_strAccount])
#else
	for (const Relations::value_type& it : m_map_strRelation[c_strAccount])
#endif
	{
		TPacketGCMessengerList ListPacket = {};
#if defined(__MESSENGER_DETAILS__)
		strlcpy(ListPacket.szName, it.first.c_str(), sizeof(ListPacket.szName));
		const bool bCompanionOnline = (m_set_strOnlineAccount.find(it.first) != m_set_strOnlineAccount.end());
#else
		strlcpy(ListPacket.szName, it.c_str(), sizeof(ListPacket.szName));
		const bool bCompanionOnline = (m_set_strOnlineAccount.find(it) != m_set_strOnlineAccount.end());
#endif
#if defined(__MESSENGER_RENEWAL__)
		__ApplyCompanionConnectionState(ListPacket,
#if defined(__MESSENGER_DETAILS__)
			it.first.c_str(),
#else
			it.c_str(),
#endif
			bCompanionOnline);
#else
		if (bCompanionOnline)
		{
			ListPacket.bConnected = MESSENGER_CONNECTED_STATE_ONLINE;
#if defined(__MESSENGER_DETAILS__)
			ListPacket.dwLastPlayTime = 0;
#endif
		}
		else
		{
			ListPacket.bConnected = MESSENGER_CONNECTED_STATE_OFFLINE;
#if defined(__MESSENGER_DETAILS__)
			ListPacket.dwLastPlayTime = it.second.dwLastPlayTime;
#endif
		}
#endif
#if defined(__MESSENGER_DETAILS__)
		if (ListPacket.bConnected & MESSENGER_CONNECTED_STATE_ONLINE)
			ListPacket.dwLastPlayTime = 0;
		else
			ListPacket.dwLastPlayTime = it.second.dwLastPlayTime;
#endif
#if defined(__MULTI_LANGUAGE_SYSTEM__) && defined(__MESSENGER_DETAILS__)
		strlcpy(ListPacket.szCountry, it.second.szCountry, sizeof(ListPacket.szCountry));
#endif
#if defined(__MESSENGER_RENEWAL__)
		__FillRenewalDetails(ListPacket,
#if defined(__MESSENGER_DETAILS__)
			it.first.c_str()
#else
			it.c_str()
#endif
		);
		__FillStatusMessage(ListPacket,
#if defined(__MESSENGER_DETAILS__)
			it.first.c_str()
#else
			it.c_str()
#endif
		);
#endif
		TempBuffer->write(&ListPacket, sizeof(ListPacket));
	}

	__Process(c_lpDesc, MESSENGER_SUBHEADER_GC_LIST, std::move(TempBuffer));
}

void CMessengerManager::__SendLogin(const std::string& c_strAccount, const std::string& c_strCompanion)
{
	const LPCHARACTER c_lpChar = CHARACTER_MANAGER::instance().FindPC(c_strAccount.c_str());
	const LPDESC c_lpDesc = c_lpChar ? c_lpChar->GetDesc() : nullptr;
	if (c_lpDesc == nullptr)
		return;

	if (c_lpDesc->GetCharacter() == nullptr)
		return;

	if (c_lpChar->GetGMLevel() == GM_PLAYER && gm_get_level(c_strCompanion.c_str()) != GM_PLAYER)
		return;

	std::unique_ptr<TEMP_BUFFER> TempBuffer = std::make_unique<TEMP_BUFFER>();
	{
		TPacketGCMessengerList ListPacket = {};
		strlcpy(ListPacket.szName, c_strCompanion.c_str(), sizeof(ListPacket.szName));
#if defined(__MESSENGER_RENEWAL__)
		__ApplyCompanionConnectionState(ListPacket, c_strCompanion.c_str(), true);
#else
		ListPacket.bConnected = MESSENGER_CONNECTED_STATE_ONLINE;
#endif
#if defined(__MESSENGER_DETAILS__)
		const Relations& vRelations = m_map_strRelation[c_strAccount];
		auto it = std::find_if(vRelations.begin(), vRelations.end(),
			[&](const RelationData& c_rData) { return c_rData.first == c_strCompanion; });
		if (it != vRelations.end())
		{
			const CompanionData& c_rCompanionData = it->second;
			ListPacket.dwLastPlayTime = 0;
#if defined(__MULTI_LANGUAGE_SYSTEM__)
			strlcpy(ListPacket.szCountry, c_rCompanionData.szCountry, sizeof(ListPacket.szCountry));
#endif
		}
		else
		{
			ListPacket.dwLastPlayTime = 0;
#if defined(__MULTI_LANGUAGE_SYSTEM__)
			strlcpy(ListPacket.szCountry, "", sizeof(ListPacket.szCountry));
#endif
		}
#endif
#if defined(__MESSENGER_RENEWAL__)
		__FillRenewalDetails(ListPacket, c_strCompanion.c_str());
		__FillStatusMessage(ListPacket, c_strCompanion.c_str());
#endif
		TempBuffer->write(&ListPacket, sizeof(ListPacket));
	}
	__Process(c_lpDesc, MESSENGER_SUBHEADER_GC_LOGIN, std::move(TempBuffer));
}

void CMessengerManager::__SendLogout(const std::string& c_strAccount, const std::string& c_strCompanion)
{
	if (!c_strCompanion.size())
		return;

	const LPCHARACTER c_lpChar = CHARACTER_MANAGER::instance().FindPC(c_strAccount.c_str());
	const LPDESC c_lpDesc = c_lpChar ? c_lpChar->GetDesc() : nullptr;
	if (c_lpDesc == nullptr)
		return;

	std::unique_ptr<TEMP_BUFFER> TempBuffer = std::make_unique<TEMP_BUFFER>();
	{
		TPacketGCMessengerList ListPacket = {};
		strlcpy(ListPacket.szName, c_strCompanion.c_str(), sizeof(ListPacket.szName));
		ListPacket.bConnected = MESSENGER_CONNECTED_STATE_OFFLINE;
#if defined(__MESSENGER_DETAILS__)
		const Relations& vRelations = m_map_strRelation[c_strAccount];
		auto it = std::find_if(vRelations.begin(), vRelations.end(),
			[&](const RelationData& c_rData) { return c_rData.first == c_strCompanion; });
		if (it != vRelations.end())
		{
			const CompanionData& c_rCompanionData = it->second;
			ListPacket.dwLastPlayTime = c_rCompanionData.dwLastPlayTime;
#if defined(__MULTI_LANGUAGE_SYSTEM__)
			strlcpy(ListPacket.szCountry, c_rCompanionData.szCountry, sizeof(ListPacket.szCountry));
#endif
		}
		else
		{
			ListPacket.dwLastPlayTime = 0;
#if defined(__MULTI_LANGUAGE_SYSTEM__)
			strlcpy(ListPacket.szCountry, "", sizeof(ListPacket.szCountry));
#endif
		}
#endif
		TempBuffer->write(&ListPacket, sizeof(ListPacket));
	}
	__Process(c_lpDesc, MESSENGER_SUBHEADER_GC_LOGOUT, std::move(TempBuffer));
}

void CMessengerManager::__LoadList(SQLMsg* pMsg)
{
	if (pMsg == nullptr)
		return;

	if (pMsg->Get() == nullptr)
		return;

	if (pMsg->Get()->uiNumRows == 0)
		return;

	sys_log(1, "Messenger::LoadList");

	std::string strAccount;
	for (UINT uiRow = 0; uiRow < pMsg->Get()->uiNumRows; ++uiRow)
	{
		MYSQL_ROW cRow = mysql_fetch_row(pMsg->Get()->pSQLResult);
		if (cRow[LIST_ROW_ACCOUNT] && cRow[LIST_ROW_COMPANION])
		{
			if (strAccount.length() == 0)
				strAccount = cRow[LIST_ROW_ACCOUNT];

#if defined(__MESSENGER_DETAILS__)
			CompanionData sAccountRowData{};
			sAccountRowData.dwLastPlayTime = static_cast<DWORD>(cRow[LIST_ROW_ACCOUNT_LAST_PLAY] ? atol(cRow[LIST_ROW_ACCOUNT_LAST_PLAY]) : 0);
#if defined(__MULTI_LANGUAGE_SYSTEM__)
			strlcpy(sAccountRowData.szCountry, cRow[LIST_ROW_ACCOUNT_LANGUAGE] ? cRow[LIST_ROW_ACCOUNT_LANGUAGE] : NULL, sizeof(sAccountRowData.szCountry));
#endif

			CompanionData sCompanionRowData{};
			sCompanionRowData.dwLastPlayTime = static_cast<DWORD>(cRow[LIST_ROW_COMPANION_LAST_PLAY] ? atol(cRow[LIST_ROW_COMPANION_LAST_PLAY]) : 0);
#if defined(__MULTI_LANGUAGE_SYSTEM__)
			strlcpy(sCompanionRowData.szCountry, cRow[LIST_ROW_COMPANION_LANGUAGE] ? cRow[LIST_ROW_COMPANION_LANGUAGE] : NULL, sizeof(sCompanionRowData.szCountry));
#endif

			m_map_strRelation[cRow[LIST_ROW_ACCOUNT]].emplace_back(cRow[LIST_ROW_COMPANION], sCompanionRowData);
			m_map_strInverseRelation[cRow[LIST_ROW_COMPANION]].emplace_back(cRow[LIST_ROW_ACCOUNT], sAccountRowData);
#else
			m_map_strRelation[cRow[LIST_ROW_ACCOUNT]].emplace(cRow[LIST_ROW_COMPANION]);
			m_map_strInverseRelation[cRow[LIST_ROW_COMPANION]].emplace(cRow[LIST_ROW_ACCOUNT]);
#endif
#if defined(__MESSENGER_RENEWAL__)
#if defined(__MESSENGER_DETAILS__)
			if (cRow[LIST_ROW_COMPANION_STATUS_MESSAGE])
				m_map_strStatusMessage[cRow[LIST_ROW_COMPANION]] = cRow[LIST_ROW_COMPANION_STATUS_MESSAGE];
#else
			if (cRow[2])
				m_map_strStatusMessage[cRow[LIST_ROW_COMPANION]] = cRow[2];
#endif
#endif
		}
	}

	__SendList(strAccount);
#if defined(__MESSENGER_DETAILS__)
	for (const RelationData& it : m_map_strInverseRelation[strAccount])
		__SendLogin(it.first, strAccount);
#else
	for (const std::string& it : m_map_strInverseRelation[strAccount])
		__SendLogin(it, strAccount);
#endif
}

#if defined(__MESSENGER_BLOCK_SYSTEM__)
void CMessengerManager::__SendBlockList(const std::string& c_strAccount)
{
	const LPCHARACTER c_lpChar = CHARACTER_MANAGER::instance().FindPC(c_strAccount.c_str());
	if (c_lpChar == nullptr)
		return;

	const LPDESC c_lpDesc = c_lpChar->GetDesc();
	if (c_lpDesc == nullptr)
		return;

	if (m_map_strBlockRelation.find(c_strAccount) == m_map_strBlockRelation.end())
		return;

	if (m_map_strBlockRelation[c_strAccount].empty())
		return;

	std::unique_ptr<TEMP_BUFFER> TempBuffer = std::make_unique<TEMP_BUFFER>(128 * 1024);
	for (const std::string& it : m_map_strBlockRelation[c_strAccount])
	{
		TPacketGCMessengerList ListPacket = {};
		strlcpy(ListPacket.szName, it.c_str(), sizeof(ListPacket.szName));
		ListPacket.bConnected = MESSENGER_CONNECTED_STATE_OFFLINE;
		TempBuffer->write(&ListPacket, sizeof(ListPacket));
	}

	__Process(c_lpDesc, MESSENGER_SUBHEADER_GC_BLOCK_LIST, std::move(TempBuffer));
}

void CMessengerManager::__SendBlockLogin(const std::string& c_strAccount, const std::string& c_strCompanion)
{
	const LPCHARACTER c_lpChar = CHARACTER_MANAGER::instance().FindPC(c_strAccount.c_str());
	const LPDESC c_lpDesc = c_lpChar ? c_lpChar->GetDesc() : nullptr;
	if (c_lpDesc == nullptr)
		return;

	if (c_lpDesc->GetCharacter() == nullptr)
		return;

	std::unique_ptr<TEMP_BUFFER> TempBuffer = std::make_unique<TEMP_BUFFER>();
	{
		TPacketGCMessengerList ListPacket = {};
		strlcpy(ListPacket.szName, c_strCompanion.c_str(), sizeof(ListPacket.szName));
		ListPacket.bConnected = MESSENGER_CONNECTED_STATE_OFFLINE;
		TempBuffer->write(&ListPacket, sizeof(ListPacket));
	}
	__Process(c_lpDesc, MESSENGER_SUBHEADER_GC_BLOCK_LOGIN, std::move(TempBuffer));
}

void CMessengerManager::__SendBlockLogout(const std::string& c_strAccount, const std::string& c_strCompanion)
{
	if (!c_strCompanion.size())
		return;

	const LPCHARACTER c_lpChar = CHARACTER_MANAGER::instance().FindPC(c_strAccount.c_str());
	const LPDESC c_lpDesc = c_lpChar ? c_lpChar->GetDesc() : nullptr;
	if (c_lpDesc == nullptr)
		return;

	std::unique_ptr<TEMP_BUFFER> TempBuffer = std::make_unique<TEMP_BUFFER>();
	{
		TPacketGCMessengerList ListPacket = {};
		strlcpy(ListPacket.szName, c_strCompanion.c_str(), sizeof(ListPacket.szName));
		ListPacket.bConnected = MESSENGER_CONNECTED_STATE_OFFLINE;
		TempBuffer->write(&ListPacket, sizeof(ListPacket));
	}
	__Process(c_lpDesc, MESSENGER_SUBHEADER_GC_BLOCK_LOGOUT, std::move(TempBuffer));
}

void CMessengerManager::__LoadBlockList(SQLMsg* pMsg)
{
	if (pMsg == nullptr)
		return;

	if (pMsg->Get() == nullptr)
		return;

	if (pMsg->Get()->uiNumRows == 0)
		return;

	sys_log(1, "Messenger::LoadBlockList");

	std::string strAccount;
	for (UINT uiRow = 0; uiRow < pMsg->Get()->uiNumRows; ++uiRow)
	{
		MYSQL_ROW cRow = mysql_fetch_row(pMsg->Get()->pSQLResult);
		if (cRow[BLOCK_LIST_ACCOUNT] && cRow[BLOCK_LIST_COMPANION])
		{
			if (strAccount.length() == 0)
				strAccount = cRow[BLOCK_LIST_ACCOUNT];

			m_map_strBlockRelation[cRow[BLOCK_LIST_ACCOUNT]].emplace(cRow[BLOCK_LIST_COMPANION]);
			m_map_strInverseBlockRelation[cRow[BLOCK_LIST_COMPANION]].emplace(cRow[BLOCK_LIST_ACCOUNT]);
		}
	}

	__SendBlockList(strAccount);
	for (const std::string& it : m_map_strInverseBlockRelation[strAccount])
		__SendBlockLogin(it, strAccount);
}
#endif

#if defined(__MESSENGER_GM__)
void CMessengerManager::__SendGMList(const std::string& c_strAccount)
{
	const LPCHARACTER c_lpChar = CHARACTER_MANAGER::instance().FindPC(c_strAccount.c_str());
	if (c_lpChar == nullptr)
		return;

	const LPDESC c_lpDesc = c_lpChar->GetDesc();
	if (c_lpDesc == nullptr)
		return;

	if (m_map_strGMRelation.find(c_strAccount) == m_map_strGMRelation.end())
		return;

	if (m_map_strGMRelation[c_strAccount].empty())
		return;

	std::unique_ptr<TEMP_BUFFER> TempBuffer = std::make_unique<TEMP_BUFFER>(128 * 1024);
	for (const RelationData& it : m_map_strGMRelation[c_strAccount])
	{
		TPacketGCMessengerList ListPacket = {};
		strlcpy(ListPacket.szName, it.first.c_str(), sizeof(ListPacket.szName));
		if (m_set_strOnlineAccount.find(it.first) != m_set_strOnlineAccount.end())
		{
			ListPacket.bConnected = MESSENGER_CONNECTED_STATE_ONLINE;
#if defined(__MESSENGER_DETAILS__)
			ListPacket.dwLastPlayTime = 0;
#endif
		}
		else
		{
			ListPacket.bConnected = MESSENGER_CONNECTED_STATE_OFFLINE;
#if defined(__MESSENGER_DETAILS__)
			ListPacket.dwLastPlayTime = it.second.dwLastPlayTime;
#endif
		}
#if defined(__MULTI_LANGUAGE_SYSTEM__) && defined(__MESSENGER_DETAILS__)
		strlcpy(ListPacket.szCountry, it.second.szCountry, sizeof(ListPacket.szCountry));
#endif
		TempBuffer->write(&ListPacket, sizeof(ListPacket));
	}

	__Process(c_lpDesc, MESSENGER_SUBHEADER_GC_GM_LIST, std::move(TempBuffer));
}

void CMessengerManager::__SendGMLogin(const std::string& c_strAccount, const std::string& c_strCompanion)
{
	const LPCHARACTER c_lpChar = CHARACTER_MANAGER::instance().FindPC(c_strAccount.c_str());
	const LPDESC c_lpDesc = c_lpChar ? c_lpChar->GetDesc() : nullptr;
	if (c_lpDesc == nullptr)
		return;

	if (c_lpDesc->GetCharacter() == nullptr)
		return;

	std::unique_ptr<TEMP_BUFFER> TempBuffer = std::make_unique<TEMP_BUFFER>();
	{
		TPacketGCMessengerList ListPacket = {};
		strlcpy(ListPacket.szName, c_strCompanion.c_str(), sizeof(ListPacket.szName));
		ListPacket.bConnected = MESSENGER_CONNECTED_STATE_ONLINE;
#if defined(__MESSENGER_DETAILS__)
		const Relations& vRelations = m_map_strGMRelation[c_strAccount];
		auto it = std::find_if(vRelations.begin(), vRelations.end(),
			[&](const RelationData& c_rData) { return c_rData.first == c_strCompanion; });
		if (it != vRelations.end())
		{
			const CompanionData& c_rCompanionData = it->second;
			ListPacket.dwLastPlayTime = 0;
#if defined(__MULTI_LANGUAGE_SYSTEM__)
			strlcpy(ListPacket.szCountry, c_rCompanionData.szCountry, sizeof(ListPacket.szCountry));
#endif
		}
		else
		{
			ListPacket.dwLastPlayTime = 0;
#if defined(__MULTI_LANGUAGE_SYSTEM__)
			strlcpy(ListPacket.szCountry, NULL, sizeof(ListPacket.szCountry));
#endif
		}
#endif
		TempBuffer->write(&ListPacket, sizeof(ListPacket));
	}
	__Process(c_lpDesc, MESSENGER_SUBHEADER_GC_GM_LOGIN, std::move(TempBuffer));
}

void CMessengerManager::__SendGMLogout(const std::string& c_strAccount, const std::string& c_strCompanion)
{
	if (!c_strCompanion.size())
		return;

	const LPCHARACTER c_lpChar = CHARACTER_MANAGER::instance().FindPC(c_strAccount.c_str());
	const LPDESC c_lpDesc = c_lpChar ? c_lpChar->GetDesc() : nullptr;
	if (c_lpDesc == nullptr)
		return;

	std::unique_ptr<TEMP_BUFFER> TempBuffer = std::make_unique<TEMP_BUFFER>();
	{
		TPacketGCMessengerList ListPacket = {};
		strlcpy(ListPacket.szName, c_strCompanion.c_str(), sizeof(ListPacket.szName));
		ListPacket.bConnected = MESSENGER_CONNECTED_STATE_OFFLINE;
#if defined(__MESSENGER_DETAILS__)
		const Relations& vRelations = m_map_strGMRelation[c_strAccount];
		auto it = std::find_if(vRelations.begin(), vRelations.end(),
			[&](const RelationData& c_rData) { return c_rData.first == c_strCompanion; });
		if (it != vRelations.end())
		{
			const CompanionData& c_rCompanionData = it->second;
			ListPacket.dwLastPlayTime = c_rCompanionData.dwLastPlayTime;
#if defined(__MULTI_LANGUAGE_SYSTEM__)
			strlcpy(ListPacket.szCountry, c_rCompanionData.szCountry, sizeof(ListPacket.szCountry));
#endif
		}
		else
		{
			ListPacket.dwLastPlayTime = 0;
#if defined(__MULTI_LANGUAGE_SYSTEM__)
			strlcpy(ListPacket.szCountry, NULL, sizeof(ListPacket.szCountry));
#endif
		}
#endif
		TempBuffer->write(&ListPacket, sizeof(ListPacket));
	}
	__Process(c_lpDesc, MESSENGER_SUBHEADER_GC_GM_LOGOUT, std::move(TempBuffer));
}

void CMessengerManager::__LoadGMList(SQLMsg* pMsg)
{
	if (pMsg == nullptr)
		return;

	if (pMsg->Get() == nullptr)
		return;

	if (pMsg->Get()->uiNumRows == 0)
		return;

	sys_log(1, "Messenger::LoadGMList");

	std::string strAccount;
	for (UINT uiRow = 0; uiRow < pMsg->Get()->uiNumRows; ++uiRow)
	{
		MYSQL_ROW cRow = mysql_fetch_row(pMsg->Get()->pSQLResult);
		if (cRow[GM_LIST_ROW_ACCOUNT] && cRow[GM_LIST_ROW_COMPANION])
		{
			if (strAccount.length() == 0)
				strAccount = cRow[GM_LIST_ROW_ACCOUNT];

#if defined(__MESSENGER_DETAILS__)
			CompanionData sCompanion{};
			sCompanion.dwLastPlayTime = static_cast<DWORD>(cRow[GM_LIST_ROW_COMPANION_LAST_PLAY] ? atol(cRow[GM_LIST_ROW_COMPANION_LAST_PLAY]) : 0);
#if defined(__MULTI_LANGUAGE_SYSTEM__)
			strlcpy(sCompanion.szCountry, cRow[GM_LIST_ROW_COMPANION_LANGUAGE] ? cRow[GM_LIST_ROW_COMPANION_LANGUAGE] : "", sizeof(sCompanion.szCountry));
#endif
			m_map_strGMRelation[cRow[GM_LIST_ROW_ACCOUNT]].emplace_back(cRow[GM_LIST_ROW_COMPANION], sCompanion);
			m_map_strGMInverseRelation[cRow[GM_LIST_ROW_COMPANION]].emplace_back(cRow[GM_LIST_ROW_ACCOUNT], sCompanion);
#else
			m_map_strGMRelation[cRow[GM_LIST_ROW_ACCOUNT]].emplace(cRow[GM_LIST_ROW_COMPANION]);
			m_map_strGMInverseRelation[cRow[GM_LIST_ROW_COMPANION]].emplace(cRow[GM_LIST_ROW_ACCOUNT]);
#endif
		}
	}

	__SendGMList(strAccount);
#if defined(__MESSENGER_DETAILS__)
	for (const RelationData& it : m_map_strGMInverseRelation[strAccount])
		__SendGMLogin(it.first, strAccount);
#else
	for (const std::string& it : m_map_strGMInverseRelation[strAccount])
		__SendGMLogin(it, strAccount);
#endif
}
#endif

void CMessengerManager::__Process(const LPDESC c_lpDesc, const BYTE c_bSubHeader, std::unique_ptr<TEMP_BUFFER> pTempBuffer)
{
	if (c_lpDesc == nullptr || pTempBuffer.get() == nullptr)
		return;

	TPacketGCMessenger Packet = {};
	Packet.bHeader = HEADER_GC_MESSENGER;
	Packet.bSubHeader = c_bSubHeader;
	Packet.wSize = sizeof(Packet) + pTempBuffer->size();
	if (pTempBuffer->size())
	{
		c_lpDesc->BufferedPacket(&Packet, sizeof(Packet));
		c_lpDesc->Packet(pTempBuffer->read_peek(), pTempBuffer->size());
	}
}
