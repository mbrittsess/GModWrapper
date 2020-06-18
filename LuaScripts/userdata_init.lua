local guidUDAnchors  = "{E98ACFD3-7ECF-45f6-B82B-8FB1F58EB601}"
local guidUDSizes    = "{B7623232-CD2A-4122-898C-AB6E67678908}"
local guidUDEnvs     = "{B4632E5C-BDA6-46c0-8950-9E722EDE34D7}"
local guidUDPrepFunc = "{AD0D9949-2232-4505-B92C-7342216A6A8F}"
local guidLenFunc    = "{771DD06F-92A2-4e23-8EEC-317F7E17A3D8}"
local udMeta, udAnch, udGCFunc = ...

local weak_meta = {__mode = "k"}

if not _R[guidUDAnchors] then _R[guidUDAnchors] = setmetatable({}, weak_meta) end
if not _R[guidUDSizes] then _R[guidUDSizes] = setmetatable({}, weak_meta) end
if not _R[guidUDEnvs] then _R[guidUDEnvs] = setmetatable({}, weak_meta) end

udMeta.MetaName = "userdata"
udMeta.__metatable = false --This is as good as we can get for making our userdata appear to not have metatables.
if not udAnch.__gc then udAnch.__gc = udGCFunc end

local d_setmetatable = debug.setmetatable
local UDAnchors = _R[guidUDAnchors]
local UDSizes   = _R[guidUDSizes]
local UDEnvs    = _R[guidUDEnvs]
if not _R[guidUDPrepFunc] then _R[guidUDPrepFunc] = function(udObj, udAnch, udSize, udEnv)
    --d_setmetatable(udObj, nil) --It turns out, GMod loses its shit and craps all over itself if it encounters a UD with no metatable.
    UDAnchors[udObj] = udAnch
    UDSizes[udObj] = udSize
    UDEnvs[udObj] = udEnv
    
    return udObj
end end

if not _R[guidLenFunc] then _R[guidLenFunc] = function(obj)
    return #obj
end end