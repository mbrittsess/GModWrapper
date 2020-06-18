/* Every single function here places "Prep;" at the beginning of each function.
It can prepare anything that needs to be done in every single function. */
#define Prep ILuaInterface* I = mm->GetLuaInterface(L)

/* Global variables */
ILuaModuleManager* mm = NULL;
HANDLE ThisHeap = NULL;
const char* OrigRequireName = NULL;

/* GUIDs Used */
static const char* guidLPLS_AnonFunc = "{5D623233-426C-4983-9779-0E7EEC23541E}"; /* Version 1 */
static const char* guidPlacerFunc    = "{29831788-D846-4a8f-923D-57EE9005C60E}"; /* Version 1 */
static const char* guidReturnValues  = "{F4318423-0526-4c5d-A60F-9AE23F13AF2D}"; /* Version 1 */
static const char* guidUpvalues      = "{F56CF593-BBB6-4f54-8320-9B0942A85506}"; /* Version 1 */
static const char* guidTypeMapInt    = "{B3AA565A-B9B3-4d3a-BEDA-669FDC835B6D}"; /* Version 1 */
static const char* guidTypeMapStr    = "{AD8E8A8E-2735-4D9A-B22D-13A64BD3B685}"; /* Version 1 */
static const char* guidUDAnchors     = "{E98ACFD3-7ECF-45f6-B82B-8FB1F58EB601}"; /* Version 1 */
static const char* guidUDSizes       = "{B7623232-CD2A-4122-898C-AB6E67678908}"; /* Version 1 */
static const char* guidUDEnvs        = "{B4632E5C-BDA6-46c0-8950-9E722EDE34D7}"; /* Version 1 */
static const char* guidUDPrepFunc    = "{AD0D9949-2232-4505-B92C-7342216A6A8F}"; /* Version 1 */
static const char* guidCFuncPointers = "{49C97096-83FA-40d3-861B-6015CA9D41BD}"; /* Version 1 */
static const char* guidLenFunc       = "{771DD06F-92A2-4e23-8EEC-317F7E17A3D8}"; /* Version 1 */
static const char* guidPushfString   = "{CC38C278-8672-43a0-AFA4-9C2A90FC91B9}"; /* Version 1 */
static const char* guidConcatFunc    = "{3C0C3DEF-DC94-4043-9073-4B21E10419A3}"; /* Version 1 */
static const char* guidLessThanFunc  = "{9FFB88AB-5676-4345-85B8-B30EBB5C4CE2}"; /* Version 1 */
static const char* guidToPtrFunc     = "{6E540BD9-836B-4419-998F-97D6E21295AA}"; /* Version 1 */
static const char* guidLPCall        = "{84A25691-6B47-4b34-AC5D-19C1CB021C8A}"; /* Version 1 */
static const char* guidLGSRefs       = "{EF44B6B3-C150-4955-8337-5D57DEAB8303}"; /* Version 1 */

/* Enumerations, constants */
enum VanillinIdxClass {
    VanIdx_Nonsensical,
    VanIdx_OnStack,
    VanIdx_AboveStack,
    VanIdx_Pseudo_Tbl,
    VanIdx_Pseudo_Upv
};

/* Structure types */
typedef struct {
    unsigned int nvals;
    ILuaObject* vals[];
} VanillinSavedVals;

/* Texts for some Lua scripts */
#if 0
static const char* PlacerFuncSrc = (""
    #include "LuaIncs\place_results.txt"
    );
static const char* LPLS_AnonFuncSrc = (""
    #include "LuaIncs\lua_pushlstring_restore.txt"
    );
static const char* GModOpenParsenameSrc = (""
    #include "LuaIncs\gmod_open_parsename.txt"
    );
static const char* UpvalueInitSrc = (""
    #include "LuaIncs\upvalues_init.txt"
    );
static const char* GType2LTypeSrc = (""
    #include "LuaIncs\gtype2luatype.txt"
    );
static const char* UserdataInitSrc = (""
    #include "LuaIncs\userdata_init.txt"
    );
static const char* PushfstringSrc = (""
    #include "LuaIncs\pushfstring.txt"
    );
static const char* MiscSrc = (""
    #include "LuaIncs\misc.txt"
    );
#else
static const char* LuaCombinedSrc = (""
    #include "LuaIncs\luacombined.txt"
    );
#endif
    
/* Meta-IDs for GMod's Userdata System */
static const int udMetaID       = 7166;
static const int udAnchorMetaID = 7177;

/* Various message strings */
static const char* NonsensicalIdxError = "'0' is a nonsensical stack index!";
static const char* MustBeActualStackIdxError = "Must pass an index specifying an actual stack location!";
static const char* TooManyUpvaluesError = "Cannot associate more than 255 upvalues with a closure!";
static const char* MoreUpvaluesThanStackSpacesError = "Tried to associate more upvalues than there are values actually on the stack!";
static const char* NegativeUpvaluesSpecifiedError = "lua_pushcclosure() cannot take a negative integer as a value for 'nupvalues'!";
static const char* InvalidNresultsError = "lua_call() only takes LUA_MULTRET and non-negative integers as values for 'nresults'!";
static const char* AttemptedSettingNonexistentUpvalue = "Tried to set upvalue #%d, function only has #%d upvalues!";
static const char* InvalidIdxError = "Attempted to pass a stack index outside of valid range!";
static const char* IllegalIdxError = "Attempted to pass a stack index not legal for this function!";

static const char* VanillinCannotImplementError = "In library '%s': called function %s(), Vanillin cannot implement it";
static const char* NotEnoughStackSpace = "In library '%s': called function %s(), needs at least %d stack space(s) (%d available)";

/* Prototypes for functions used */
extern "C" __declspec(dllexport) int gmod_open(ILuaInterface* I);
extern "C" __declspec(dllexport) int gmod_close(ILuaInterface* I);

static void CompileAndPush(lua_State *L, const char* src, const char* name);
static enum VanillinIdxClass IdxClassification(lua_State *L, int idx, int* aidx, void* StructPtr);
/* Going to be erasing the next few functions from here... */
static int IsValidIdx(lua_State *L, int idx);
static int IsAcceptableIndx(lua_State *L, int idx);
static int IsPseudoIdx(lua_State *L, int idx);
static int IsUpvalueIdx(lua_State *L, int idx);
/* Until here...once IdxClassification() is completed and used throughout the library */
static int UpvalueIdxToOrdinal(lua_State *L, int idx);
static ILuaObject* ObjFromIdx(lua_State *L, int idx);
static void BadIdxError(lua_State *L, const char* funcname, int idx);
static VanillinSavedVals* SaveStackVals(lua_State* L, unsigned int nvals);
static void RestoreStackVals(lua_State* L, VanillinSavedVals* svals);
static void StackDump(lua_State *L, int linnum);
static void CallFunction(lua_State *L, unsigned int nargs, unsigned int nresults);
static ILuaObject* GetSelf(lua_State *L);
static ILuaObject* GetOwnUpvalue(lua_State* L, int nupvalue);
static int lua_absindex(lua_State *L, int idx); /* Same name as the one in 5.2, own implementation */
static bool IsVanillinUD(lua_State *L, int idx);
static int UserdataGC(lua_State *L);
static __inline void pUR(ILuaObject* obj);
static __inline int rTypeUR(ILuaObject* obj);
static __inline int rIntUR(ILuaObject* obj);
static __inline const char* rStringUR(ILuaObject* obj);
static __inline ILuaObject* iwStringUR(ILuaObject* obj, const char* key);
static __inline ILuaObject* iwIntUR(ILuaObject* obj, int key);
static __inline ILuaObject* iwObjUR(ILuaObject* obj, ILuaObject* key);
static __inline ILuaObject* iwObjURBoth(ILuaObject* obj, ILuaObject* key);