
// Add to any location:

#if defined(ENABLE_MESSENGER_RENEWAL)
	PyModule_AddIntConstant(poModule, "ENABLE_MESSENGER_RENEWAL", 1);
	#if defined(ENABLE_COMMUNITY_GUILD_RENEWAL)
	PyModule_AddIntConstant(poModule, "ENABLE_COMMUNITY_GUILD_RENEWAL", 1);
	#else
	PyModule_AddIntConstant(poModule, "ENABLE_COMMUNITY_GUILD_RENEWAL", 0);
	#endif
#else
	PyModule_AddIntConstant(poModule, "ENABLE_MESSENGER_RENEWAL", 0);
#endif

