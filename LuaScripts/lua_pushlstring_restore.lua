local InitialCopy, DiffCopy = ...

if (type(InitialCopy) ~= "string") or (type(DiffCopy) ~= "string") or (#InitialCopy ~= #DiffCopy) then
    error("lua_pushlstring()'s anonymous Lua function was passed a non-string, or the passed strings are of different lengths!")
else
    local idx = 0
    return string.gsub(InitialCopy, '.', function(char) --[[We don't use '\1' as a pattern because there would be no way to tell *where*
        the match occurred.]]
        idx = idx + 1
        if char == '\1' then
            return (DiffCopy:sub(idx, idx) == '1') and '\0' or false
        else
            return false
        end
    end)
end