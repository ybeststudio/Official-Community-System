// Add the following declaration/member block related section:
#if defined(__MESSENGER_RENEWAL__)
struct packet_messenger_list;
#endif

// Find this line:
bool IsFriend(const char* c_strAccount, const char* c_szName);

// Add after it:
#if defined(__MESSENGER_RENEWAL__)
	void SetConnectionState(LPCHARACTER c_lpChar, BYTE bConnectionState);
	void SetStatusMessage(LPCHARACTER c_lpChar, const char* c_szMessage);
	void DeleteStatusMessage(LPCHARACTER c_lpChar);
#endif

// Find this line:
void __SendLogout(const std::string& c_strAccount, const std::string& c_strCompanion);

// Add after it:
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

// Find this line:
LIST_ROW_COMPANION_LANGUAGE,

// Add after it:
#if defined(__MESSENGER_RENEWAL__)
		LIST_ROW_COMPANION_STATUS_MESSAGE,
#endif

// Find this line:
RelationMap m_map_strGMInverseRelation;

// Add after it:
#if defined(__MESSENGER_RENEWAL__)
	std::map<std::string, std::string> m_map_strStatusMessage;
#endif
