#if defined(ENABLE_MESSENGER_RENEWAL)

class CPythonCommunity : public CSingleton<CPythonCommunity>
{
public:
	CPythonCommunity();
	virtual ~CPythonCommunity();

	void Destroy();

	void SetCommunityHandler(PyObject* poHandler);
	void SetMessengerHandler(PyObject* poHandler);
	void SetGuildHandler(PyObject* poHandler);
	void SetConfigHandler(PyObject* poHandler);

	void ClearCommunityHandler();
	void ClearMessengerHandler();
	void ClearGuildHandler();
	void ClearConfigHandler();

	PyObject* GetCommunityHandler() const { return m_poCommunityHandler; }
	PyObject* GetMessengerHandler() const { return m_poMessengerHandler; }
	bool HasMessengerHandler() const { return NULL != m_poMessengerHandler; }

	BYTE GetMainConnectionState() const { return m_bMainConnectionState; }
	void SetMainConnectionState(BYTE bState) { m_bMainConnectionState = bState; }

	// Friend list bridge (legacy messenger packets -> official uiCommunity API)
	void OnFriendOnline(const char* c_szName
#if defined(ENABLE_MESSENGER_DETAILS)
		, DWORD dwLastPlayTime = 0
#if defined(ENABLE_MULTI_LANGUAGE_SYSTEM)
		, const char* c_szCountry = NULL
#endif
#endif
	);
	void OnFriendOffline(const char* c_szName
#if defined(ENABLE_MESSENGER_DETAILS)
		, DWORD dwLastPlayTime = 0
#if defined(ENABLE_MULTI_LANGUAGE_SYSTEM)
		, const char* c_szCountry = NULL
#endif
#endif
	);
	void OnFriendRemove(const char* c_szName);

#if defined(ENABLE_MESSENGER_RENEWAL)
	void OnFriendRenewalDetails(const char* c_szName, BYTE bLevel, BYTE bIsConqueror, BYTE bChannel, long lMapIndex);
	void OnFriendConnectionState(const char* c_szName, BYTE bConnectionState);
	void OnFriendStatusMessage(const char* c_szName, const char* c_szMessage);
	void NotifyMyStatusMessage(const char* c_szMessage);
#if defined(ENABLE_COMMUNITY_GUILD_RENEWAL)
	void OnGuildMemberLocation(DWORD dwPID, BYTE bChannel, long lMapIndex);
#endif
#endif

#if defined(ENABLE_MESSENGER_BLOCK)
	void OnBlockOnline(const char* c_szName);
	void OnBlockOffline(const char* c_szName);
	void OnBlockRemove(const char* c_szName);
#endif

private:
	PyObject* m_poCommunityHandler;
	PyObject* m_poMessengerHandler;
	PyObject* m_poGuildHandler;
	PyObject* m_poConfigHandler;

	DWORD m_dwConfigFlag;
	BYTE m_bMainConnectionState;
};

#endif
