local len = 0
local arr = {}
for line in io.lines() do
    local val, str = line:match("^(%d+)%s+(%S+)")
    len = (#str > len) and #str or len
    arr[tonumber(val)] = str
end
for i = 0, #arr do
    io.write(string.format("  [%-"..tostring(len+3).."s] = \n", '"'..arr[i]..'"'))
end