// Find this line:
BYTE GetWhisperCounter() const { return m_bWhisperCounter; }

// Add after it:
#if defined(__MESSENGER_RENEWAL__)
	void SetMessengerConnectionState(BYTE bState) { m_bMessengerConnectionState = bState; }
	BYTE GetMessengerConnectionState() const { return m_bMessengerConnectionState; }

	void SetStatusMessage(const char* c_szMessage);
	const char* GetStatusMessage() const { return m_strStatusMessage.c_str(); }
#endif

// Find this line:
BYTE m_bWhisperCounter;

// Add after it:
#if defined(__MESSENGER_RENEWAL__)
	BYTE m_bMessengerConnectionState;
	std::string m_strStatusMessage;
#endif
