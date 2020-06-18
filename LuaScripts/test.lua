local foo = "Hello, world!\1C:\\Program Files\\\1Buggy worm guys!\1Heyo!\1"
local bar = "0000000000000\0010000000000000000000000000000000000\00100000\001"
print(#foo, #bar)

local s = foo:gsub("()\1", function(pos) 
    return (bar[pos]=='\1') and '\0' or false
end)

print(#s, s)