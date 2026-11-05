local menu = hs.menubar.new():setTitle("Totem: —")
local events = hs.eventtap.event.types
local labels = {
    f13 = "BASE",
    f14 = "NUM",
    f15 = "SYM",
    f16 = "NAV",
    f17 = "FN",
    f18 = "GAME",
    f19 = "SYS",
}

local tap = hs.eventtap.new({events.keyDown, events.keyUp}, function(event)
    local label = labels[hs.keycodes.map[event:getKeyCode()]]
    if label == nil then
        return false
    end
    if event:getType() == events.keyDown then
        menu:setTitle("Totem: " .. label)
    end
    return true
end):start()

return {menu = menu, tap = tap}
