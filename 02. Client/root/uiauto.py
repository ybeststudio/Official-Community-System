# In `AutoOnOff`, extend the loop with:
				if app.ENABLE_MESSENGER_RENEWAL and self.__communityAutoHuntSyncHandler:
					self.__communityAutoHuntSyncHandler(onoff != 0)

# Add anywhere in the AutoWindow class:
	if app.ENABLE_MESSENGER_RENEWAL:
		def SetCommunityAutoHuntSyncHandler(self, handler):
			self.__communityAutoHuntSyncHandler = handler
