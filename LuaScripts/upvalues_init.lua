local guidUpvalues = "{F56CF593-BBB6-4f54-8320-9B0942A85506}"
local guidCFuncPointers = "{49C97096-83FA-40d3-861B-6015CA9D41BD}" --Doing this here because we might as well anyway

local weak_key_meta = {__mode = "k"}

if not _R[guidUpvalues] then _R[guidUpvalues] = setmetatable({}, weak_key_meta) end
if not _R[guidCFuncPointers] then _R[guidCFuncPointers] = setmetatable({}, weak_key_meta) end