local Crosh = require("Crosh")

local Bash = table.concat(arg, " ", 1)
local Lua = Crosh:Compile(Bash)
Lua()
