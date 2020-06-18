local OrigRequireName, udMeta, udAnch = ...

local guidLPLS_AnonFunc = "{5D623233-426C-4983-9779-0E7EEC23541E}"
local guidUpvalues      = "{F56CF593-BBB6-4f54-8320-9B0942A85506}"
local guidUDAnchors     = "{E98ACFD3-7ECF-45f6-B82B-8FB1F58EB601}"
local guidUDSizes       = "{B7623232-CD2A-4122-898C-AB6E67678908}"
local guidUDEnvs        = "{B4632E5C-BDA6-46c0-8950-9E722EDE34D7}"
local guidUDPrepFunc    = "{AD0D9949-2232-4505-B92C-7342216A6A8F}"
local guidCFuncPointers = "{49C97096-83FA-40d3-861B-6015CA9D41BD}"
local guidLenFunc       = "{771DD06F-92A2-4e23-8EEC-317F7E17A3D8}"
local guidPushfString   = "{CC38C278-8672-43a0-AFA4-9C2A90FC91B9}"
local guidConcatFunc    = "{3C0C3DEF-DC94-4043-9073-4B21E10419A3}"
local guidLessThanFunc  = "{9FFB88AB-5676-4345-85B8-B30EBB5C4CE2}"
local guidToPtrFunc     = "{6E540BD9-836B-4419-998F-97D6E21295AA}"
local guidLPCall        = "{84A25691-6B47-4b34-AC5D-19C1CB021C8A}"
local guidLGSRefs       = "{EF44B6B3-C150-4955-8337-5D57DEAB8303}"

local _R = _R
local weak_key_meta = {__mode = "k"}
local weak_val_meta = {__mode = "v"}

--lua_pushlstring()'s function
do local gsub, sub = string.gsub, string.sub
if not _R[guidLPLS_AnonFunc] then _R[guidLPLS_AnonFunc] = function(InitialCopy, DiffCopy)
    local function subfunc(pos)
        return (sub(DiffCopy, pos, pos) == '1') and '\0' or false
    end
    return gsub(InitialCopy, "()\1", subfunc)
end end end

--lua_pushfstring()'s function
do local ins, concat, fmt, type = table.insert, table.concat, string.format, type
local actions = {
    ["string"]  = function(pos, value, args, strings)
        ins(strings, value)
    end;
    ["boolean"] = function(pos, value, args, strings)
        return
    end;
    ["number"]  = function(pos, value, args, strings)
        if type(args[pos-1]) == "boolean" then
            ins(strings, fmt("%d", value))
        else
            ins(strings, fmt("%f", value))
        end
    end;
}

if not _R[guidPushfString] then _R[guidPushfString] = function(...)
    local args = {...}
    local strings = {}
    for pos, value in ipairs(args) do actions[type(value)](pos, value, args, strings) end
    
    return concat(strings)
end end end

--Full Userdata stuff
do local d_setmetatable = debug.setmetatable

if not _R[guidUDAnchors] then _R[guidUDAnchors] = setmetatable({}, weak_key_meta) end
if not _R[guidUDSizes]   then _R[guidUDSizes]   = setmetatable({}, weak_key_meta) end
if not _R[guidUDEnvs]    then _R[guidUDEnvs]    = setmetatable({}, weak_key_meta) end

udMeta.MetaName, udMeta.__metatable = "userdata", false

--if not udAnch.__gc then udAnch.__gc = udGCFunc end

local UDAnchors, UDSizes, UDEnvs = _R[guidUDAnchors], _R[guidUDSizes], _R[guidUDEnvs]

if not _R[guidUDPrepFunc] then _R[guidUDPrepFunc] = function(udObj, udAnch, udSize, udEnv)
    UDAnchors[udObj] = udAnch
    UDSizes[udObj]   = udSize
    UDEnvs[udObj]    = udEnv
    
    return udObj
end end end

--Minor object property/action stuff
do local concat, d_getmetatable, d_setmetatable = table.concat, debug.getmetatable, debug.setmetatable
local match, tonumber, tostring = string.match, tonumber, tostring
if not _R[guidLenFunc] then _R[guidLenFunc] = function(obj)
    return #obj
end end

if not _R[guidLessThanFunc] then _R[guidLessThanFunc] = function(obj1, obj2)
    return obj1 < obj2
end end

if not _R[guidConcatFunc] then _R[guidConcatFunc] = function(...)
    return concat({...})
end end

if not _R[guidToPtrFunc] then _R[guidToPtrFunc] = function(obj)
    local mt = d_getmetatable(obj)
    if mt then d_setmetatable(obj, nil) end
    local retval = tonumber(match(tostring(obj), "(%x+)$"), 16)
    if mt then d_setmetatable(obj, mt) end
    return retval
end end end

--lua_pcall()'s function
do local pcall, unpack = pcall, unpack
if not _R[guidLPCall] then _R[guidLPCall] = function(func, errfunc, ...)
    local res = {pcall(func, ...)}
    if res[1] == true then
        return 1, unpack(res, 2) --Function executed successfully
    else
        if not errfunc then
            return 2, res[2]     --Function errored, no error handler
        else
            res = {pcall(errfunc, res[2])}
            if res[1] == true then
                return 3, res[2] --Function errored, error handler ran successfully
            else
                return 4, res[2] --Function errored, error handler errored
            end
        end
    end
end end end

--lua_getstack() references
if not _R[guidLGSRefs] then _R[guidLGSRefs] = setmetatable({}, weak_val_meta) end

--Upvalue stuff
if not _R[guidUpvalues] then _R[guidUpvalues] = setmetatable({}, weak_key_meta) end
if not _R[guidCFuncPointers] then _R[guidCFuncPointers] = setmetatable({}, weak_key_meta) end

return ("luaopen_"..(OrigRequireName:match("%-") and OrigRequireName:match("%-(.*)") or OrigRequireName):gsub("%.", "_")),
       (function(udGCFunc)
            if not udAnch.__gc then udAnch.__gc = udGCFunc end
        end)
