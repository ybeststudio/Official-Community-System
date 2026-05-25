// Add the following declaration/member block related section:
#if defined(ENABLE_MESSENGER_RENEWAL)
#	include "PythonCommunity.h"
#endif

// Find this line:
CPythonMessenger m_pyManager;

// Add after it:
#if defined(ENABLE_MESSENGER_RENEWAL)
	CPythonCommunity m_pyCommunity;
#endif
