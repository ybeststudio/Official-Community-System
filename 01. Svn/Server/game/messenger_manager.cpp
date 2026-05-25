// Add the following `CMessengerManager::__ApplyCompanionConnectionState` function anywhere in this file:
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

// Before
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

// After
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

// Before
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

// After
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

// Before
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

// After
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

// Add the following `DBManager::instance` function anywhere in this file:
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

// Find this line:
m_set_dwRequestToAdd.emplace(dwComplex);

// Add after it:
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

// Before
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

// After
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

// Before
		else
			ListPacket.dwLastPlayTime = it.second.dwLastPlayTime;
#endif
#if defined(__MULTI_LANGUAGE_SYSTEM__) && defined(__MESSENGER_DETAILS__)
		strlcpy(ListPacket.szCountry, it.second.szCountry, sizeof(ListPacket.szCountry));
#endif
		TempBuffer->write(&ListPacket, sizeof(ListPacket));
	}

	__Process(c_lpDesc, MESSENGER_SUBHEADER_GC_LIST, std::move(TempBuffer));
}

void CMessengerManager::__SendLogin(const std::string& c_strAccount, const std::string& c_strCompanion)
{

// After
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

// Before
	if (c_lpChar->GetGMLevel() == GM_PLAYER && gm_get_level(c_strCompanion.c_str()) != GM_PLAYER)
		return;

	std::unique_ptr<TEMP_BUFFER> TempBuffer = std::make_unique<TEMP_BUFFER>();
	{
		TPacketGCMessengerList ListPacket = {};
		strlcpy(ListPacket.szName, c_strCompanion.c_str(), sizeof(ListPacket.szName));
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

// After
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

// Before
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
		TempBuffer->write(&ListPacket, sizeof(ListPacket));
	}

// After
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

// Before
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
		}

// After
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
