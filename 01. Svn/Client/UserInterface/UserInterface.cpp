// Find this line:
initMessenger();

// Add after it:
#if defined(ENABLE_MESSENGER_RENEWAL)
	initCommunity();
#endif
