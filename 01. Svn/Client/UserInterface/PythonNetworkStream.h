// Find this line:
bool SendMessengerRemovePacket(const char* c_szKey, const char* c_szName);

// Add after it:
#if defined(ENABLE_MESSENGER_RENEWAL)
	void RequestMessengerRefresh();
	bool SendMessengerSetConnectionStatePacket(BYTE bConnectionState);
	bool SendMessengerSetStatusMessagePacket(const char* c_szMessage);
	bool SendMessengerDeleteStatusMessagePacket();
#endif
