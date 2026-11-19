local Crosh = {}
local State = {}
local Tokenizer = {}
local Parser = {}
local LuaGenerator = {}

function Tokenizer:NewToken(Kind, Value)
	Tokenizer.Token = { Kind = Kind, Value = Value }
	table.insert(State.Tokens, Tokenizer.Token)
end

function Tokenizer:Advance(Step)
	local Direction = Step > 0 and 1 or -1

	for Idx = 1, math.abs(Step), Direction do
		Tokenizer.Character = State.BashCode:sub(Tokenizer.Index, Tokenizer.Index)
		Tokenizer.Index = Tokenizer.Index + Direction
	end
end

function Tokenizer:Tokenize()
	local BashCode = State.BashCode

	Tokenizer.Index = 1
	Tokenizer.Character = State.BashCode:sub(1, 1)
	State.Tokens = {}

	while #Tokenizer.Character > 0 do
		Tokenizer:Advance(1)
	end
end

function Parser:Parse() end

function LuaGenerator:Generate() end

function Crosh:CompileBash(BashCode)
	State.BashCode = BashCode

	Tokenizer:Tokenize()
	Parser:Parse()
	LuaGenerator:Generate()

	return State.LuaCode or ""
end

function Crosh:Compile(BashCode)
	local LuaCode = Crosh:CompileBash(BashCode)
	return loadstring(LuaCode)
end

return Crosh
