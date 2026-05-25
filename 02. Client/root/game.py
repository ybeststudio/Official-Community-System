# In `StartGame`, extend the if-statement with:
		if app.ENABLE_MESSENGER_RENEWAL and app.ENABLE_AUTO_SYSTEM and self.interface:
			self.interface.SyncCommunityAutoHuntState(chrmgr.GetAutoOnOff())


# In `BINARY_NEW_AddAffect`, extend the elif-statement with:
		elif app.ENABLE_AUTO_SYSTEM and app.ENABLE_MESSENGER_RENEWAL and chr.NEW_AFFECT_AUTO == type:
			if self.interface and chrmgr.GetAutoOnOff():
				self.interface.SyncCommunityAutoHuntState(True)


# In `BINARY_NEW_RemoveAffect`, extend the elif-statement with:
		elif app.ENABLE_AUTO_SYSTEM and app.ENABLE_MESSENGER_RENEWAL and chr.NEW_AFFECT_AUTO == type:
			if self.interface:
				self.interface.SyncCommunityAutoHuntState(False)


# In `OnMessengerAddFriendQuestion`, extend the if-statement with:
		if app.ENABLE_MESSENGER_RENEWAL:
			if self.interface:
				wndCommunity = self.interface.GetCommunityWindow()
				if wndCommunity:
					wndCommunity.AddFriendRequest(name, level, channel, mapIndex)
			return

# Add anywhere in the GameWindow class:
	if app.ENABLE_AUTO_SYSTEM:
		def __AutoOff(self):
			if self.interface:
				self.interface.AutoOff()
			chrmgr.SetAutoOnOff(False)
			if app.ENABLE_MESSENGER_RENEWAL and self.interface:
				self.interface.SyncCommunityAutoHuntState(False)
		def __AutoOn(self):
			if self.interface:
				self.interface.AutoOn()
			chrmgr.SetAutoOnOff(False)
			if app.ENABLE_MESSENGER_RENEWAL and self.interface:
				self.interface.SyncCommunityAutoHuntState(False)
		def __AutoLoginOff(self):
			chrmgr.SetAutoOnOff(False)
			if app.ENABLE_MESSENGER_RENEWAL and self.interface:
				self.interface.SyncCommunityAutoHuntState(False)


# In `OpenLeftSeatDialog`, extend the if-statement with:
			if app.ENABLE_MESSENGER_RENEWAL and self.interface:
				self.interface.SyncCommunityAutoAfkState(True)


# In `CloseLeftSeatDialog`, extend the if-statement with:
			if app.ENABLE_MESSENGER_RENEWAL and self.interface:
				self.interface.SyncCommunityAutoAfkState(False)


# In `__ReleaseLeftSeat`, extend the if-statement with:
			if app.ENABLE_MESSENGER_RENEWAL and self.interface:
				self.interface.SyncCommunityAutoAfkState(False)
