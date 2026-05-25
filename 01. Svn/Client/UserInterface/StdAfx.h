// Find this line:
void initMessenger();

// Add after it:
#if defined(ENABLE_MESSENGER_RENEWAL)
void initCommunity();
#endif
