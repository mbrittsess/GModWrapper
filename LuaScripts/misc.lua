local guidConcatFunc   = "{3C0C3DEF-DC94-4043-9073-4B21E10419A3}"
local guidLessThanFunc = "{9FFB88AB-5676-4345-85B8-B30EBB5C4CE2}"
local guidToPtrFunc    = "{6E540BD9-836B-4419-998F-97D6E21295AA}"
local guidLPCall       = "{84A25691-6B47-4b34-AC5D-19C1CB021C8A}"

local _R = _R

local concat = table.concat
if not _R[guidConcatFunc] then _R[guidConcatFunc] = function(...)
    return concat({...})
end end

if not _R[guidLessThanFunc] then _R[guidLessThanFunc] = function(obj1, obj2)
    return obj1 < obj2
end end

local d_getmetatable, d_setmetatable = debug.getmetatable, debug.setmetatable
local match, tonumber, tostring = string.match, tonumber, tostring
if not _R[guidToPtrFunc] then _R[guidToPtrFunc] = function(obj)
    local mt = d_getmetatable(obj)
    if mt then d_setmetatable(obj, nil) end
    local retval = tonumber(match(tostring(obj), "(%x+)$"), 16)
    if mt then d_setmetatable(obj, mt) end
    return retval
end end

local pcall, unpack = pcall, unpack
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
end end