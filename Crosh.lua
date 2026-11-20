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

local Whitespace = {
	[" "] = true,
	["\t"] = true,
	["\r"] = true,
}

local Operators = {
	"&&",
	"||",
	"|&",
	"|",
	"&",
	";",
	";;",
	";&",
	";;&",
	"(",
	")",
	"{",
	"}",
	"!",
}

local Redirections = {
	"&>>",
	"&>",
	">>",
	"<<-",
	"<<<",
	"<<",
	"<&",
	">&",
	">|",
	"<>",
	">",
	"<",
}

local Keywords = {
	["if"] = true,
	["then"] = true,
	["else"] = true,
	["elif"] = true,
	["fi"] = true,
	["for"] = true,
	["while"] = true,
	["until"] = true,
	["do"] = true,
	["done"] = true,
	["case"] = true,
	["esac"] = true,
	["in"] = true,
	["function"] = true,
	["select"] = true,
	["time"] = true,
	["coproc"] = true,
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
	for _, Operator in ipairs(Redirections) do
		if Tokenizer:Match(Operator) then
			Tokenizer:NewToken("Redirection", Operator)
			Tokenizer:Advance(#Operator)
			return true
		end
	end

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
	local Parts = {}

	Tokenizer:Advance(1)

	while #Tokenizer.Character > 0 do
		if Tokenizer.Character == "\\" and Quote == '"' then
			local NextCharacter = Tokenizer:Peek()

			if NextCharacter == '"' or NextCharacter == "\\" or NextCharacter == "$" or NextCharacter == "`" then
				table.insert(Parts, NextCharacter)
				Tokenizer:Advance(2)
			else
				table.insert(Parts, "\\")
				Tokenizer:Advance(1)
			end
		elseif Tokenizer.Character == Quote then
			Tokenizer:NewToken("String", table.concat(Parts), Quote)
			Tokenizer:Advance(1)
			return
		else
			table.insert(Parts, Tokenizer.Character)
			Tokenizer:Advance(1)
		end
	end

	Logger:Error("Unterminated quoted string")
end

function Tokenizer:ReadIdent()
	local Parts = {}
	local HadEscape = false

	while #Tokenizer.Character > 0 do
		if Whitespace[Tokenizer.Character] or Tokenizer.Character == "\n" then
			break
		end

		local IsOperator = false

		for _, Operator in ipairs(Operators) do
			if Tokenizer:Match(Operator) then
				IsOperator = true
				break
			end
		end

		for _, Operator in ipairs(Redirections) do
			if Tokenizer:Match(Operator) then
				IsOperator = true
				break
			end
		end

		if IsOperator or Tokenizer.Character == "'" or Tokenizer.Character == '"' then
			break
		end

		if Tokenizer.Character == "\\" then
			HadEscape = true

			local NextCharacter = Tokenizer:Peek()

			if NextCharacter == "" then
				Logger:Error("Unexpected end of input after escape")
			end

			table.insert(Parts, NextCharacter)
			Tokenizer:Advance(2)
		else
			table.insert(Parts, Tokenizer.Character)
			Tokenizer:Advance(1)
		end
	end

	if #Parts == 0 then
		Logger:Error("Unexpected character: %s", Tokenizer.Character)
	end

	local Value = table.concat(Parts)

	if not HadEscape and Value:sub(1, 1) == "$" then
		Tokenizer:NewToken("Variable", Value:sub(2))
	elseif not HadEscape and Value:match("^%d+$") and (Tokenizer:Match(">") or Tokenizer:Match("<")) then
		Tokenizer:NewToken("Number", Value)
	elseif not HadEscape and Keywords[Value] then
		Tokenizer:NewToken("Keyword", Value)
	else
		Tokenizer:NewToken("Ident", Value)
	end
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
	local Result = {}
	local Index = 1
	local Quote = nil

	while Index <= #BashCode do
		local Character = BashCode:sub(Index, Index)
		local NextCharacter = BashCode:sub(Index + 1, Index + 1)

		if Character == "\\" and Quote ~= "'" then
			if NextCharacter == "\n" then
				Index = Index + 2
			elseif NextCharacter == "\r" and BashCode:sub(Index + 2, Index + 2) == "\n" then
				Index = Index + 3
			else
				table.insert(Result, Character)
				table.insert(Result, NextCharacter)
				Index = Index + 2
			end
		elseif Character == "'" or Character == '"' then
			if not Quote then
				Quote = Character
			elseif Quote == Character then
				Quote = nil
			end

			table.insert(Result, Character)
			Index = Index + 1
		else
			table.insert(Result, Character)
			Index = Index + 1
		end
	end

	return table.concat(Result)
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
	Command.Redirections = {}

	while Parser.Token and Parser.Token.Kind ~= "Newline" do
		if Parser.Token.Kind == "Keyword" then
			Parser.Token.Kind = "Ident"
		elseif Parser.Token.Kind == "Operator" or Parser.Token.Kind == "Newline" or Parser.Token.Kind == "Keyword" then
			break
		elseif Parser.Token.Kind == "Number" then
			local Number = Parser.Token.Value
			Parser:Advance(1)

			if not Parser.Token or Parser.Token.Kind ~= "Redirection" then
				Parser:Advance(-1)
				Parser.Token.Kind = "Ident"
				table.insert(Command.Arguments, Parser.Token)
				goto continue
			end

			local Operator = Parser.Token.Value
			Parser:Advance(1)

			if not Parser.Token then
				Logger:Error("Expected redirection target")
			end

			table.insert(Command.Redirections, {
				Kind = "Redirection",
				Fd = Number,
				Operator = Operator,
				Target = Parser.Token,
			})

			Parser:Advance(1)
		elseif Parser.Token.Kind == "Redirection" then
			local Operator = Parser.Token.Value
			Parser:Advance(1)

			if not Parser.Token then
				Logger:Error("Expected redirection target")
			end

			table.insert(Command.Redirections, {
				Kind = "Redirection",
				Fd = nil,
				Operator = Operator,
				Target = Parser.Token,
			})

			Parser:Advance(1)
		end

		table.insert(Command.Arguments, Parser.Token)
		Parser:Advance(1)
		::continue::
	end

	Parser.LastCommand = Command

	return Command
end

function Parser:ParseGroup()
	local Open = Parser.Token.Value
	local Close = Open == "(" and ")" or "}"

	local Group = {
		Kind = Open == "(" and "Subshell" or "Group",
		Commands = {},
	}

	Parser:Advance(1)

	while Parser.Token do
		if Parser.Token.Value == Close then
			Parser:Advance(1)
			return Group
		end

		if Parser.Token.Kind == "Newline" or Parser.Token.Value == ";" then
			Parser:Advance(1)
		else
			local Command = Parser:ParseStatement()
			table.insert(Group.Commands, Command)
		end
	end

	Logger:Error("Expected '%s' to close group", Close)
end

function Parser:ParseOperator(Left)
	local Operator = Parser.Token.Value

	local NameTable = {
		["&&"] = "And",
		["||"] = "Or",
		["|"] = "Pipe",
		["|&"] = "PipeBoth",
	}

	if Operator == ";" or Operator == "\n" or Operator == "&" then
		if Operator == "&" then
			Left.Background = true
		end

		Parser:Advance(1)
		return Left
	end

	if Operator == "!" then
		Parser:Advance(1)

		local Command = Parser:ParseStatement()
		return {
			Kind = "Negate",
			Operand = Command,
		}
	end

	if NameTable[Operator] then
		Parser:Advance(1)

		if not Parser.Token then
			Logger:Error("Expected command after '%s'", Operator)
		end

		local Right = Parser:ParseStatement()
		return {
			Kind = NameTable[Operator],
			Left = Left,
			Right = Right,
		}
	end

	return Left
end

function Parser:ParseStatement()
	if not Parser.Token then
		return nil
	end

	local Statement

	if Parser.Token.Value == "(" or Parser.Token.Value == "{" then
		Statement = Parser:ParseGroup()
	elseif Parser.Token.Kind == "Ident" or Parser.Token.Kind == "Variable" then
		Statement = Parser:ParseCommand()
	elseif Parser.Token.Kind == "Keyword" then
		Logger:Error("Keyword '%s' is not implemented yet", Parser.Token.Value)
	else
		Logger:Error("No handler for token %s", Parser.Token.Kind)
	end

	if Parser.Token and Parser.Token.Kind == "Operator" then
		Statement = Parser:ParseOperator(Statement)
	end

	return Statement
end

function Parser:Parse()
	State.Ast = {}
	Parser.Index = 1
	Parser.Token = {}
	Parser:Advance(0)

	while Parser.Token do
		local Statemenet = Parser:ParseStatement()
		if Statemenet then
			table.insert(State.Ast, Statemenet)
		end
	end
end

function LuaGenerator:PutFormatted(...)
	State.LuaCode = State.LuaCode .. string.format(...)
end

function LuaGenerator:GenerateCommand(Ast)
	local Arguments = {}
	for _, Token in ipairs(Ast.Arguments) do
		local Value = Token.Value
		if Token.Quote then
			Value = Token.Quote .. Value .. Token.Quote
		end
		table.insert(Arguments, Value)
	end

	local Command = table.concat(Arguments, " ")
	LuaGenerator:PutFormatted(
		[[
os.execute(%q)
]],
		Command
	)
end

function LuaGenerator:GenerateOperator(Ast) end

function LuaGenerator:GenerateLua(Ast)
	if Ast.Kind == "Command" then
		LuaGenerator:GenerateCommand(Ast)
	elseif Ast.Kind == "Operator" then
		LuaGenerator:GenerateOperator(Ast)
	end
end

function LuaGenerator:Generate()
	State.LuaCode = ""

	for _, Ast in ipairs(State.Ast) do
		LuaGenerator:GenerateLua(Ast)
	end
end

function Crosh:CompileBash(BashCode)
	State.BashCode = BashCode

	Tokenizer:Tokenize()
	-- Logger:PrintTable(State.Tokens)

	Parser:Parse()
	Logger:PrintTable(State.Ast)

	LuaGenerator:Generate()
	print(State.LuaCode)

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
