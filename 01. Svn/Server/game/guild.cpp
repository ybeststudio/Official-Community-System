// Before
	for (it = m_memberOnline.begin(); it != m_memberOnline.end(); ++it)
		SendLoginPacket(*it, pid);

}

void CGuild::LoginMember(LPCHARACTER ch)
{

// After
	for (it = m_memberOnline.begin(); it != m_memberOnline.end(); ++it)
		SendLoginPacket(*it, pid);

#if defined(__COMMUNITY_GUILD_RENEWAL__)
	NotifyMemberLocation(pid);
#endif
}

void CGuild::LoginMember(LPCHARACTER ch)
{

// Before
	for (it = m_memberOnline.begin(); it != m_memberOnline.end(); ++it)
		SendLoginPacket(*it, ch);

	m_memberOnline.insert(ch);

	SendAllGradePacket(ch);
	SendGuildInfoPacket(ch);
	SendListPacket(ch);
	SendSkillInfoPacket(ch);
	SendEnemyGuild(ch);

}

void CGuild::P2PLogoutMember(DWORD pid)
{

// After
	for (it = m_memberOnline.begin(); it != m_memberOnline.end(); ++it)
		SendLoginPacket(*it, ch);

	m_memberOnline.insert(ch);

	SendAllGradePacket(ch);
	SendGuildInfoPacket(ch);
	SendListPacket(ch);
	SendSkillInfoPacket(ch);
	SendEnemyGuild(ch);

#if defined(__COMMUNITY_GUILD_RENEWAL__)
	NotifyMemberLocation(ch->GetPlayerID());
	SyncAllMemberLocationsTo(ch);
#endif
}

void CGuild::P2PLogoutMember(DWORD pid)
{

// Add the following `CGuild::SendMemberLocationPacket` function anywhere in this file:
#if defined(__COMMUNITY_GUILD_RENEWAL__)
void CGuild::SendMemberLocationPacket(LPCHARACTER ch, DWORD pid, BYTE bChannel, long lMapIndex)
{
	if (!ch || !ch->GetDesc())
		return;

	if (bChannel == 0 && lMapIndex == 0)
		return;

	TPacketGCGuild pack = {};
	pack.header = HEADER_GC_GUILD;
	pack.subheader = GUILD_SUBHEADER_GC_MEMBER_LOCATION;
	pack.size = sizeof(pack) + sizeof(TPacketGCGuildMemberLocation);

	TPacketGCGuildMemberLocation loc = {};
	loc.dwPID = pid;
	loc.bChannel = bChannel;
	loc.lMapIndex = lMapIndex;

	TEMP_BUFFER buf;
	buf.write(&pack, sizeof(pack));
	buf.write(&loc, sizeof(loc));
	ch->GetDesc()->Packet(buf.read_peek(), buf.size());
}

void CGuild::NotifyMemberLocation(DWORD pid)
{
	BYTE bChannel = 0;
	long lMapIndex = 0;

	if (LPCHARACTER pkMember = CHARACTER_MANAGER::instance().FindByPID(pid))
	{
		bChannel = g_bChannel;
		lMapIndex = pkMember->GetMapIndex();
	}
	else if (m_memberP2POnline.find(pid) != m_memberP2POnline.end())
	{
		if (CCI* pcci = P2P_MANAGER::instance().FindByPID(pid))
			bChannel = pcci->bChannel;
	}
	else
	{
		return;
	}

	for (TGuildMemberOnlineContainer::iterator it = m_memberOnline.begin(); it != m_memberOnline.end(); ++it)
		SendMemberLocationPacket(*it, pid, bChannel, lMapIndex);
}

void CGuild::SyncAllMemberLocationsTo(LPCHARACTER ch)
{
	if (!ch)
		return;

	const DWORD dwSelfPID = ch->GetPlayerID();

	for (TGuildMemberOnlineContainer::iterator it = m_memberOnline.begin(); it != m_memberOnline.end(); ++it)
	{
		LPCHARACTER pkMember = *it;
		if (!pkMember || pkMember->GetPlayerID() == dwSelfPID)
			continue;

		SendMemberLocationPacket(ch, pkMember->GetPlayerID(), g_bChannel, pkMember->GetMapIndex());
	}

	for (TGuildMemberP2POnlineContainer::iterator it = m_memberP2POnline.begin(); it != m_memberP2POnline.end(); ++it)
	{
		const DWORD pid = *it;
		if (pid == dwSelfPID)
			continue;

		BYTE bChannel = 0;
		long lMapIndex = 0;

		if (LPCHARACTER pkMember = CHARACTER_MANAGER::instance().FindByPID(pid))
		{
			bChannel = g_bChannel;
			lMapIndex = pkMember->GetMapIndex();
		}
		else if (CCI* pcci = P2P_MANAGER::instance().FindByPID(pid))
		{
			bChannel = pcci->bChannel;
		}
		else
		{
			continue;
		}

		SendMemberLocationPacket(ch, pid, bChannel, lMapIndex);
	}
}
#endif
