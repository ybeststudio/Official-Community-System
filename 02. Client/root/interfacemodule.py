# Add to the imports:
if app.ENABLE_MESSENGER_RENEWAL:
	import uiCommunity

# In `__init__`, extend the if-statement with:
		if app.ENABLE_MESSENGER_RENEWAL:
			self.wndCommunity = None

# In `__MakeMessengerWindow`, extend the if-statement with:
		if app.ENABLE_MESSENGER_RENEWAL:
			self.wndCommunity = uiCommunity.CommunityWindow()
			if app.ENABLE_CHATTING_WINDOW_RENEWAL:
				self.wndCommunity.BindInterface(self)
			self.wndCommunity.SetWhisperButtonEvent(lambda n, i=proxy(self): i.OpenWhisperDialog(n))
			self.wndCommunity.SetGuildButtonEvent(ui.__mem_func__(self.ToggleGuildWindow))

# In `GetMessengerHandler`, extend the if-statement with:
		if app.ENABLE_MESSENGER_RENEWAL:
			if self.wndCommunity:
				return self.wndCommunity

# In `GetCommunityWindow`, extend the if-statement with:
		if app.ENABLE_MESSENGER_RENEWAL:
			return self.wndCommunity

# In `__MakeWindows`, extend the if-statement with:
		if app.ENABLE_AUTO_SYSTEM:
			self.wndAutoWindow = uiAuto.AutoWindow()
			if app.ENABLE_MESSENGER_RENEWAL:
				self.wndAutoWindow.SetCommunityAutoHuntSyncHandler(ui.__mem_func__(self.SyncCommunityAutoHuntState))


# In `Close`, extend the if-statement with:
		if app.ENABLE_MESSENGER_RENEWAL:
			if self.wndCommunity:
				self.wndCommunity.Destroy()

# In `Close`, extend the loop with:
		if app.ENABLE_MESSENGER_RENEWAL:
			del self.wndCommunity

# In `RefreshAlignment`, extend the if-statement with:
		if app.ENABLE_MESSENGER_RENEWAL and self.wndCommunity:
			self.wndCommunity.RefreshAlignment()


# In `HideAllWindows`, extend the if-statement with:
		if app.ENABLE_MESSENGER_RENEWAL:
			if self.wndCommunity:
				self.wndCommunity.Hide()

# In `ToggleMessenger`, extend the if-statement with:
		if app.ENABLE_MESSENGER_RENEWAL:
			self.ToggleCommunity()
			return

# In `__HideWindows`, extend the if-statement with:
		if app.ENABLE_MESSENGER_RENEWAL:
			if self.wndCommunity:
				hideWindows += self.wndCommunity,

# Add anywhere in the Interface class:
	if app.ENABLE_MESSENGER_RENEWAL:
		def __ApplyCommunityConnectionState(self, connectionState, syncServer=False):
			import community
			if syncServer and hasattr(community, "ForceSendChangeConnectionState"):
				community.ForceSendChangeConnectionState(connectionState)
			elif hasattr(community, "SetMainCharacterConnectionState"):
				community.SetMainCharacterConnectionState(connectionState)
			if self.wndCommunity:
				self.wndCommunity.ChangeMyConnectionState(connectionState, True)

		def __GetCommunityRestoreConnectionState(self):
			import community
			import chr
			if app.ENABLE_LEFT_SEAT and getattr(self, "_communityAfkActive", False):
				return community.LEFT_SEAT
			if app.ENABLE_AUTO_SYSTEM and self.__IsCommunityAutoHuntActive():
				return community.AUTO_HUNT
			return community.CONNECT

		def __IsCommunityAutoHuntActive(self):
			if getattr(self, "_communityAutoHuntActive", False):
				return True
			if app.ENABLE_AUTO_SYSTEM:
				import chrmgr
				return bool(chrmgr.GetAutoOnOff())
			return False

		if app.ENABLE_LEFT_SEAT:
			def SyncCommunityAutoAfkState(self, isAfk):
				import community
				self._communityAfkActive = bool(isAfk)
				if isAfk:
					self.__ApplyCommunityConnectionState(community.LEFT_SEAT)
				else:
					self.__ApplyCommunityConnectionState(self.__GetCommunityRestoreConnectionState())

		if app.ENABLE_AUTO_SYSTEM:
			def SyncCommunityAutoHuntState(self, isAutoHunt):
				import community
				self._communityAutoHuntActive = bool(isAutoHunt)
				if isAutoHunt:
					if app.ENABLE_LEFT_SEAT and getattr(self, "_communityAfkActive", False):
						return
					self.__ApplyCommunityConnectionState(community.AUTO_HUNT, True)
				else:
					restoreState = community.CONNECT
					if app.ENABLE_LEFT_SEAT and getattr(self, "_communityAfkActive", False):
						restoreState = community.LEFT_SEAT
					self.__ApplyCommunityConnectionState(restoreState, True)
