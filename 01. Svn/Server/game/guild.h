// Find this line:
void SendLogoutPacket(LPCHARACTER ch, DWORD pid);

// Add after it:
#if defined(__COMMUNITY_GUILD_RENEWAL__)
	void SendMemberLocationPacket(LPCHARACTER ch, DWORD pid, BYTE bChannel, long lMapIndex);
	void NotifyMemberLocation(DWORD pid);
	void SyncAllMemberLocationsTo(LPCHARACTER ch);
#endif
