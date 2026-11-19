local Crosh = {}

function Crosh:CompileBash(BashCode)
	return "print('meow')"
end

function Crosh:Compile(BashCode)
	local LuaCode = Crosh:CompileBash(BashCode)
	return loadstring(LuaCode)
end

return Crosh
