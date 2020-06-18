local guidPushfString = "{CC38C278-8672-43a0-AFA4-9C2A90FC91B9}"

local _R = _R
local ins = table.insert
local fmt = string.format
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

local concat = table.concat
if not _R[guidPushfString] then _R[guidPushfString] = function(...)
    local args = {...}
    local strings = {}
    for pos, value in ipairs(args) do actions[type(value)](pos, value, args, strings) end
    
    return concat(strings)
end end