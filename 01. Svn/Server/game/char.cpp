// Find this line:
m_bWhisperCounter = 0;

// Add after it:
#if defined(__MESSENGER_RENEWAL__)
	m_bMessengerConnectionState = MESSENGER_CONNECTION_STATE_CONNECT;
	m_strStatusMessage.clear();
#endif

// Before
		else
			GetDesc()->SetPhase(PHASE_CLOSE);

		return;
	}

	sys_log(0, "WarpEnd %s %d %u %u", GetName(), m_lWarpMapIndex, m_posWarp.x, m_posWarp.y);

	Show(m_lWarpMapIndex, m_posWarp.x, m_posWarp.y, 0);
	Stop();

	m_lWarpMapIndex = 0;
	m_posWarp.x = m_posWarp.y = m_posWarp.z = 0;


	{

// After
		else
			GetDesc()->SetPhase(PHASE_CLOSE);

		return;
	}

	sys_log(0, "WarpEnd %s %d %u %u", GetName(), m_lWarpMapIndex, m_posWarp.x, m_posWarp.y);

	Show(m_lWarpMapIndex, m_posWarp.x, m_posWarp.y, 0);
	Stop();

	m_lWarpMapIndex = 0;
	m_posWarp.x = m_posWarp.y = m_posWarp.z = 0;

#if defined(__COMMUNITY_GUILD_RENEWAL__)
	if (CGuild* pGuild = GetGuild())
		pGuild->NotifyMemberLocation(GetPlayerID());
#endif

	{

// In `void CHARACTER::SetLeftSeat(bool bLeftSeat)`, find this block:
RestartLeftSeatWaitTimer();

// Add after it:
#if defined(__MESSENGER_RENEWAL__)
	if (bLeftSeat)
	{
		if (!m_bLeftSeat)
			CMessengerManager::instance().SetConnectionState(this, MESSENGER_CONNECTION_STATE_LEFT_SEAT);
	}
	else if (m_bLeftSeat && GetMessengerConnectionState() == MESSENGER_CONNECTION_STATE_LEFT_SEAT)
	{
		CMessengerManager::instance().SetConnectionState(this, MESSENGER_CONNECTION_STATE_CONNECT);
	}
#endif

// Add the following `CHARACTER::SetStatusMessage` function anywhere in this file:
#if defined(__MESSENGER_RENEWAL__)
void CHARACTER::SetStatusMessage(const char* c_szMessage)
{
	if (!c_szMessage)
	{
		m_strStatusMessage.clear();
		return;
	}

	size_t len = strnlen(c_szMessage, MESSENGER_STATUS_MESSAGE_MAX_LEN + 1);
	if (len > MESSENGER_STATUS_MESSAGE_MAX_LEN)
		len = MESSENGER_STATUS_MESSAGE_MAX_LEN;

	m_strStatusMessage.assign(c_szMessage, len);
}
#endif
