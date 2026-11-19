local Crosh = {}
local State = {}
local Logger = {}
local Tokenizer = {}
local Parser = {}
local LuaGenerator = {}

function Logger:Error(...)
	io.stdout:flush()
	io.stderr:write("\27[1;37mCrosh:\27[0m ")
	io.stderr:write("\27[1;31mError:\27[0m ")
	io.stderr:write(string.format(...))
	io.stderr:write("\n")
	os.exit(1)
end

function Logger:PrintTable(Table, Indent, Seen)
	Indent = Indent or 0
	Seen = Seen or {}

	if type(Table) ~= "table" then
		print(string.rep("  ", Indent) .. tostring(Table))
		return
	end

	if Seen[Table] then
		print(string.rep("  ", Indent) .. "<circular reference>")
		return
	end

	Seen[Table] = true

	local Prefix = string.rep("  ", Indent)

	print(Prefix .. "{")

	for Key, Value in pairs(Table) do
		io.write(Prefix .. "  [" .. string.format("%q", Key) .. "] = ")

		if type(Value) == "table" then
			print()
			Logger:PrintTable(Value, Indent + 1, Seen)
		else
			print(string.format("%q", Value) .. ",")
		end
	end

	print(Prefix .. "}")

	Seen[Table] = nil
end

local Operators = {
	"||",
	"&&",
	"|",
	"&",
	";",
	"(",
	")",
	"{",
	"}",
	"<",
	">",
	"!",
}

local Whitespace = {
	[" "] = true,
	["\t"] = true,
	["\r"] = true,
}

function Tokenizer:NewToken(Kind, Value, Quote)
	local Token = {
		Kind = Kind,
		Value = Value,
		Quote = Quote,
	}

	table.insert(State.Tokens, Token)
	return Token
end

function Tokenizer:Advance(Step)
	Step = Step or 1

	Tokenizer.Index = Tokenizer.Index + Step

	if Tokenizer.Index < 1 then
		Tokenizer.Index = 1
	end

	Tokenizer.Character = State.BashCode:sub(Tokenizer.Index, Tokenizer.Index)

	return Tokenizer.Character
end

function Tokenizer:Peek(Offset)
	Offset = Offset or 1

	return State.BashCode:sub(Tokenizer.Index + Offset, Tokenizer.Index + Offset)
end

function Tokenizer:Match(Value)
	if State.BashCode:sub(Tokenizer.Index, Tokenizer.Index + #Value - 1) == Value then
		return true
	end

	return false
end

function Tokenizer:SkipWhitespace()
	while Whitespace[Tokenizer.Character] do
		Tokenizer:Advance(1)
	end
end

function Tokenizer:ReadOperator()
	for _, Operator in ipairs(Operators) do
		if Tokenizer:Match(Operator) then
			Tokenizer:NewToken("Operator", Operator)
			Tokenizer:Advance(#Operator)
			return true
		end
	end

	return false
end

function Tokenizer:ReadQuoted()
	local Quote = Tokenizer.Character
	local Start = Tokenizer.Index + 1

	while #Tokenizer.Character > 0 do
		if Tokenizer.Character == "\\" and Quote == '"' then
			Tokenizer:Advance(2)
		elseif Tokenizer.Character == Quote then
			local Value = State.BashCode:sub(Start, Tokenizer.Index - 1)

			Tokenizer:NewToken("String", Value, Quote)
			Tokenizer:Advance(1)

			return
		else
			Tokenizer:Advance(1)
		end
	end

	Logger:Error("Unterminated quoted string")
end

function Tokenizer:ReadIdent()
	local Start = Tokenizer.Index

	while #Tokenizer.Character > 0 do
		if Whitespace[Tokenizer.Character] then
			break
		end

		if
			Tokenizer:Match("|")
			or Tokenizer:Match("&")
			or Tokenizer:Match(";")
			or Tokenizer:Match("(")
			or Tokenizer:Match(")")
			or Tokenizer:Match("<")
			or Tokenizer:Match(">")
		then
			break
		end

		if Tokenizer.Character == "'" or Tokenizer.Character == '"' then
			break
		end

		Tokenizer:Advance(1)
	end

	if Tokenizer.Index == Start then
		Logger:Error("Unexpected character: %s", Tokenizer.Character)
	end

	local Value = State.BashCode:sub(Start, Tokenizer.Index - 1)

	Tokenizer:NewToken("Ident", Value)
end

function Tokenizer:ReadComment()
	while #Tokenizer.Character > 0 and Tokenizer.Character ~= "\n" do
		Tokenizer:Advance(1)
	end
end

function Tokenizer:ReadNewline()
	Tokenizer:NewToken("Newline")
	Tokenizer:Advance(1)
end

function Tokenizer:Preprocess(BashCode)
	BashCode = BashCode:gsub("\\\r\n", "")
	BashCode = BashCode:gsub("\\\n", "")
	return BashCode
end

function Tokenizer:Tokenize()
	State.Tokens = {}

	State.BashCode = Tokenizer:Preprocess(State.BashCode)

	Tokenizer.Index = 1
	Tokenizer.Character = State.BashCode:sub(1, 1)
	while #Tokenizer.Character > 0 do
		Tokenizer:SkipWhitespace()

		if #Tokenizer.Character == 0 then
			break
		end

		if Tokenizer.Character == "'" or Tokenizer.Character == '"' then
			Tokenizer:ReadQuoted()
		elseif Tokenizer.Character == "#" then
			Tokenizer:ReadComment()
		elseif Tokenizer.Character == "\n" then
			Tokenizer:ReadNewline()
		elseif Tokenizer:ReadOperator() then
		else
			Tokenizer:ReadIdent()
		end
	end

	return State.Tokens
end

function Parser:Advance(Step)
	Parser.Index = Parser.Index + Step
	Parser.Token = State.Tokens[Parser.Index]
end

function Parser:ParseCommand()
	local Command = {}
	Command.Kind = "Command"
	Command.Arguments = {}

	while Parser.Token and Parser.Token.Kind ~= "Newline" do
		table.insert(Command.Arguments, Parser.Token)
		Parser:Advance(1)
	end

	return Command
end

function Parser:ParseStatement()
	Logger:PrintTable(Parser.Token)
	if Parser.Token.Kind == "Ident" then
		return Parser:ParseCommand()
	else
		Logger:Error("No handler for token %s", Parser.Token.Kind)
	end
end

function Parser:Parse()
	State.Ast = {}
	Parser.Index = 1
	Parser.Token = {}
	Parser:Advance(0)

	while Parser.Token do
		local Statemenet = Parser:ParseStatement()
		table.insert(State.Ast, Statemenet)
	end
end

function LuaGenerator:Generate()
	State.LuaCode = ""
end

function Crosh:CompileBash(BashCode)
	State.BashCode = BashCode

	Tokenizer:Tokenize()
	Logger:PrintTable(State.Tokens)

	Parser:Parse()
	Logger:PrintTable(State.Ast)

	LuaGenerator:Generate()

	return State.LuaCode
end

function Crosh:Compile(BashCode)
	local LuaCode = Crosh:CompileBash(BashCode)

	local Function, Error = loadstring(LuaCode)

	if not Function then
		Logger:Error("Failed to compile generated Lua: %s", Error)
	end

	return Function
end

return Crosh, Logger.Error, Logger.PrintTable
