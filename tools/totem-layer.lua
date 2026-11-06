local menu = hs.menubar.new():setTitle("Totem: —")
local labels = {"BASE", "NUM", "SYM", "NAV", "FUNC", "GAME", "SYS"}

local watcher = hs.distributednotifications.new(function(_, _, info)
    local index = info and info.layer
    local label = type(index) == "number" and labels[index + 1] or "—"
    menu:setTitle("Totem: " .. label)
end, "com.totem.layer.changed"):start()

hs.application.launchOrFocusByBundleID("com.totem.layer.reader")
hs.distributednotifications.post("com.totem.layer.request")

return {menu = menu, watcher = watcher}
