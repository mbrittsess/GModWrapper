local Registry = _R

local guidPlacerFunc   = "{29831788-D846-4a8f-923D-57EE9005C60E}"
local guidReturnValues = "{F4318423-0526-4c5d-A60F-9AE23F13AF2D}"

local select = select

local function StuffTable(...)
    return {...}, select("#", ...)
end

Registry[guidPlacerFunc] = function(func, ...)
    local results, nresults = StuffTable(func(...))
    results.n = nresults
    
    Registry[guidReturnValues] = results
end