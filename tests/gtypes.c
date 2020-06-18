#define NO_SDK
#include <windows.h>
#include <stdlib.h>
#include <GMLuaModule.h>

ILuaModuleManager* mm = NULL;

extern "C" __declspec(dllexport) int gmod_open(ILuaInterface* I) {

    lua_State *L = (lua_State*)I->GetLuaState();
    mm = I->GetModuleManager();
    
    I->NewTable();
    
    ILuaObject* Result = I->GetObject(I->Top());
    
//  for (int t = GLua::TYPE_INVALID; t <= GLua::TYPE_COUNT; t++) {
    for (int t = GLua::TYPE_INVALID; t <= 34; t++) {
        I->PushLong((long)t);
        I->Push(I->GetTypeName(t));
        Result->SetMember(I->GetObject(-2), I->GetObject(-1));
    };
    
    I->SetGlobal("GTypesResult", Result);
    
    I->Push("Hello from gtypes!");
    
    return 1;
};