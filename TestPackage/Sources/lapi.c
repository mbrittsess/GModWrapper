#define NO_SDK
#include <windows.h>
#include <Strsafe.h>
#include <stdlib.h>
#include <stdarg.h>
#include <GMLuaModule.h>
#include <lua.hpp>
#include "gmwrapper.h"

/* IMPORTANT TO-DO: MAKE SURE ALL FUNCTIONS CORRECTLY THROW ERRORS WHEN THEY SHOULD */
/* MINOR TO-DO: MAKE SURE ALL FUNCTIONS ARE SWITCHED OVER TO USING IdxClassification() INSTEAD OF MANUAL CHECKS */

/* DLL Entrypoints, gmod_open() and gmod_close() */

extern "C" __declspec(dllexport) int gmod_open(ILuaInterface* I) {

    /* Phase one: perform native global setup. */
    lua_State* L = (lua_State*)I->GetLuaState();
    mm = I->GetModuleManager();
    ThisHeap = GetProcessHeap();
    lua_CFunction LuaOpenFunc = NULL;
    const char* FinalRequireName = NULL;
    OrigRequireName = lua_tostring(L, 1);
    
    /* Phase two: perform Lua global setup. */
    CompileAndPush(L, LuaCombinedSrc, "luacombined.lua");
    if (lua_type(L, -1) != LUA_TFUNCTION) {
        luaL_error(L, "Error initializing Vanillin in library '%s': %s", OrigRequireName, lua_tostring(L, -1));
    };
    lua_pushvalue(L, 1);
    pUR(I->GetMetaTable("fulluserdata", udMetaID));
    pUR(I->GetMetaTable("fulluserdataanchor", udAnchorMetaID));
    lua_call(L, 3, 2);
    lua_pushcfunction(L, UserdataGC);
    lua_call(L, 1, 0);
    
    /* The first return value from the last call was the name of the function we need to find. */
    FinalRequireName = lua_tostring(L, -1); lua_pop(L, 1);
    
    /* Phase three: get a handle to this DLL's module. */
   {HMODULE OurModuleHandle;
    
    if (!GetModuleHandleExW(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
        (LPCWSTR)gmod_open, /* Why yes, GetModuleHandleEx *does* produce one of the strangest typecasts in existence. */
        &OurModuleHandle)) {
        luaL_error(L, "Error initializing Vanillin in library '%s': failed to get handle to own module", OrigRequireName);
    };
    
    /* Phase four: get the address of our luaopen_X() function. */
    LuaOpenFunc = (lua_CFunction)GetProcAddress(OurModuleHandle, FinalRequireName);
    if (LuaOpenFunc == NULL) {
        luaL_error(L, "Error initializing Vanillin in library '%s': could not find function %s()", OrigRequireName, FinalRequireName);
    };
   };
   
    /* Now we'll restore the stack to the way it should be before jumping off to the appropriate function. */
    lua_settop(L, 1);
    int modretvals = LuaOpenFunc(L);
   {int top = lua_gettop(L);
    
    /* Phase five: ensure any returned values actually get returned. 
        Garry's require() doesn't work correctly for binary modules. It gets as far as correctly returning whatever value lies
    in package.loaded[modname], but messes up the part where any return values from the module get placed into package.loaded[modname]
    and then returned. Instead, the return values from the loader are basically ignored, but thankfully, if a value happens to
    already be in package.loaded[modname], Garry won't assign boolean 'true' to that space, and will return that value. Ergo,
    to correctly return values, we just have to place them ourself into package.loaded[modname]. */
    if (modretvals != 0) {
        int raidx = lua_absindex(L, -1); /* We're only concerned with the first return value. */
        lua_getglobal(L, "package");
        lua_getfield(L, -1, "loaded");
        lua_pushvalue(L, raidx);
        lua_setfield(L, -2, OrigRequireName);
        lua_settop(L, top);
    };};
    
    return modretvals;
};

extern "C" __declspec(dllexport) int gmod_close(ILuaInterface* I) {
    return 0;
};

/* Implementation of the Lua C API, the meat-and-potatoes of this library. */

/* WRAPPROGRESS
** FUNCTION: lua_newstate
** STATUS: COMPLETE
*/
LUA_API lua_State *(lua_newstate) (lua_Alloc f, void *ud) {
    /* REVIEW DECISION:
       Options:
        1. Return NULL, catch it at IModuleManager::GetLuaInterface(lua_State*) --> ILuaInterface*
        2. Return NULL, call MessageBoxA() and then maybe exit the program?
        3. Return NULL, every single other function must then check for the validity of the lua_State*
       It *is* a valid behavior for lua_newstate() to return NULL; this signals an out-of-memory
    condition.
    */
    return NULL;
};

/* WRAPPROGRESS
** FUNCTION: lua_close
** STATUS: COMPLETE
*/
LUA_API void       (lua_close) (lua_State *L) {
    luaL_error(L, VanillinCannotImplementError, OrigRequireName, __FUNCTION__);
    return;
};

/* WRAPPROGRESS
** FUNCTION: lua_newthread
** STATUS: COMPLETE
*/
LUA_API lua_State *(lua_newthread) (lua_State *L) {
    luaL_error(L, VanillinCannotImplementError, OrigRequireName, __FUNCTION__);
    return NULL;
};

/* WRAPPROGRESS
** FUNCTION: lua_atpanic
** STATUS: COMPLETE
*/
LUA_API lua_CFunction (lua_atpanic) (lua_State *L, lua_CFunction panicf) {
    luaL_error(L, VanillinCannotImplementError, OrigRequireName, __FUNCTION__);
    return NULL;
};

/* WRAPPROGRESS
** FUNCTION: lua_gettop
** STATUS: COMPLETE
*/
LUA_API int   (lua_gettop) (lua_State *L) {
    Prep;
    
    return I->GetStackTop();
};

/* WRAPPROGRESS
** FUNCTION: lua_settop
** STATUS: COMPLETE
*/
#if 0
LUA_API void  (lua_settop) (lua_State *L, int idx) {
    Prep;
    
    int StackTop = I->GetStackTop();
    if (idx == 0) {
        I->Pop(StackTop);
        return;
    } else if (!IsValidIdx(L, idx) && (idx < 0)) {
        return I->Error(MustBeActualStackIdxError);
    } else {
        int NewTop = (idx < 0) ? (StackTop + (idx+1)) : idx;
        if (NewTop < StackTop) {
            I->Pop(StackTop - NewTop);
            return;
        } else if (NewTop > StackTop) {
            for (int i = 0; i < (NewTop - StackTop); i++) {
                I->PushNil();
            };
        };
        
        return;
    };
};
#elif 0
LUA_API void  (lua_settop) (lua_State *L, int idx) {
    Prep;
    
    int StackTop = I->GetStackTop();
    int NewTop;
    switch (IdxClassification(L, idx, &NewTop, NULL)) {
        case VanIdx_OnStack :
        case VanIdx_AboveStack :
            if (NewTop < StackTop) {
                I->Pop(StackTop - NewTop);
                return;
            } else if (NewTop > StackTop) {
                for (int i = 0; i < (NewTop - StackTop); i++) {
                    I->PushNil();
                };
            };
        
            return;
        default :
            if (idx == 0) {
                I->Pop(StackTop);
                return;
            } else {
                return I->Error(MustBeActualStackIdxError);
            };
    };
};
#else
LUA_API void  (lua_settop) (lua_State *L, int idx) {
    Prep;
    
    int StackTop = I->GetStackTop();
    int NewTop;
    switch (IdxClassification(L, idx, &NewTop, NULL)) {
        case VanIdx_OnStack    :
        case VanIdx_AboveStack :
            if (NewTop < StackTop) {
                I->Pop(StackTop - NewTop);
                return;
            } else if (NewTop > StackTop) {
                for (int i = 0; i < (NewTop - StackTop); i++) {
                    I->PushNil();
                };
            };
            
            return;
        default :
            if (idx == 0) {
                I->Pop(StackTop);
                return;
            } else {
                BadIdxError(L, __FUNCTION__, idx);
                return;
            };
    };
};
#endif

/* WRAPPROGRESS
** FUNCTION: lua_pushvalue
** STATUS: COMPLETE
*/
#if 0
LUA_API void  (lua_pushvalue) (lua_State *L, int idx) {
    pUR(ObjFromIdx(L, idx));
    
    return;
};
#else
LUA_API void  (lua_pushvalue) (lua_State *L, int idx) {
    if (IdxClassification(L, idx, NULL, NULL) == VanIdx_Nonsensical) {
        BadIdxError(L, __FUNCTION__, idx);
    } else {
        pUR(ObjFromIdx(L, idx));
    };
    
    return;
};
#endif

/* WRAPPROGRESS
** FUNCTION: lua_remove
** STATUS: COMPLETE
*/
#if 0
LUA_API void  (lua_remove) (lua_State *L, int idx) {
    Prep;
    
    int aidx;
    int top = I->GetStackTop();
    switch (IdxClassification(L, idx, &aidx, NULL)) {
        case VanIdx_OnStack :
            if (aidx == top) {
                I->Pop(1);
                return;
            } else {
                VanillinSavedVals* vals = SaveStackVals(L, (top - aidx));
                I->Pop(1);
                RestoreStackVals(L, vals);
                return;
            };
        default :
            I->Error("lua_remove() can only be called with valid (on-stack) indices!");
            return;
    };
};
#else
LUA_API void  (lua_remove) (lua_State *L, int idx) {
    Prep;
    
    int aidx;
    int top = I->GetStackTop();
    switch (IdxClassification(L, idx, &aidx, NULL)) {
        case VanIdx_OnStack :
            if (aidx == top) {
                I->Pop(1);
                return;
            } else {
                VanillinSavedVals* vals = SaveStackVals(L, (top - aidx));
                I->Pop(1);
                RestoreStackVals(L, vals);
                return;
            };
        default :
            BadIdxError(L, __FUNCTION__, idx);
            return;
    };
};
#endif

/* WRAPPROGRESS
** FUNCTION: lua_insert
** STATUS: COMPLETE
*/
#if 0
LUA_API void  (lua_insert) (lua_State *L, int idx) {
    Prep;
    
    int aidx;
    int top = I->GetStackTop();
    switch (IdxClassification(L, idx, &aidx, NULL)) {
        case VanIdx_OnStack :
            if (aidx == top) {
                return;
            } else {
                ILuaObject* val = I->GetObject(-1); I->Pop(1);
                VanillinSavedVals* vals = SaveStackVals(L, (top - aidx ));
                pUR(val);
                RestoreStackVals(L, vals);
                return;
            };
        default :
            I->Error("lua_insert() can only be called with valid (on-stack) indices!");
            return;
    };
};
#else
LUA_API void  (lua_insert) (lua_State *L, int idx) {
    Prep;
    
    int aidx;
    int top = I->GetStackTop();
    switch (IdxClassification(L, idx, &aidx, NULL)) {
        case VanIdx_OnStack :
            if (aidx == top) {
                return;
            } else {
                ILuaObject* val = I->GetObject(-1); I->Pop(1);
                VanillinSavedVals* vals = SaveStackVals(L, (top - aidx ));
                pUR(val);
                RestoreStackVals(L, vals);
                return;
            };
        default :
            BadIdxError(L, __FUNCTION__, idx);
            return;
    };
};
#endif

/* WRAPPROGRESS
** FUNCTION: lua_replace
** STATUS: COMPLETE
*/
#if 0
LUA_API void  (lua_replace) (lua_State *L, int idx) {
    Prep;
    
    int aidx;
    int top = I->GetStackTop();
    if (top == 0) {
        I->Error("lua_replace() cannot be called without at least one value on the stack!");
        return;
    } else {
        switch (IdxClassification(L, idx, &aidx, NULL)) {
            case VanIdx_OnStack :
                if (aidx != top) {
                    ILuaObject* src = I->GetObject(-1); I->Pop(1);
                    VanillinSavedVals* vals = SaveStackVals(L, (top - 1) - aidx);
                    I->Pop(1);
                    pUR(src);
                    RestoreStackVals(L, vals);
                };
                
                return;
            case VanIdx_Pseudo_Tbl :
                switch (aidx) {
                    case LUA_REGISTRYINDEX :
                        I->Error("Vanillin lacks the capability to replace the Registry Table!");
                        return;
                    case LUA_ENVIRONINDEX :
                        pUR(iwStringUR(I->GetGlobal("debug"), "setfenv"));
                        pUR(GetSelf(L));
                        pUR(I->GetObject(-3));
                        CallFunction(L, 2, 0);
                        I->Pop(1);
                        return;
                    case LUA_GLOBALSINDEX :
                        I->Error("Vanillin lacks the capability to replace the Globals Table!");
                        return;
                    default :
                        I->Error("Impossible condition in function lua_replace()");
                        return ;
                };
            case VanIdx_Pseudo_Upv :
                {ILuaObject* UpvTbl = iwObjURBoth(iwStringUR(I->GetObject(LUA_REGISTRYINDEX), guidUpvalues), GetSelf(L));
                if (UpvTbl->isNil()) {
                    UpvTbl->UnReference();
                    I->Error("Tried to set an upvalue on a function that doesn't appear to have any!");
                    return;
                } else {
                    int ordinal = UpvalueIdxToOrdinal(L, idx);
                    if (ordinal > rIntUR(UpvTbl->GetMember("n"))) {
                        UpvTbl->UnReference();
                        I->Error("Tried to set a non-existent upvalue on a function!");
                        return;
                    } else {
                        ILuaObject* val = I->GetObject(-1); I->Pop(1);
                        UpvTbl->SetMember((float)ordinal, val);
                        val->UnReference();
                        UpvTbl->UnReference();
                        return;
                    };
                };};
            default :
                I->Error(IllegalIdxError);
                return;
        };
    };
};
#else
LUA_API void  (lua_replace) (lua_State *L, int idx) {
    Prep;
    
    int aidx;
    int top = I->GetStackTop();
    if (top == 0) {
        luaL_error(L, NotEnoughStackSpace, OrigRequireName, __FUNCTION__, 1, top);
        return;
    } else {
        switch (IdxClassification(L, idx, &aidx, NULL)) {
            case VanIdx_OnStack :
                if (aidx != top) {
                    ILuaObject* src = I->GetObject(-1); I->Pop(1);
                    VanillinSavedVals* vals = SaveStackVals(L, (top - 1) - aidx);
                    I->Pop(1);
                    pUR(src);
                    RestoreStackVals(L, vals);
                };
                
                return;
            case VanIdx_Pseudo_Tbl :
               {const char* idxname;
                switch (idx) {
                    case LUA_REGISTRYINDEX :
                        idxname = "LUA_REGISTRYINDEX"; break;
                    case LUA_ENVIRONINDEX  :
                        idxname = "LUA_ENVIRONINDEX";  break;
                    case LUA_GLOBALSINDEX  :
                        idxname = "LUA_GLOBALSINDEX";  break;};
                luaL_error(L, "In library '%s': called function %s(), Vanillin cannot use it with %s",
                    OrigRequireName, __FUNCTION__, idxname);};
            case VanIdx_Pseudo_Upv :
               {ILuaObject* UpvTbl = iwObjURBoth(iwStringUR(I->GetObject(LUA_REGISTRYINDEX), guidUpvalues), GetSelf(L));
                int ordinal = UpvalueIdxToOrdinal(L, idx);
                if (UpvTbl->isNil()) {
                    UpvTbl->UnReference();
                    luaL_error(L, "In library '%s': called function %s(), attempted to set upvalue #%d in a function without any",
                        OrigRequireName, __FUNCTION__, ordinal);
                    return;
                } else {
                    int nupvalues = rIntUR(UpvTbl->GetMember("n"));
                    if (ordinal > nupvalues) {
                        UpvTbl->UnReference();
                        luaL_error(L, "In library '%s': called function %s(), attempted to set upvalue #%d, function only has %d upvalues",
                            OrigRequireName, __FUNCTION__, ordinal, nupvalues);
                        return;
                    } else {
                        ILuaObject* val = I->GetObject(-1); I->Pop(1);
                        UpvTbl->SetMember((float)ordinal, val);
                        val->UnReference();
                        UpvTbl->UnReference();
                        return;
                    };
                };};
            default :
                BadIdxError(L, __FUNCTION__, idx);
                return;
        };
    };
};
#endif

/* WRAPPROGRESS
** FUNCTION: lua_checkstack
** STATUS: COMPLETE
*/
LUA_API int   (lua_checkstack) (lua_State *L, int sz) {
    if (sz >= 0) {
        return TRUE;
    } else {
        luaL_error(L, "In library '%s': called function %s(), asked if %d extra stack spaces available, that makes no sense",
            OrigRequireName, __FUNCTION__, sz);
        return FALSE;
    };
};

/* WRAPPROGRESS
** FUNCTION: lua_xmove
** STATUS: COMPLETE
*/
LUA_API void  (lua_xmove) (lua_State *from, lua_State *to, int n) {
    luaL_error(from, VanillinCannotImplementError, OrigRequireName, __FUNCTION__);
    return;
};

/* WRAPPROGRESS
** FUNCTION: lua_isnumber
** STATUS: COMPLETE
*/
#if 0
LUA_API int             (lua_isnumber) (lua_State *L, int idx) {
    Prep;
    
    if (I->GetType(idx) == GLua::TYPE_NUMBER) {
        return 1;
    } else {
        ILuaObject* arg = I->GetObject(idx);
        pUR(I->GetGlobal("tonumber"));
        pUR(arg);
        CallFunction(L, 1, 1);
        int retval = (I->GetType(-1) == GLua::TYPE_NUMBER) ? 1 : 0;
        I->Pop(1);
        
        return retval;
    };
};
#else
LUA_API int             (lua_isnumber) (lua_State *L, int idx) {
    Prep;
    
    switch (IdxClassification(L, idx, NULL, NULL)) {
        case VanIdx_Nonsensical :
            BadIdxError(L, __FUNCTION__, idx);
            return 0;
        default :
            pUR(ObjFromIdx(L, idx));
            int type = I->GetType(-1);
            if (type == GLua::TYPE_NUMBER) {
                I->Pop(1);
                return 1;
            } else if (type == GLua::TYPE_STRING) {
                pUR(I->GetGlobal("tonumber"));
                pUR(I->GetObject(-2));
                CallFunction(L, 1, 1);
                int retval = (I->GetType(-1) == GLua::TYPE_NUMBER) ? 1 : 0;
                I->Pop(2);
                
                return retval;
            } else {
                I->Pop(1);
                return 0;
            };
    };
};
#endif

/* WRAPPROGRESS
** FUNCTION: lua_isstring
** STATUS: COMPLETE
*/
#if 0
LUA_API int             (lua_isstring) (lua_State *L, int idx) {
    Prep;
    
    int ObjType = rTypeUR(ObjFromIdx(L, idx));
    
    return ((ObjType == GLua::TYPE_STRING) || (ObjType == GLua::TYPE_NUMBER)) ? TRUE : FALSE;
};
#else
LUA_API int             (lua_isstring) (lua_State *L, int idx) {
    Prep;
    
    if (IdxClassification(L, idx, NULL, NULL) == VanIdx_Nonsensical) {
        BadIdxError(L, __FUNCTION__, idx);
        return 0;
    } else {
        int type = rTypeUR(ObjFromIdx(L, idx));
        return ((type == GLua::TYPE_STRING) || (type == GLua::TYPE_NUMBER)) ? TRUE : FALSE;
    };
};
#endif

/* WRAPPROGRESS
** FUNCTION: lua_iscfunction
** STATUS: COMPLETE
*/
LUA_API int             (lua_iscfunction) (lua_State *L, int idx) {
    Prep;
    
    ILuaObject* Obj = ObjFromIdx(L, idx);
    if (Obj->GetType() == GLua::TYPE_FUNCTION) {
        pUR(iwStringUR(I->GetGlobal("debug"), "getinfo"));
        pUR(Obj);
        CallFunction(L, 1, 1);
        
        int ret = rStringUR(iwStringUR(I->GetObject(-1), "what"))[0] == 'C'; /* "what" can only be "Lua", "C", "main", "tail", or "" */
        I->Pop(1);
        
        return ret;
    } else {
        return FALSE;
    };
};

/* WRAPPROGRESS
** FUNCTION: lua_isuserdata
** STATUS: COMPLETE
*/
LUA_API int             (lua_isuserdata) (lua_State *L, int idx) {
    int type = lua_type(L, idx);
    return ((type == LUA_TUSERDATA) || (type == LUA_TLIGHTUSERDATA));
};

/* WRAPPROGRESS
** FUNCTION: lua_type
** STATUS: COMPLETE
*/
#if 0 /* Old version */
LUA_API int             (lua_type) (lua_State *L, int idx) {
    Prep;
    
    return I->GetObject(LUA_REGISTRYINDEX)->GetMember(guidTypeMapInt)->GetMember(I->GetTypeName(ObjFromIdx(L, idx)->GetType()))->GetInt();
};
#elif 0 /* Reference-counted version */
LUA_API int             (lua_type) (lua_State *L, int idx) {
    Prep;
    
    return rIntUR(iwStringUR(iwStringUR(I->GetObject(LUA_REGISTRYINDEX), guidTypeMapInt), I->GetTypeName(rTypeUR(ObjFromIdx(L, idx)))));
};
#else
LUA_API int             (lua_type) (lua_State *L, int idx) {
    Prep;
    
    /* Quick hack to make lua_isnone() work */
    if (lua_absindex(L, idx) > lua_gettop(L)) {
        return LUA_TNONE;
    };
    
    switch (I->GetType(idx)) {
        case GLua::TYPE_INVALID :
            return LUA_TNONE;
        case GLua::TYPE_NIL :
            return LUA_TNIL;
        case GLua::TYPE_STRING :
            return LUA_TSTRING;
        case GLua::TYPE_NUMBER :
            return LUA_TNUMBER;
        case GLua::TYPE_TABLE :
            return LUA_TTABLE;
        case GLua::TYPE_BOOL :
            return LUA_TBOOLEAN;
        case GLua::TYPE_FUNCTION :
            return LUA_TFUNCTION;
        case GLua::TYPE_THREAD :
            return LUA_TTHREAD;
        case GLua::TYPE_LIGHTUSERDATA :
            return LUA_TLIGHTUSERDATA;
        case udMetaID :
            return LUA_TUSERDATA;
        default:
            return LUA_TTABLE;
    };
};
#endif

/* WRAPPROGRESS
** FUNCTION: lua_typename
** STATUS: COMPLETE
*/
LUA_API const char     *(lua_typename) (lua_State *L, int tp) {
    static const char* TypeNames[] = {
        "nil",
        "boolean",
        "userdata",
        "number",
        "string",
        "table",
        "function",
        "userdata",
        "thread"
    };
    
    return ((tp <= LUA_TTHREAD) && (tp >= LUA_TNIL)) ? TypeNames[tp] : "unknown";
};

/* WRAPPROGRESS
** FUNCTION: lua_equal
** STATUS: COMPLETE
*/
LUA_API int            (lua_equal) (lua_State *L, int idx1, int idx2) {
    Prep;

    ILuaObject* obj1 = I->GetObject(idx1);
    ILuaObject* obj2 = I->GetObject(idx1);
    int retval = (int)I->IsEqual(obj1, obj2);
    obj1->UnReference();
    obj2->UnReference();
    
    return retval;
};

/* WRAPPROGRESS
** FUNCTION: lua_rawequal
** STATUS: COMPLETE
*/
LUA_API int            (lua_rawequal) (lua_State *L, int idx1, int idx2) {
    int aidx1 = lua_absindex(L, idx1);
    int aidx2 = lua_absindex(L, idx2);
    lua_getglobal(L, "rawequal");
    lua_pushvalue(L, aidx1);
    lua_pushvalue(L, aidx2);
    lua_call(L, 2, 1);
    int retval = lua_toboolean(L, -1);
    lua_pop(L, 1);
    
    return retval;
};

/* WRAPPROGRESS
** FUNCTION: lua_lessthan
** STATUS: COMPLETE
*/
LUA_API int            (lua_lessthan) (lua_State *L, int idx1, int idx2) {
    int aidx1 = lua_absindex(L, idx1);
    int aidx2 = lua_absindex(L, idx2);
    
    lua_getfield(L, LUA_REGISTRYINDEX, guidLessThanFunc);
    lua_pushvalue(L, aidx1);
    lua_pushvalue(L, aidx2);
    lua_call(L, 2, 1);
    
    int retval = lua_toboolean(L, -1);
    lua_pop(L, 1);
    
    return retval;
};

/* WRAPPROGRESS
** FUNCTION: lua_tonumber
** STATUS: COMPLETE
*/
LUA_API lua_Number      (lua_tonumber) (lua_State *L, int idx) {
    Prep;
    
    pUR(ObjFromIdx(L, idx));
    lua_Number ret = (lua_Number) I->GetDouble(-1);
    I->Pop(1);
    
    return ret;
};

/* WRAPPROGRESS
** FUNCTION: lua_tointeger
** STATUS: COMPLETE
    The stack-access methods will correctly convert strings into integers, but the direct object-access
methods fail to implement this behavior.
*/
LUA_API lua_Integer     (lua_tointeger) (lua_State *L, int idx) {
    Prep;
    
    pUR(ObjFromIdx(L, idx));
    lua_Integer ret = (lua_Integer) I->GetInteger(-1);
    I->Pop(1);
    
    return ret;
};

/* WRAPPROGRESS
** FUNCTION: lua_toboolean
** STATUS: COMPLETE
    The stack-access methods for getting booleans correctly follow the Lua rules of "false and nil are false,
everything else is true". The object-access methods for getting booleans, however, returns false for all non-boolean
values.
*/
LUA_API int             (lua_toboolean) (lua_State *L, int idx) {
    Prep;
    
    pUR(ObjFromIdx(L, idx));
    int ret = (int) I->GetBool(-1);
    I->Pop(1);
    
    return ret;
};

/* WRAPPROGRESS
** FUNCTION: lua_tolstring
** STATUS: COMPLETE
*/
LUA_API const char     *(lua_tolstring) (lua_State *L, int idx, size_t *len) {
    Prep;
    
    const char* ret = NULL;
    pUR(ObjFromIdx(L, idx));
    int ObjType = I->GetType(-1);
    if ((ObjType == GLua::TYPE_STRING) || (ObjType == GLua::TYPE_NUMBER)) {
        ret = I->GetString(-1);
        if (len != NULL) {
            *len = (size_t) I->StringLength(-1);
        };
    };
    
    I->Pop(1);
    return ret;
};

/* WRAPPROGRESS
** FUNCTION: lua_objlen
** STATUS: COMPLETE
*/
LUA_API size_t          (lua_objlen) (lua_State *L, int idx) {
    Prep;
    
    int aidx = lua_absindex(L, idx);
    
    switch (lua_type(L, idx)) {
        case LUA_TSTRING :
            return I->StringLength(idx);
        case LUA_TUSERDATA :
            return rIntUR(iwObjURBoth(iwStringUR(I->GetObject(LUA_REGISTRYINDEX), guidUDSizes), I->GetObject(idx)));
        case LUA_TTABLE :
           {lua_getfield(L, LUA_REGISTRYINDEX, guidLenFunc);
            lua_pushvalue(L, aidx);
            lua_call(L, 1, 1);
            int retval = (int)lua_tointeger(L, -1);
            lua_pop(L, 1);
            return retval;};
        default :
            return 0;
    };
};

/* WRAPPROGRESS
** FUNCTION: lua_tocfunction
** STATUS: COMPLETE
*/
LUA_API lua_CFunction   (lua_tocfunction) (lua_State *L, int idx) {
    /* Hm, one problem here is that we can only convert C functions created by Vanillin to pointers, but lua_iscfunction() can correctly
    report if *any* function is a C function. We'll just have to risk it, and if the users complain, we'll see what we can do. */
    lua_CFunction retval = NULL;
    
    lua_getfield(L, LUA_REGISTRYINDEX, guidCFuncPointers);
    pUR(ObjFromIdx(L, idx));
    lua_gettable(L, -2);
    
    if (lua_islightuserdata(L, -1)) {
        retval = (lua_CFunction)lua_touserdata(L, -1);
    };
    
    lua_pop(L, 2);
    return retval;
};

/* WRAPPROGRESS
** FUNCTION: lua_touserdata
** STATUS: COMPLETE
*/
#if 0
LUA_API void	       *(lua_touserdata) (lua_State *L, int idx) {
    Prep;
    
    return I->GetUserData(idx);
};
#else
LUA_API void	       *(lua_touserdata) (lua_State *L, int idx) {
    Prep;
    
    void* ret = NULL;
    ILuaObject* obj = ObjFromIdx(L, idx);
    if (obj->GetType() == GLua::TYPE_LIGHTUSERDATA) {
        pUR(obj);
        ret = I->GetLightUserData(-1);
        I->Pop(1);
    } else if (obj->isUserData()) {
        pUR(obj);
        ret = I->GetUserData(-1);
        I->Pop(1);
    } else {
        obj->UnReference();
        ret = NULL;
    };
    
    return ret;
};
#endif

/* WRAPPROGRESS
** FUNCTION: lua_tothread
** STATUS: COMPLETE
*/
LUA_API lua_State      *(lua_tothread) (lua_State *L, int idx) {
    Prep;
    
    I->Error("Vanillin lacks the capability to retrieve states from threads-on-the-stack!");
    return NULL;
};

/* WRAPPROGRESS
** FUNCTION: lua_topointer
** STATUS: COMPLETE
    This function is...rather incorrect in its functioning. o__o
*/
LUA_API const void     *(lua_topointer) (lua_State *L, int idx) {
    Prep;
    
    ILuaObject* obj = ObjFromIdx(L, idx);
    
    switch (obj->GetType()) {
        case GLua::TYPE_NIL    :
        case GLua::TYPE_NUMBER :
        case GLua::TYPE_BOOL   :
            obj->UnReference();
            return NULL;
        case GLua::TYPE_STRING :
            return (void*)rStringUR(obj);
        default :
            break;
    };
    
    void* retval = obj->GetUserData();
    if (retval != NULL) {
        obj->UnReference();
        return retval;
    } else {
        lua_getfield(L, LUA_REGISTRYINDEX, guidToPtrFunc);
        pUR(obj);
        lua_call(L, 1, 1);
        retval = (void*)I->GetInteger(-1);
        I->Pop(1);
        return retval;
    };
};

/* WRAPPROGRESS
** FUNCTION: lua_pushnil
** STATUS: COMPLETE
*/
LUA_API void  (lua_pushnil) (lua_State *L) {
    Prep;
    
    I->PushNil();
    
    return;
};

/* WRAPPROGRESS
** FUNCTION: lua_pushnumber
** STATUS: COMPLETE
*/
LUA_API void  (lua_pushnumber) (lua_State *L, lua_Number n) {
    Prep;
    
    I->PushDouble((double) n);
    
    return;
};

/* WRAPPROGRESS
** FUNCTION: lua_pushinteger
** STATUS: COMPLETE
*/
LUA_API void  (lua_pushinteger) (lua_State *L, lua_Integer n) {
    Prep;
    
    I->PushLong((long) n);
    
    return;
};

/* WRAPPROGRESS
** FUNCTION: lua_pushlstring
** STATUS: COMPLETE
    Damn, this is a hard one. GVomit offers us no way to work with explicit-length strings, so we must
implement support for them ourselves in an extraordinarily hacky way. There are possibly several ways
to implement this function more efficiently, but I'm using as simple a one as I can for the moment.
*/
LUA_API void  (lua_pushlstring) (lua_State *L, const char *s, size_t len) {
    Prep;

    /* First we'll create space for two strings (plus space for the nul terminator), both equal in length to the given one. */
    LPSTR InitialCopy = (LPSTR)HeapAlloc(ThisHeap, 0, len+1);/* Normally I use HEAP_NO_SERIALIZE but GMod is a multithreaded environment. */
    LPSTR DiffCopy    = (LPSTR)HeapAlloc(ThisHeap, 0, len+1);
    
    /* The first string will receive a copy of the original string. */
    CopyMemory(InitialCopy, s, len);
    /* Then we'll ensure that both are NUL-terminated... */
    InitialCopy[len] = '\x00';
    DiffCopy[len]    = '\x00';
    
    /*  Now, in order to put our string into GMod at its full length, we have to change every ASCII NUL into something else,
    anything else, it doesn't matter. In this case, let's just pick ASCII SOH, as it's the next value up. We will do this to
    our InitialCopy string, not the original, for obvious reasons.
        As we do this, we will also fill in the corresponding values of the DiffCopy string. Any non-NUL will be filled with 
    (and these are also arbitrarily-chosen) an ASCII 0 character, and any NUL characters will be filled with an ASCII 1 character. */
    {int i;
     for (i = 0; i < len; i++) {
        if (InitialCopy[i] == '\x00') {
            InitialCopy[i] = '\x01';
            DiffCopy[i]    =   '1';
        } else {
            DiffCopy[i]    =   '0';
        };
    };};
    
    /* Now that this is done, we will push a Lua function, created by this library and stored in the Lua registry, onto the stack,
    followed by both strings, and call it. This function re-creates the original string and leaves that on the stack, completing
    our job. */
    pUR(iwStringUR(I->GetObject(LUA_REGISTRYINDEX), guidLPLS_AnonFunc));
    I->Push(InitialCopy);
    I->Push(DiffCopy);
    CallFunction(L, 2, 1);
    
    /* Finally, we free the memory we used originally. */
    HeapFree(ThisHeap, 0, InitialCopy);
    HeapFree(ThisHeap, 0, DiffCopy);
    
    return;
};

/* WRAPPROGRESS
** FUNCTION: lua_pushstring
** STATUS: COMPLETE
*/
LUA_API void  (lua_pushstring) (lua_State *L, const char *s) {
    Prep;
    
    I->Push(s);
    
    return;
};

/* WRAPPROGRESS
** FUNCTION: lua_pushvfstring
** STATUS: COMPLETE
*/
LUA_API const char *(lua_pushvfstring) (lua_State *L, const char *fmt,
                                                      va_list argp) {
    int CountPushed = 0;
    int FmtOfs = 0;
    
    lua_getfield(L, LUA_REGISTRYINDEX, guidPushfString);
    
    while (1) {
        if (fmt[FmtOfs] == '\0') {
            /* Time to finish this. */
            break;
        } else if (fmt[FmtOfs] == '%') {
            /* Escape sequence processing. */
            FmtOfs += 1;
            switch (fmt[FmtOfs]) {
                case '%' :
                    lua_pushstring(L, "%");
                    break;
                case 's' :
                    lua_pushstring(L, va_arg(argp, const char*));
                    break;
                case 'f' :
                    lua_pushnumber(L, va_arg(argp, lua_Number));
                    break;
                case 'p' :
                   {char buf[4*sizeof(void *) + 8];
                    StringCbPrintfA(buf, sizeof(buf), "%p", va_arg(argp, void*));
                    lua_pushstring(L, (const char*)buf);
                    break;};
                case 'd' :
                    lua_pushboolean(L, FALSE);
                    lua_pushinteger(L, va_arg(argp, lua_Integer));
                    CountPushed += 1;
                    break;
                case 'c' :
                   {char to_be_pushed = (char)va_arg(argp, int);
                    lua_pushlstring(L, &to_be_pushed, 1);
                    break;};
                case '\0' :
                    goto force_break; /* Trailing %'s on the end of the string should do nothing. */
                    break;
                default :
                    lua_pushlstring(L, &fmt[FmtOfs], 1);
                    break;
            };
            FmtOfs += 1;
            CountPushed += 1;
            continue;
            
           force_break:
            break;
        } else {
            /* Literal insertion. */
            int LiteralStart = FmtOfs;
            char ThisChar = fmt[FmtOfs];
            while ((ThisChar != '%') && (ThisChar != '\0')) {
                FmtOfs += 1;
                ThisChar = fmt[FmtOfs];
            };
            lua_pushlstring(L, &fmt[LiteralStart], FmtOfs - LiteralStart);
            CountPushed += 1;
        };
    };
    
    lua_call(L, CountPushed, 1);
    
    return lua_tostring(L, -1);
};

/* WRAPPROGRESS
** FUNCTION: lua_pushfstring
** STATUS: COMPLETE
*/
LUA_API const char *(lua_pushfstring) (lua_State *L, const char *fmt, ...) {
    va_list argp;
    va_start(argp, fmt);
    const char* retval = lua_pushvfstring(L, fmt, argp);
    va_end(argp);
    return retval;
};

/* WRAPPROGRESS
** FUNCTION: lua_pushcclosure
** STATUS: COMPLETE
*/
LUA_API void  (lua_pushcclosure) (lua_State *L, lua_CFunction fn, int n) {
    Prep;
    
    if (n > 255) {
        I->Error(TooManyUpvaluesError);
        
        return;
    } else if (n < 0) {
        I->Error(NegativeUpvaluesSpecifiedError);
        
        return;
    } else if (n > I->GetStackTop()) {
        I->Error(MoreUpvaluesThanStackSpacesError);
        
        return;
    } else if (n != 0) {
        ILuaObject* upvTbl = I->GetNewTable();
        
        upvTbl->SetMember("n", (float)n);
        
        int StackIdx = -n;
        int UpvIdx   = 1;
        while (StackIdx <= -1) {
            ILuaObject* val = I->GetObject(StackIdx);
            upvTbl->SetMember((float)UpvIdx, val);
            val->UnReference();
            StackIdx += 1;
            UpvIdx   += 1;
        };
        
        I->Pop(n);
        I->Push(fn);
        ILuaObject* NewFunc = I->GetObject(-1);
        ILuaObject* upvTblCollection = iwStringUR(I->GetObject(LUA_REGISTRYINDEX), guidUpvalues);
        upvTblCollection->SetMember(NewFunc, upvTbl);
        
        upvTblCollection->UnReference();
        NewFunc->UnReference();
        upvTbl->UnReference();
    } else {
        I->Push(fn);
    };
    
    lua_getfield(L, LUA_REGISTRYINDEX, guidCFuncPointers);
    lua_pushvalue(L, -2);
    lua_pushlightuserdata(L, (void*)fn);
    lua_settable(L, -3);
    lua_pop(L, 1);
    
    return;
};

/* WRAPPROGRESS
** FUNCTION: lua_pushboolean
** STATUS: COMPLETE
*/
LUA_API void  (lua_pushboolean) (lua_State *L, int b) {
    Prep;
    
    I->Push((bool)b);
    
    return;
};

/* WRAPPROGRESS
** FUNCTION: lua_pushlightuserdata
** STATUS: COMPLETE
*/
LUA_API void  (lua_pushlightuserdata) (lua_State *L, void *p) {
    Prep;
    
    I->PushLightUserData(p);
    
    return;
};

/* WRAPPROGRESS
** FUNCTION: lua_pushthread
** STATUS: COMPLETE
*/
LUA_API int   (lua_pushthread) (lua_State *L) {
    Prep;
    
    I->Error("Vanillin lacks the capability to push threads-on-the-stack from states!");
    return 0;
};

/* WRAPPROGRESS
** FUNCTION: lua_gettable
** STATUS: COMPLETE
*/
#if 0
LUA_API void  (lua_gettable) (lua_State *L, int idx) {
    Prep;
    
    ILuaObject* objTbl = ObjFromIdx(L, idx);
    ILuaObject* objKey = I->GetObject(-1);
    I->Pop(1);
    pUR(iwObjURBoth(objTbl, objKey));
    
    return;
};
#else
LUA_API void  (lua_gettable) (lua_State *L, int idx) {
    Prep;
    
    pUR(ObjFromIdx(L, idx));
    
    
    return;
};
#endif

/* WRAPPROGRESS
** FUNCTION: lua_getfield
** STATUS: COMPLETE
*/
#if 0
LUA_API void  (lua_getfield) (lua_State *L, int idx, const char *k) {
    pUR(iwStringUR(ObjFromIdx(L, idx), k));
    
    return;
};
#else
LUA_API void  (lua_getfield) (lua_State *L, int idx, const char *k) {
    Prep;
    
    pUR(ObjFromIdx(L, idx));
    I->Push(k);
    I->GetTable(-2);
    ILuaObject* value = I->GetObject(-1);
    I->Pop(2);
    I->Push(value);
    value->UnReference();
    
    return;
};
#endif

/* WRAPPROGRESS
** FUNCTION: lua_rawget
** STATUS: COMPLETE
*/
LUA_API void  (lua_rawget) (lua_State *L, int idx) {
    Prep;
    
    ILuaObject* tblObj = ObjFromIdx(L, idx);
    ILuaObject* keyObj = I->GetObject(-1); I->Pop(1);
    pUR(I->GetGlobal("rawget"));
    pUR(tblObj);
    pUR(keyObj);
    CallFunction(L, 2, 1);
    
    return;
};

/* WRAPPROGRESS
** FUNCTION: lua_rawgeti
** STATUS: COMPLETE
*/
LUA_API void  (lua_rawgeti) (lua_State *L, int idx, int n) {
    Prep;
    
    ILuaObject* tblObj = ObjFromIdx(L, idx);
    pUR(I->GetGlobal("rawget"));
    pUR(tblObj);
    I->PushLong((long)n);
    CallFunction(L, 2, 1);
    
    return;
};

/* WRAPPROGRESS
** FUNCTION: lua_createtable
** STATUS: COMPLETE
    It should be noted, that lua_createtable()'s second and third arguments are merely recommendations. 
*/
LUA_API void  (lua_createtable) (lua_State *L, int narr, int nrec) {
    Prep;
    
    I->NewTable();
    
    return;
};

/* WRAPPROGRESS
** FUNCTION: lua_newuserdata
** STATUS: COMPLETE
*/
LUA_API void *(lua_newuserdata) (lua_State *L, size_t sz) {
    Prep;
    
    ILuaObject* udMeta = I->GetMetaTable("fulluserdata", udMetaID);
    ILuaObject* udAnchMeta = I->GetMetaTable("fulluserdataanchor", udAnchorMetaID);
    void* MemoryBlock = HeapAlloc(ThisHeap, HEAP_ZERO_MEMORY, sz);
    if (MemoryBlock == NULL) {
        I->Error("memory allocation error: not enough memory, or default process heap is corrupt");
        return NULL;
    };
    pUR(iwStringUR(I->GetObject(LUA_REGISTRYINDEX), guidUDPrepFunc));
    I->PushUserData(udMeta, MemoryBlock);
    I->PushUserData(udAnchMeta, MemoryBlock);
    I->PushLong((long)sz);
    I->PushValue(LUA_ENVIRONINDEX);
    lua_call(L, 4, 1);
    
    return MemoryBlock;
};

/* WRAPPROGRESS
** FUNCTION: lua_getmetatable
** STATUS: COMPLETE
*/
#if 0
LUA_API int   (lua_getmetatable) (lua_State *L, int objindex) {
    Prep;
    
    ILuaObject* obj = ObjFromIdx(L, objindex);
    if (obj->GetType() == udMetaID) { /* Still has default metatable */
        obj->UnReference();
        return 0; /* Then we will claim that it has no metatable. */
    } else {
        lua_getglobal(L, "debug");
        lua_getfield(L, -1, "getmetatable");
        pUR(obj);
        lua_call(L, 1, 1);
        if (lua_isnil(L, -1)) {
            lua_pop(L, 2);
            return 0;
        } else {
            lua_remove(L, -2);
            return 1;
        };
    };
};
#else
LUA_API int   (lua_getmetatable) (lua_State *L, int objindex) {
    Prep;
    
    int aidx = lua_absindex(L, objindex);
    
    lua_getglobal(L, "debug");
    lua_getfield(L, -1, "getmetatable");
    pUR(ObjFromIdx(L, aidx));
    lua_call(L, 1, 1);
    if (lua_isnil(L, -1)) {
        lua_pop(L, 2);
        return 0;
    } else {
        pUR(I->GetMetaTable("fulluserdata", udMetaID));
        if (lua_rawequal(L, -1, -2)) { /* In this case, it's got the default full userdata MT, which we treat as not actually being an MT */
            lua_pop(L, 3);
            return 0;
        } else {
            lua_remove(L, -3);
            lua_pop(L, 1);
            return 1;
        };
    };
};
#endif

/* WRAPPROGRESS
** FUNCTION: lua_getfenv
** STATUS: COMPLETE
*/
LUA_API void  (lua_getfenv) (lua_State *L, int idx) {
    Prep;
    
    ILuaObject* obj = ObjFromIdx(L, idx);
    if (obj->GetType() == udMetaID) {
        pUR(iwObjURBoth(iwStringUR(I->GetObject(LUA_REGISTRYINDEX), guidUDEnvs), obj));
        return;
    } else {
        pUR(iwStringUR(I->GetGlobal("debug"), "getfenv"));
        pUR(obj);
        CallFunction(L, 1, 1);
        return;
    };
};

/* WRAPPROGRESS
** FUNCTION: lua_settable
** STATUS: COMPLETE
*/
LUA_API void  (lua_settable) (lua_State *L, int idx) {
    Prep;
    
    ILuaObject* tObj = ObjFromIdx(L, idx);
    ILuaObject* kObj = I->GetObject(-2);
    ILuaObject* vObj = I->GetObject(-1);
    
    tObj->SetMember(kObj, vObj);
    
    tObj->UnReference();
    kObj->UnReference();
    vObj->UnReference();
    I->Pop(2);
    
    return;
};

/* WRAPPROGRESS
** FUNCTION: lua_setfield
** STATUS: COMPLETE
*/
LUA_API void  (lua_setfield) (lua_State *L, int idx, const char *k) {
    Prep;
    
    ILuaObject* tObj = ObjFromIdx(L, idx);
    ILuaObject* vObj = I->GetObject(-1);
    
    tObj->SetMember(k, vObj);
    
    tObj->UnReference();
    vObj->UnReference();
    I->Pop(1);
    
    return;
};

/* WRAPPROGRESS
** FUNCTION: lua_rawset
** STATUS: COMPLETE
*/
LUA_API void  (lua_rawset) (lua_State *L, int idx) {
    Prep;
    
    ILuaObject* tblObj = I->GetObject(idx);
    ILuaObject* keyObj = I->GetObject(-2);
    ILuaObject* valObj = I->GetObject(-1);
    I->Pop(2);
    pUR(I->GetGlobal("rawset"));
    pUR(tblObj);
    pUR(keyObj);
    pUR(valObj);
    CallFunction(L, 3, 0);
    
    return;
};

/* WRAPPROGRESS
** FUNCTION: lua_rawseti
** STATUS: COMPLETE
*/
LUA_API void  (lua_rawseti) (lua_State *L, int idx, int n) {
    Prep;
    
    ILuaObject* tblObj = I->GetObject(idx);
    ILuaObject* valObj = I->GetObject(-1);
    I->Pop(1);
    pUR(I->GetGlobal("rawset"));
    pUR(tblObj);
    I->PushLong((long)n);
    pUR(valObj);
    CallFunction(L, 3, 0);
    
    return;
};

/* WRAPPROGRESS
** FUNCTION: lua_setmetatable
** STATUS: COMPLETE
*/
LUA_API int   (lua_setmetatable) (lua_State *L, int objindex) {
    Prep;
    
    int objaidx = lua_absindex(L, objindex);
    int tblaidx = lua_absindex(L, -1);
    if (I->GetType(objaidx) == udMetaID) {
        lua_pushstring(L, "MetaID");
        lua_pushinteger(L, udMetaID);
        lua_rawset(L, tblaidx);
        
        lua_pushstring(L, "MetaName");
        lua_pushstring(L, "userdata"); /* Need to make this a global in a bit... */
        lua_rawset(L, tblaidx);
    };
    lua_getglobal(L, "debug");
    lua_getfield(L, -1, "setmetatable");
    lua_remove(L, -2);
    lua_pushvalue(L, objaidx);
    lua_pushvalue(L, tblaidx);
    lua_call(L, 2, 0);
    lua_pop(L, 1);
    
    return 1;
};

/* WRAPPROGRESS
** FUNCTION: lua_setfenv
** STATUS: COMPLETE
*/
LUA_API int   (lua_setfenv) (lua_State *L, int idx) {
    Prep;
    
    ILuaObject* obj = ObjFromIdx(L, idx);
    ILuaObject* newEnv = I->GetObject(-1); I->Pop(1);
    switch (obj->GetType()) {
        case udMetaID:
           {ILuaObject* udEnvs = iwStringUR(I->GetObject(LUA_REGISTRYINDEX), guidUDEnvs);
            udEnvs->SetMember(obj, newEnv);
            obj->UnReference();
            udEnvs->UnReference();
            newEnv->UnReference();
            return 1;};
        case GLua::TYPE_FUNCTION :
        case GLua::TYPE_THREAD :
            pUR(iwStringUR(I->GetGlobal("debug"), "setfenv"));
            pUR(obj);
            pUR(newEnv);
            CallFunction(L, 2, 0);
            return 1;
        default :
            obj->UnReference();
            newEnv->UnReference();
            return 0;
    };
};

/* WRAPPROGRESS
** FUNCTION: lua_call
** STATUS: COMPLETE
*/
LUA_API void  (lua_call) (lua_State *L, int nargs, int nresults) {
    Prep;
    
    CallFunction(L, nargs, nresults);
    
    return;
};

/* WRAPPROGRESS
** FUNCTION: lua_pcall
** STATUS: COMPLETE
*/
LUA_API int   (lua_pcall) (lua_State *L, int nargs, int nresults, int errfunc) {
    Prep;

    int erraidx = (errfunc != 0) ? lua_absindex(L, errfunc) : 0;
    int baseaidx = lua_absindex(L, -(nargs+1));
    lua_getfield(L, LUA_REGISTRYINDEX, guidLPCall);
    lua_insert(L, baseaidx);
    lua_pushvalue(L, erraidx);
    lua_insert(L, baseaidx+1);
    lua_call(L, nargs+2, (nresults == LUA_MULTRET) ? LUA_MULTRET : nresults + 1);
    int status = lua_tointeger(L, baseaidx);
    lua_remove(L, baseaidx);
    switch (status) {
        case 1 : /* Everything went fine. */
            return 0;
        case 2 : /* Function errored, no handler */
            return LUA_ERRRUN;
        case 3 : /* Function errored, handler ran successfully */
            return LUA_ERRRUN;
        case 4 : /* Function errored, handler errored */
            return LUA_ERRERR;
        default :
            I->Error("Impossible condition in lua_pcall()!");
            return -1;
    };
};

/* WRAPPROGRESS
** FUNCTION: lua_cpcall
** STATUS: COMPLETE
*/
LUA_API int   (lua_cpcall) (lua_State *L, lua_CFunction func, void *ud) {
    lua_getglobal(L, "pcall");
    lua_pushcfunction(L, func);
    lua_pushlightuserdata(L, ud);
    lua_call(L, 2, 2);
    if (!lua_toboolean(L, -2)) {
        lua_remove(L, -2);
        return LUA_ERRRUN;
    } else {
        lua_pop(L, 2);
        return 0;
    };
};

/* WRAPPROGRESS
** FUNCTION: lua_load
** STATUS: COMPLETE
*/
LUA_API int   (lua_load) (lua_State *L, lua_Reader reader, void *dt,
                                        const char *chunkname) {
    lua_getglobal(L, "CompileString");
    lua_pushliteral(L, "");
    
    size_t length = 0;
    int npieces = 1;
    while (1) {
        const char* piece = reader(L, dt, &length);
        if ((piece != NULL) && (length != 0)) {
            lua_pushlstring(L, piece, length);
            npieces += 1;
        } else {
            break;
        };
    };
    lua_concat(L, npieces);
    lua_pushstring(L, chunkname);
    lua_pushboolean(L, TRUE);
    lua_call(L, 3, 1);
    
    return (lua_isstring(L, -1)) ? LUA_ERRSYNTAX : 0;
};

/* WRAPPROGRESS
** FUNCTION: lua_dump
** STATUS: COMPLETE
*/
LUA_API int (lua_dump) (lua_State *L, lua_Writer writer, void *data) {
    lua_getglobal(L, "string");
    lua_getfield(L, -1, "dump");
    lua_remove(L, -2);
    lua_pushvalue(L, -2);
    lua_call(L, 1, 1);
    
    size_t length;
    const char* piece = lua_tolstring(L, -1, &length);
    int retval = writer(L, piece, length, data);
    lua_pop(L, 1);
    
    return retval;
};

/* WRAPPROGRESS
** FUNCTION: lua_yield
** STATUS: COMPLETE
*/
LUA_API int  (lua_yield) (lua_State *L, int nresults) {
    Prep;
    
    I->Error("Vanillin lacks the capability to yield its own thread!");
    return 0;
};

/* WRAPPROGRESS
** FUNCTION: lua_resume
** STATUS: COMPLETE
*/
LUA_API int  (lua_resume) (lua_State *L, int narg) {
    Prep;
    
    I->Error("Vanillin lacks the capability to start threads!");
    return 0;
};

/* WRAPPROGRESS
** FUNCTION: lua_status
** STATUS: COMPLETE
*/
LUA_API int  (lua_status) (lua_State *L) {
    Prep;
    
    I->Error("Vanillin lacks the capability to report thread status!");
    return 0;
};

/* WRAPPROGRESS
** FUNCTION: lua_gc
** STATUS: INCOMPLETE
*/
LUA_API int (lua_gc) (lua_State *L, int what, int data) {
    /* Use the _G.collectgarbage() function */
    return 0;
};

/* WRAPPROGRESS
** FUNCTION: lua_error
** STATUS: COMPLETE
*/
LUA_API int   (lua_error) (lua_State *L) {
    lua_getglobal(L, "error");
    lua_pushvalue(L, -2);
    lua_pushinteger(L, 0);
    lua_call(L, 2, 0);
    
    return 0; /* Useless value... */
};

/* WRAPPROGRESS
** FUNCTION: lua_next
** STATUS: COMPLETE
*/
LUA_API int   (lua_next) (lua_State *L, int idx) {
    int aidx = lua_absindex(L, idx);
    lua_getglobal(L, "next");
    lua_pushvalue(L, aidx);
    lua_pushvalue(L, -3);
    lua_remove(L, -4);
    lua_call(L, 2, 2);
    if (lua_isnil(L, -2)) {
        lua_pop(L, 2);
        return 0;
    } else {
        return 1;
    };
};

/* WRAPPROGRESS
** FUNCTION: lua_concat
** STATUS: COMPLETE
*/
LUA_API void  (lua_concat) (lua_State *L, int n) {
    Prep; /* This is only here so we can error, try to fix this somehow. */

    if (n == 1) {
        return;
    } else if (n == 0) {
        lua_pushstring(L, "");
        return;
    } else if (n > 1) {
        lua_getfield(L, LUA_REGISTRYINDEX, guidConcatFunc);
        lua_insert(L, -(n+1));
        lua_call(L, n, 1);
        return;
    } else {
        I->Error("A negative number was passed to lua_concat(), that makes no sense!");
        return;
    };
};

/* WRAPPROGRESS
** FUNCTION: lua_getallocf
** STATUS: COMPLETE
*/
LUA_API lua_Alloc (lua_getallocf) (lua_State *L, void **ud) {
    Prep;
    
    I->Error("Vanillin lacks the capability to retrieve a state's memory-allocation function!");
    return NULL;
};

/* WRAPPROGRESS
** FUNCTION: lua_setallocf
** STATUS: COMPLETE
*/
LUA_API void lua_setallocf (lua_State *L, lua_Alloc f, void *ud) {
    Prep;
    
    I->Error("Vanillin lacks the capability to set a state's memory-allocation function!");
    return;
};

/* WRAPPROGRESS
** FUNCTION: lua_getstack
** STATUS: INCOMPLETE 100% --Needs testing
*/
/*  The design of this function and lua_getinfo() is a bit complex...I'm documenting it here so that I
don't forget it later, and so that others can figure it out after me.
    lua_getinfo() is used to retrieve info about functions, and it roughly corresponds to debug.getinfo().
Like debug.getinfo(), you can explicitly pass it a function to get info about, or a number representing
the execution level.
    In order to get info about a *specific* function, you only have to call lua_getinfo(). You push the
function you want info about onto the stack and make sure your 'where' string starts with '>', then it
fills in the lua_Debug structure you passed.
    But in order to get info about a particular stack level, you have to create a lua_Debug, pass it to
lua_getstack() and indicate which execution level you want info on, which will fill it with a bit of info,
and *then* you pass that to lua_getinfo().
    The problem is that the lua_Debug structure can technically be passed around anywhere and you can get
info from it, even if you've changed the execution level at some point. Because of upvalues, you can't
just stick the function into the lua_Debug structure itself, so here's the solution I've come up with.
    lua_Debug has two fields that aren't used by users, and they're both ints: lua_Debug.event and
lua_Debug.i_ci.
    I want to be able to get info on the stored execution level anywhere, but I don't want to cause the
functions to be anchored into existence when nobody is using them. So here's the solution I came up with;
it doesn't allow you to get info on an execution stack level that doesn't exist anymore, but that is fine
with me.
    There's (yet another) registry table, under guidLGSRefs, with weak values. We use luaL_ref to store
the value into the table and get a unique integer key for it, and store that key into lua_Debug.i_ci, which
we can use to retrieve the function later. */
LUA_API int lua_getstack (lua_State *L, int level, lua_Debug *ar) {
    int orig_top = lua_gettop(L);
    
    lua_getglobal(L, "print");
    lua_pushliteral(L, "lua_getstack() is being called");
    lua_call(L, 1, 0);
    
    lua_getglobal(L, "debug");
    lua_getfield(L, -1, "getinfo");
    lua_remove(L, -2);
    lua_pushinteger(L, level+1);
    lua_pushliteral(L, "f");
    lua_call(L, 2, 1);
    
    if (lua_isnil(L, -1)) {
        lua_settop(L, orig_top);
        
        return 0;
    } else {
        lua_getfield(L, LUA_REGISTRYINDEX, guidLGSRefs);
        lua_getfield(L, -2, "func");
        ar->i_ci = luaL_ref(L, -2);
        lua_settop(L, orig_top);
        
        return 1;
    };
};

/* WRAPPROGRESS
** FUNCTION: lua_getinfo
** STATUS: INCOMPLETE 100%
*/
LUA_API int lua_getinfo (lua_State *L, const char *what, lua_Debug *ar) {
    int status = 1;
    BOOL found_L = FALSE;
    BOOL found_f = FALSE;
    
    /* Initialize ar since we seem to be having so many problems */
    ar->name = "not_initialized_by_lua_getinfo";
    ar->namewhat = "not_initialized_by_lua_getinfo";
    ar->what = "not_initialized_by_lua_getinfo";
    ar->source = "not_initialized_by_lua_getinfo";
    ar->currentline = 0;
    ar->nups = 0;
    ar->linedefined = 0;
    ar->lastlinedefined = 0;
    ar->short_src[0] = 0;
    
    lua_getglobal(L, "debug");
    lua_getfield(L, -1, "getinfo");
    lua_remove(L, -2);
    
    if (what[0] == '>') { /* Swap debug.getinfo and the function under it */
        lua_pushvalue(L, -2);
        lua_remove(L, -3);
        what += 1;
    } else { /* Push _R[guidLGSRefs][ar->i_ci] */
        lua_getfield(L, LUA_REGISTRYINDEX, guidLGSRefs);
        lua_rawgeti(L, -1, ar->i_ci);
        lua_remove(L, -2);
    };
    
    lua_pushliteral(L, "nSlufL");
    lua_call(L, 2, 1);
    
    int tbl_idx = lua_absindex(L, -1);
    for (; *what; what++) {
        switch(*what) {
            case 'n' :
                lua_getfield(L, tbl_idx, "name");
                ar->name = (lua_isnil(L, -1)) ? NULL : lua_tostring(L, -1);
                lua_getfield(L, tbl_idx, "namewhat");
                ar->namewhat = lua_tostring(L, -1);
                break;
            case 'S' :
                lua_getfield(L, tbl_idx, "source");
                ar->source = lua_tostring(L, -1);
                lua_getfield(L, tbl_idx, "short_src");
                lstrcpynA(ar->short_src, lua_tostring(L, -1), LUA_IDSIZE);
                lua_getfield(L, tbl_idx, "linedefined");
                ar->linedefined = lua_tointeger(L, -1);
                lua_getfield(L, tbl_idx, "lastlinedefined");
                ar->lastlinedefined = lua_tointeger(L, -1);
                lua_getfield(L, tbl_idx, "what");
                ar->what = lua_tostring(L, -1);
                break;
            case 'l' :
                lua_getfield(L, tbl_idx, "currentline");
                ar->currentline = lua_tointeger(L, -1);
                break;
            case 'u' :
                /* Let's see if this is a Vanillin C function. */
                lua_getfield(L, LUA_REGISTRYINDEX, guidUpvalues);
                lua_getfield(L, tbl_idx, "func");
                lua_gettable(L, -2);
                if (lua_isnil(L, -1)) { /* Not a Vanillin function */
                    lua_getfield(L, tbl_idx, "nups");
                    ar->nups = lua_tointeger(L, -1);
                } else {
                    lua_getfield(L, -1, "n");
                    ar->nups = lua_tointeger(L, -1);
                };
                break;
            case 'L' :
                found_L = TRUE;
                break;
            case 'f' :
                found_f = TRUE;
                break;
            default :
                status = 0;
                break;
        };
    };
    
    lua_settop(L, tbl_idx);
    
    if (found_f) {
        lua_getfield(L, tbl_idx, "func");
    };
    if (found_L) {
        lua_getfield(L, tbl_idx, "activelines");
    };
    
    lua_remove(L, tbl_idx);
    
    return status;
};

/* Various utility functions used throughout. */

static void CompileAndPush(lua_State *L, const char* src, const char* name) {
    lua_getglobal(L, "CompileString");
    lua_pushstring(L, src);
    lua_pushstring(L, name);
    lua_pushboolean(L, FALSE);
    lua_call(L, 3, 1);
    
    return;
};

/* Pass it the idx you want to classify, optionally a pointer to an int which will receive the "absolute" index, and optionally
a pointer to a structure (whose layout and meaning have not been made yet) which will give some instructions on actions to
automatically take as regards error messages for various classes of index. */
static enum VanillinIdxClass IdxClassification(lua_State *L, int idx, int* aidx, void* StructPtr) {
    /* Going to be using that fourth parameter for a structure to automate error-handling in a bit. */
    Prep;
    
    enum VanillinIdxClass retval;
    int raidx = idx;
    int top = I->GetStackTop();
    
    if (idx > 0) { /* Absolute index */
        raidx = idx;
        retval = (idx > top) ? VanIdx_AboveStack : VanIdx_OnStack;
    } else if ((idx <= LUA_REGISTRYINDEX) && (idx >= lua_upvalueindex(255))) { /* Pseudoindex */
        raidx = idx;
        retval = (idx <= lua_upvalueindex(1)) ? VanIdx_Pseudo_Upv : VanIdx_Pseudo_Tbl;
    } else if ((idx < 0) && (idx >= -top)) { /* Valid relative index */
        raidx = top + idx + 1;
        retval = VanIdx_OnStack;
    } else { /* 0, or invalid relative index */
        raidx = 0;
        retval = VanIdx_Nonsensical;
    };
    
    if (aidx != NULL) {
        *aidx = raidx;
    };
    
    return retval;
};

/* BEGIN AREA OF NOTICE
    All of the following functions will be replaced in a bit with the function above, IdxClassification() */
static int IsValidIdx(lua_State *L, int idx) {
    Prep;
    
    if        (idx > 0) {
        return (idx <= I->Top())        ? TRUE : FALSE;
    } else if (idx < 0) {
        return (idx >= (-1 * I->Top())) ? TRUE : FALSE;
    } else {
        return FALSE;
    };
};

/*  So, there is a bit of an issue with the design of this one...
There's not really any notion of an equivalent to lua_checkstack()
in the GVomit API, but also no indication of whether or not it's
applicable. If it's not applicable (because GVomit ensures there's
always enough space), then we can just always return yes so long
as the type of index is sensible. But if that's *not* the case, then
we'll pretty much just have to come up with a constant number out of
our ass for determining if there is enough space. And needless to
say, part of the issue is that lua_checkstack() is also designed to
*add* stack space if there's not enough space...no way to do that,
except perhaps by repeated pushing of 'nil' followed by popping an
equal number. */
static int IsAcceptableIdx(lua_State *L, int idx) {
};

static int IsPseudoIdx(lua_State *L, int idx) {
    return ((idx <= LUA_REGISTRYINDEX) && (idx >= lua_upvalueindex(255))) ? TRUE : FALSE;
};

static int IsUpvalueIdx(lua_State *L, int idx) {
    return ((idx <= lua_upvalueindex(1)) && (idx >= lua_upvalueindex(255))) ? TRUE : FALSE;
};

/* END AREA OF NOTICE */

static int UpvalueIdxToOrdinal(lua_State *L, int idx) {
    return (idx * -1) + LUA_GLOBALSINDEX;
};

#if 0
static ILuaObject* ObjFromIdx(lua_State *L, int idx) {
    Prep;
    
    if        (idx == 0) {
        I->Error(NonsensicalIdxError);
        return NULL;
    } else if (idx == LUA_GLOBALSINDEX) {
        return I->GetGlobal("_G");
    } else if (idx == LUA_REGISTRYINDEX) {
        return I->GetGlobal("_R");
    } else if (idx == LUA_ENVIRONINDEX) {
        pUR(I->GetGlobal("getfenv"));
        CallFunction(L, 0, 1);
        ILuaObject* retobj = I->GetObject(-1);
        I->Pop(1);
        return retobj;
    } else if (IsUpvalueIdx(L, idx)) {
        return GetOwnUpvalue(L, UpvalueIdxToOrdinal(L, idx));
    } else if (lua_absidx(L, idx) > I->GetStackTop()) {
        I->Error(InvalidIdxError);
        return;
    } else {
        return I->GetObject(idx);
    };
};
#elif 0
static ILuaObject* ObjFromIdx(lua_State *L, int idx) {
    Prep;
    
    if        (idx == 0) {
        I->Error(NonsensicalIdxError);
        return NULL;
    } else if (lua_absindex(L, idx) > I->GetStackTop()) {
        I->Error(InvalidIdxError);
        return NULL;
    } else if (IsUpvalueIdx(L, idx)) {
        return GetOwnUpvalue(L, UpvalueIdxToOrdinal(L, idx));
    } else {
        return I->GetObject(idx);
    };
};
#elif 1
static ILuaObject* ObjFromIdx(lua_State *L, int idx) {
    Prep;
    
    if (idx == LUA_REGISTRYINDEX) {
        return I->GetObject(LUA_REGISTRYINDEX);
    };
    
    switch (IdxClassification(L, idx, NULL, NULL)) {
        case VanIdx_Nonsensical :
        case VanIdx_AboveStack  :
            BadIdxError(L, __FUNCTION__, idx);
            return NULL;
        case VanIdx_Pseudo_Tbl  :
        case VanIdx_OnStack     :
            return I->GetObject(idx);
        case VanIdx_Pseudo_Upv  :
            return GetOwnUpvalue(L, UpvalueIdxToOrdinal(L, idx));
        default :
            luaL_error(L, "In library '%s': called function %s(), impossible condition encountered",
                OrigRequireName, __FUNCTION__);
            return NULL;
    };
};
#endif

static void BadIdxError(lua_State *L, const char* funcname, int idx) {
    switch (IdxClassification(L, idx, NULL, NULL)) {
        case VanIdx_Nonsensical :
            luaL_error(L, "In library '%s': called function %s(), bad index '%d' (nonsensical), stack top: '%d'",
                OrigRequireName, funcname, idx, lua_gettop(L));
            return;
        case VanIdx_OnStack :
            luaL_error(L, "In library '%s': called function %s(), bad index '%d', stack top: '%d'",
                OrigRequireName, funcname, idx, lua_gettop(L));
            return;
        case VanIdx_AboveStack :
            luaL_error(L, "In library '%s': called function %s(), bad index '%d' (above stack), stack top: '%d'",
                OrigRequireName, funcname, idx, lua_gettop(L));
            return;
        case VanIdx_Pseudo_Tbl :
           {const char* idxname;
            switch (idx) {
                case LUA_REGISTRYINDEX :
                    idxname = "LUA_REGISTRYINDEX"; break;
                case LUA_ENVIRONINDEX  :
                    idxname = "LUA_ENVIRONINDEX";  break;
                case LUA_GLOBALSINDEX  :
                    idxname = "LUA_GLOBALSINDEX";  break;};
            luaL_error(L, "In library '%s': called function %s(), bad index '%s'",
                OrigRequireName, funcname, idxname);};
            return;
        case VanIdx_Pseudo_Upv :
            luaL_error(L, "In library '%s': called function %s(), bad index 'lua_upvalueindex(%d)'",
                OrigRequireName, funcname, UpvalueIdxToOrdinal(L, idx));
            return;
    };
};

static VanillinSavedVals* SaveStackVals(lua_State* L, unsigned int nvals) {
    Prep;
    
    VanillinSavedVals* svals = (VanillinSavedVals*)HeapAlloc(ThisHeap, 0, (sizeof(unsigned int) + (sizeof(ILuaObject*) * nvals)));
    int top = I->GetStackTop();
    svals->nvals = nvals;
    
    if (nvals > top) {
        I->Error("SaveStackVals() was asked to save more values than are actually on the stack!");
        return NULL;
    };
    
    int stkidx = top - (nvals - 1);
    int arridx = 0;
    while (arridx < nvals) {
        svals->vals[arridx] = I->GetObject(stkidx);
        stkidx += 1;
        arridx += 1;
    };
    
    I->Pop(nvals);
    
    return svals;
};

static void RestoreStackVals(lua_State* L, VanillinSavedVals* svals) {
    Prep;
    
    unsigned int nvals = svals->nvals;
    for (int arridx = 0; arridx < nvals; arridx++) {
        pUR(svals->vals[arridx]);
    };
    
    HeapFree(ThisHeap, 0, (void*)svals);
    
    return;
};

/* CallFunction() exists because none of the GVomit Call() functions actually work. There are three of them:
    1. I->Call(int args, int returns)
    2. I->Call(ILuaObject* func, LArgList* in, LArgList* out)
    3. I->Call(ILuaObject* func, LArgList* in, ILuaObject* member)
   Going in reverse order...
    3. I don't actually know how this one even works. HEADERS ARE NOT A SUBSTITUTE FOR DOCUMENTATION. I'm soon going
    to sound like a broken record about this, but Jesus H. Christ, GVomit doesn't have *sparse* documentation, it has
    *no* documentation. 
    2. Sharing a problem with the above, this one uses the LArgList type, which has ONE mention in all of the headers.
    That single mention says that there *is* a class called LArgList, with no elaboration on what it contains in any
    way, and a note saying that it's obsolete and exists only for backwards compatibility. Great fuckin' help, that.
    1. Well now, this looks like the one we'd want to use anyway...after all, it's basically the stupid GVomit version
    of lua_call(). Right? Well...it would be. IF THE ABHORRED THING ACTUALLY PUSHED ANY RETURN VALUES ON THE STACK. After
    several hours of frustrating experimentation, I was able to determine that I->Call() never pushes any return values
    on the stack, no matter how many you ask for, no matter how many the function returns, it never actually pushes any
    on the stack. 
    TO BE CONTINUED */
static void StackDump(lua_State *L, int linnum) {
    Prep;
    
    I->Msg("Dump of stack at line %d:\n", linnum);
    
    int i;
    for (i = -1; i >= -(I->Top()); i--) {
        I->PushValue(i); const char* tostring = I->GetString(-1); I->Pop(1);
        I->Msg("[%d] (%s) %s\n", I->Top()+i+1, lua_typename(L, lua_type(L, i)), tostring);
    };
    
    return;
};
#if 0
static void CallFunction(lua_State *L, unsigned int nargs, unsigned int nresults) {
    Prep;
    
    I->Push(I->GetGlobal("_R")->GetMember(guidPlacerFunc)); /* I had trouble "inserting" PlacerFunc, so I'll try
        replicating the call stack *above* the point where you entered the call...*just* for now. */
    {int i = nargs + 1;
     while (i >= 1) {
        I->PushValue(-(nargs+2));
        i -= 1;
    };};
    I->Call(nargs+1, 0);
    
    /* Now to push the results... */
    I->Pop(nargs+1); /* Strip the original call stack */
    {ILuaObject* tblReturns = I->GetGlobal("_R")->GetMember(guidReturnValues);
     int nActualReturns     = tblReturns->GetMember("n")->GetInt();
     int nDesiredReturns    = (nresults == LUA_MULTRET) ? nActualReturns : nresults;
     
     int nReturnsToPush = (nDesiredReturns < nActualReturns) ? nDesiredReturns : nActualReturns;
     int nNilsToPush    = (nDesiredReturns < nActualReturns) ? 0               : (nActualReturns - nDesiredReturns);
     {int r;
      ILuaObject* idx = NULL;
      for (r = 1; r <= nReturnsToPush; r++) {
        I->PushLong(r);
        idx = I->GetObject(-1);
        I->Pop(1);
        
        I->Push(tblReturns->GetMember(idx));
     };};
     {int n;
      for (n = 1; n <= nNilsToPush; n++) {
        I->PushNil();
     };};
    };

    return;
};
#elif 0
/*  Or, even worse, it turns out that *this* is what you must do to get return values: when you
call I->Call(), the return values are stashed in some invisible place, and the only way to get them
is to call I->GetReturn() (which is not mentioned anywhere), which for some inexplicable (and 
also undocumented) reason takes a 0-based *positive-only* numeric index from the top of the stack,
and directly returns an object, which I only just now found (by accidenT) have to be manually 
reference-counted.
    GVomit sure does make life so much easier, doesn't it? */
static void CallFunction(lua_State *L, unsigned int nargs, unsigned int nresults) {
    Prep;
    
    I->Call(nargs, nresults);
    for (int ridx = nresults - 1; ridx >= 0; ridx++) {
        ILuaObject* ResultObj = I->GetReturn(ridx);
        ResultObj->Push();
        ResultObj->UnReference();
    };
    
    return;
};
#else
/*  Finally cracked the I->Call() and I->GetReturn() crap. Simple solution: pass a negative number
to I->Call() for the number of results, it will put the exact amount of results returned by the
function directly onto the stack. Found out completely by accident. */
static void CallFunction(lua_State *L, unsigned int nargs, unsigned int nresults) {
    Prep;
    
    if (lua_type(L, -(nargs+1)) != LUA_TFUNCTION) {
        I->Msg("Calling a non-function value wanting %d args and %d results, dumping stack for debugging:", nargs, nresults);
        StackDump(L, 6666);
    };
    
    if        (nresults == 0) {
        I->Call(nargs, 0);
    } else if (nresults > 0) {
        int DesiredTop = I->GetStackTop() - nargs - 1 + nresults;
        I->Call(nargs, -1);
        lua_settop(L, DesiredTop);
    } else if (nresults == LUA_MULTRET) {
        I->Call(nargs, -1);
    } else { /* Negative 'nresults' other than LUA_MULTRET are not allowed! */
        I->Error(InvalidNresultsError);
        return;
    };
    
    return;
};
#endif

static ILuaObject* GetSelf(lua_State *L) {
    Prep;

    pUR(iwStringUR(I->GetGlobal("debug"), "getinfo"));
    I->PushLong(1);
    I->Push("f");
    CallFunction(L, 2, 1);
    ILuaObject* ret = iwStringUR(I->GetObject(-1), "func"); I->Pop(1);
    
    return ret;
};

#if 0
static ILuaObject* GetOwnUpvalue(lua_State* L, int nupvalue) {
    Prep;
    
    I->Push(I->GetGlobal("debug")->GetMember("getinfo"));
    I->PushLong(1);
    I->Push("f");
    CallFunction(L, 2, 1);
    ILuaObject* OwnFunc = I->GetObject(-1)->GetMember("func");
    I->Pop(1);
    ILuaObject* FuncUpvalTable = I->GetGlobal("_R")->GetMember(guidUpvalues)->GetMember(OwnFunc);
    
    if (FuncUpvalTable->isNil()) {
        return FuncUpvalTable;
    } else { /* No need to check to see if it's within bounds. If it's not, then it'll just be nil, as intended! */
        I->PushLong((long)nupvalue);
        ILuaObject* retobj = FuncUpvalTable->GetMember(I->GetObject(-1));
        I->Pop(1);
        return retobj;
    };
};
#elif 0
static ILuaObject* GetOwnUpvalue(lua_State* L, int nupvalue) {
    Prep;
    
    pUR(iwStringUR(I->GetGlobal("debug"), "getinfo"));
    I->PushLong(1);
    I->Push("f");
    CallFunction(L, 2, 1);
    ILuaObject* self   = iwStringUR(I->GetObject(-1), "func"); I->Pop(1);
    ILuaObject* upvTbl = iwObjURBoth(iwStringUR(I->GetGlobal("_R"), guidUpvalues), self);
    if (upvTbl->isNil()) {
        return upvTbl;
    } else {
        return iwIntUR(upvTbl, nupvalue);
    };
};
#else
static ILuaObject* GetOwnUpvalue(lua_State* L, int nupvalue) {
    Prep;
    
    ILuaObject* upvTbl = iwObjURBoth(iwStringUR(I->GetObject(LUA_REGISTRYINDEX), guidUpvalues), GetSelf(L));
    if (upvTbl->isNil()) {
        return upvTbl;
    } else {
        return iwIntUR(upvTbl, nupvalue);
    };
};
#endif

static int lua_absindex(lua_State *L, int idx) {
    if ((idx >= 0) || (idx <= LUA_REGISTRYINDEX)) {
        return idx;
    } else {
        return lua_gettop(L) + 1 + idx;
    };
};

static bool IsVanillinUD(lua_State *L, int idx) {
    lua_getfield(L, LUA_REGISTRYINDEX, guidUDAnchors);
    lua_pushvalue(L, lua_absindex(L, idx));
    lua_gettable(L, -2);
    bool ret = (bool) lua_isnil(L, -1);
    lua_pop(L, 2);
    
    return ret;
};

/* The lua_CFunction which frees memory associated with full userdata. */
static int UserdataGC(lua_State *L) {
    Prep;
    
    void* MemoryBlock = I->GetUserData(1);
    I->Msg("Freeing userdata at address %p with size %d bytes", MemoryBlock, HeapSize(ThisHeap, 0, MemoryBlock));
    HeapFree(ThisHeap, 0, MemoryBlock);
    
    return 0;
};

/* Reference-Count Helper Functions */

/* push and UnReference */
static __inline void pUR(ILuaObject* obj) {
    obj->Push();
    obj->UnReference();
    
    return;
};

/* retrieve Type and UnReference */
static __inline int rTypeUR(ILuaObject* obj) {
    int ret = obj->GetType();
    obj->UnReference();
    
    return ret;
};

/* retrieve Integer and UnReference */
static __inline int rIntUR(ILuaObject* obj) {
    int ret = obj->GetInt();
    obj->UnReference();
    
    return ret;
};

/* retrieve String and UnReference */
static __inline const char* rStringUR(ILuaObject* obj) {
    const char* ret = obj->GetString();
    obj->UnReference();
    
    return ret;
};

static __inline void* rUserDataUR(ILuaObject* obj) {
    void* ret = obj->GetUserData();
    obj->UnReference();
    
    return ret;
};

/* index with String and UnReference */
static __inline ILuaObject* iwStringUR(ILuaObject* obj, const char* key) {
    ILuaObject* ret = obj->GetMember(key);
    obj->UnReference();
    
    return ret;
};

/* index with Int and UnReference */
static __inline ILuaObject* iwIntUR(ILuaObject* obj, int key) {
    ILuaObject* ret = obj->GetMember((float)key);
    obj->UnReference();
    
    return ret;
};

/* index with ILuaObject and UnReference table */
static __inline ILuaObject* iwObjUR(ILuaObject* obj, ILuaObject* key) {
    ILuaObject* ret = obj->GetMember(key);
    obj->UnReference();
    
    return ret;
};

/* index with ILuaObject and UnReference table and key */
static __inline ILuaObject* iwObjURBoth(ILuaObject* obj, ILuaObject* key) {
    ILuaObject* ret = obj->GetMember(key);
    obj->UnReference();
    key->UnReference();
    
    return ret;
};