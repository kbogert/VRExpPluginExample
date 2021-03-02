// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
struct FBlueprintSessionResult;
class UObject;
class APlayerController;
struct FBPUniqueNetId;
class UFindFriendSessionCallbackProxy;
#ifdef ADVANCEDSESSIONS_FindFriendSessionCallbackProxy_generated_h
#error "FindFriendSessionCallbackProxy.generated.h already included, missing '#pragma once' in FindFriendSessionCallbackProxy.h"
#endif
#define ADVANCEDSESSIONS_FindFriendSessionCallbackProxy_generated_h

#define AdvancedSessions_HostProject_Plugins_AdvancedSessions_Source_AdvancedSessions_Classes_FindFriendSessionCallbackProxy_h_11_DELEGATE \
struct _Script_AdvancedSessions_eventBlueprintFindFriendSessionDelegate_Parms \
{ \
	TArray<FBlueprintSessionResult> SessionInfo; \
}; \
static inline void FBlueprintFindFriendSessionDelegate_DelegateWrapper(const FMulticastScriptDelegate& BlueprintFindFriendSessionDelegate, TArray<FBlueprintSessionResult> const& SessionInfo) \
{ \
	_Script_AdvancedSessions_eventBlueprintFindFriendSessionDelegate_Parms Parms; \
	Parms.SessionInfo=SessionInfo; \
	BlueprintFindFriendSessionDelegate.ProcessMulticastDelegate<UObject>(&Parms); \
}


#define AdvancedSessions_HostProject_Plugins_AdvancedSessions_Source_AdvancedSessions_Classes_FindFriendSessionCallbackProxy_h_16_SPARSE_DATA
#define AdvancedSessions_HostProject_Plugins_AdvancedSessions_Source_AdvancedSessions_Classes_FindFriendSessionCallbackProxy_h_16_RPC_WRAPPERS \
 \
	DECLARE_FUNCTION(execFindFriendSession);


#define AdvancedSessions_HostProject_Plugins_AdvancedSessions_Source_AdvancedSessions_Classes_FindFriendSessionCallbackProxy_h_16_RPC_WRAPPERS_NO_PURE_DECLS \
 \
	DECLARE_FUNCTION(execFindFriendSession);


#define AdvancedSessions_HostProject_Plugins_AdvancedSessions_Source_AdvancedSessions_Classes_FindFriendSessionCallbackProxy_h_16_INCLASS_NO_PURE_DECLS \
private: \
	static void StaticRegisterNativesUFindFriendSessionCallbackProxy(); \
	friend struct Z_Construct_UClass_UFindFriendSessionCallbackProxy_Statics; \
public: \
	DECLARE_CLASS(UFindFriendSessionCallbackProxy, UOnlineBlueprintCallProxyBase, COMPILED_IN_FLAGS(0), CASTCLASS_None, TEXT("/Script/AdvancedSessions"), ADVANCEDSESSIONS_API) \
	DECLARE_SERIALIZER(UFindFriendSessionCallbackProxy)


#define AdvancedSessions_HostProject_Plugins_AdvancedSessions_Source_AdvancedSessions_Classes_FindFriendSessionCallbackProxy_h_16_INCLASS \
private: \
	static void StaticRegisterNativesUFindFriendSessionCallbackProxy(); \
	friend struct Z_Construct_UClass_UFindFriendSessionCallbackProxy_Statics; \
public: \
	DECLARE_CLASS(UFindFriendSessionCallbackProxy, UOnlineBlueprintCallProxyBase, COMPILED_IN_FLAGS(0), CASTCLASS_None, TEXT("/Script/AdvancedSessions"), ADVANCEDSESSIONS_API) \
	DECLARE_SERIALIZER(UFindFriendSessionCallbackProxy)


#define AdvancedSessions_HostProject_Plugins_AdvancedSessions_Source_AdvancedSessions_Classes_FindFriendSessionCallbackProxy_h_16_STANDARD_CONSTRUCTORS \
	/** Standard constructor, called after all reflected properties have been initialized */ \
	ADVANCEDSESSIONS_API UFindFriendSessionCallbackProxy(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get()); \
	DEFINE_DEFAULT_OBJECT_INITIALIZER_CONSTRUCTOR_CALL(UFindFriendSessionCallbackProxy) \
	DECLARE_VTABLE_PTR_HELPER_CTOR(ADVANCEDSESSIONS_API, UFindFriendSessionCallbackProxy); \
DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(UFindFriendSessionCallbackProxy); \
private: \
	/** Private move- and copy-constructors, should never be used */ \
	ADVANCEDSESSIONS_API UFindFriendSessionCallbackProxy(UFindFriendSessionCallbackProxy&&); \
	ADVANCEDSESSIONS_API UFindFriendSessionCallbackProxy(const UFindFriendSessionCallbackProxy&); \
public:


#define AdvancedSessions_HostProject_Plugins_AdvancedSessions_Source_AdvancedSessions_Classes_FindFriendSessionCallbackProxy_h_16_ENHANCED_CONSTRUCTORS \
	/** Standard constructor, called after all reflected properties have been initialized */ \
	ADVANCEDSESSIONS_API UFindFriendSessionCallbackProxy(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get()) : Super(ObjectInitializer) { }; \
private: \
	/** Private move- and copy-constructors, should never be used */ \
	ADVANCEDSESSIONS_API UFindFriendSessionCallbackProxy(UFindFriendSessionCallbackProxy&&); \
	ADVANCEDSESSIONS_API UFindFriendSessionCallbackProxy(const UFindFriendSessionCallbackProxy&); \
public: \
	DECLARE_VTABLE_PTR_HELPER_CTOR(ADVANCEDSESSIONS_API, UFindFriendSessionCallbackProxy); \
DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(UFindFriendSessionCallbackProxy); \
	DEFINE_DEFAULT_OBJECT_INITIALIZER_CONSTRUCTOR_CALL(UFindFriendSessionCallbackProxy)


#define AdvancedSessions_HostProject_Plugins_AdvancedSessions_Source_AdvancedSessions_Classes_FindFriendSessionCallbackProxy_h_16_PRIVATE_PROPERTY_OFFSET
#define AdvancedSessions_HostProject_Plugins_AdvancedSessions_Source_AdvancedSessions_Classes_FindFriendSessionCallbackProxy_h_13_PROLOG
#define AdvancedSessions_HostProject_Plugins_AdvancedSessions_Source_AdvancedSessions_Classes_FindFriendSessionCallbackProxy_h_16_GENERATED_BODY_LEGACY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	AdvancedSessions_HostProject_Plugins_AdvancedSessions_Source_AdvancedSessions_Classes_FindFriendSessionCallbackProxy_h_16_PRIVATE_PROPERTY_OFFSET \
	AdvancedSessions_HostProject_Plugins_AdvancedSessions_Source_AdvancedSessions_Classes_FindFriendSessionCallbackProxy_h_16_SPARSE_DATA \
	AdvancedSessions_HostProject_Plugins_AdvancedSessions_Source_AdvancedSessions_Classes_FindFriendSessionCallbackProxy_h_16_RPC_WRAPPERS \
	AdvancedSessions_HostProject_Plugins_AdvancedSessions_Source_AdvancedSessions_Classes_FindFriendSessionCallbackProxy_h_16_INCLASS \
	AdvancedSessions_HostProject_Plugins_AdvancedSessions_Source_AdvancedSessions_Classes_FindFriendSessionCallbackProxy_h_16_STANDARD_CONSTRUCTORS \
public: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


#define AdvancedSessions_HostProject_Plugins_AdvancedSessions_Source_AdvancedSessions_Classes_FindFriendSessionCallbackProxy_h_16_GENERATED_BODY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	AdvancedSessions_HostProject_Plugins_AdvancedSessions_Source_AdvancedSessions_Classes_FindFriendSessionCallbackProxy_h_16_PRIVATE_PROPERTY_OFFSET \
	AdvancedSessions_HostProject_Plugins_AdvancedSessions_Source_AdvancedSessions_Classes_FindFriendSessionCallbackProxy_h_16_SPARSE_DATA \
	AdvancedSessions_HostProject_Plugins_AdvancedSessions_Source_AdvancedSessions_Classes_FindFriendSessionCallbackProxy_h_16_RPC_WRAPPERS_NO_PURE_DECLS \
	AdvancedSessions_HostProject_Plugins_AdvancedSessions_Source_AdvancedSessions_Classes_FindFriendSessionCallbackProxy_h_16_INCLASS_NO_PURE_DECLS \
	AdvancedSessions_HostProject_Plugins_AdvancedSessions_Source_AdvancedSessions_Classes_FindFriendSessionCallbackProxy_h_16_ENHANCED_CONSTRUCTORS \
static_assert(false, "Unknown access specifier for GENERATED_BODY() macro in class FindFriendSessionCallbackProxy."); \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


template<> ADVANCEDSESSIONS_API UClass* StaticClass<class UFindFriendSessionCallbackProxy>();

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID AdvancedSessions_HostProject_Plugins_AdvancedSessions_Source_AdvancedSessions_Classes_FindFriendSessionCallbackProxy_h


PRAGMA_ENABLE_DEPRECATION_WARNINGS
