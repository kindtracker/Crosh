local Crosh = require("CroshCompiler")
local CroshRuntime = require("CroshRuntime")
local FFI = require("ffi")

local User = "kitty"
local HostName = "crosh"
local Home = "/home/kitty"

FFI.cdef([[
  char *strerror(int errnum);
  int gethostname(char *name, size_t len);
  char *realpath(const char *path, char *resolved_path);
  int chdir(const char *path);
  int setenv(const char *name, const char *value, int overwrite);
  int unsetenv(const char *name);
  int execvp(const char *file, char *const argv[]);
  char *getenv(const char *name);
]])

function string.starts(String, Start)
  return string.sub(String, 1, string.len(Start)) == Start
end

function string.split(String, Separator)
  Separator = Separator or "%s+"

  local Results = {}
  local Start = 1

  for SeparatorStart, SeparatorEnd in String:gmatch("()(" .. Separator .. ")") do
    table.insert(Results, String:sub(Start, SeparatorStart - 1))
    Start = SeparatorStart + #SeparatorEnd
  end

  table.insert(Results, String:sub(Start))

  return Results
end

local function NormalizePath(Path)
  local Buffer = FFI.new("char[4096]")

  if FFI.C.realpath(Path, Buffer) == nil then
    return Path
  end

  return FFI.string(Buffer)
end

local function ChangeDirectory(Path)
  if FFI.C.chdir(Path) ~= 0 then
    return false, FFI.string(FFI.C.strerror(FFI.errno()))
  end

  return true
end

local function GetCurrentDirectory()
  local Buffer = FFI.new("char[4096]")
  local CurrentDirectory = FFI.C.getcwd(Buffer, 4096)

  return FFI.string(CurrentDirectory)
end

local function RewritePath(Path)
  Path = NormalizePath(Path)

  if Path:starts(Home) then
    Path = "~" .. Path:sub(#Home + 1)
  end

  return Path
end

local DirectoryStack = {}

_G["Crosh"] = function(Command)
  local Arguments = Command:split("%s+")
  local Name = Arguments[1]

  if not Name then
    return false
  end

  if Name == "exit" then
    os.exit(tonumber(Arguments[2]) or 0)
  elseif Name == "cd" then
    local Path = Arguments[2] or Home

    if Path == "~" then
      Path = Home
    elseif Path:starts("~/") then
      Path = Home .. Path:sub(2)
    end

    local Success, Error = ChangeDirectory(Path)

    if not Success then
      io.stderr:write("cd: " .. tostring(Error) .. "\n")
    end
  elseif Name == "pwd" then
    print(GetCurrentDirectory())
  elseif Name == "pushd" then
    local Current = GetCurrentDirectory()
    local Path = Arguments[2] or Home

    local Success, Error = ChangeDirectory(Path)

    if Success then
      table.insert(DirectoryStack, Current)
      print(GetCurrentDirectory())
    else
      io.stderr:write("pushd: " .. tostring(Error) .. "\n")
    end
  elseif Name == "popd" then
    local Path = table.remove(DirectoryStack)

    if not Path then
      io.stderr:write("popd: directory stack empty\n")
      return false
    end

    local Success, Error = ChangeDirectory(Path)

    if not Success then
      io.stderr:write("popd: " .. tostring(Error) .. "\n")
    else
      print(GetCurrentDirectory())
    end
  elseif Name == "dirs" then
    io.write(GetCurrentDirectory())

    for i = #DirectoryStack, 1, -1 do
      io.write(" " .. DirectoryStack[i])
    end

    io.write("\n")
  elseif Name == "exec" then
    if not Arguments[2] then
      return false
    end

    local CArguments = FFI.new("char *[?]", #Arguments)
    for i = 2, #Arguments do
      CArguments[i - 2] = Arguments[i]
    end

    CArguments[#Arguments - 1] = nil

    FFI.C.execvp(Arguments[2], CArguments)
    io.stderr:write(Arguments[2] .. ": " .. FFI.string(FFI.C.strerror(FFI.errno())) .. "\n")
    os.exit(127)
  elseif Name == "eval" then
    local Source = Command:match("^%S+%s+(.*)$")

    if Source then
      local Lua = Crosh:Compile(Source)
      Lua()
    end
  elseif Name == "source" or Name == "." then
    if not Arguments[2] then
      io.stderr:write(Name .. ": filename required\n")
      return false
    end

    local FilePtr = io.open(Arguments[2], "r")
    if not FilePtr then
      io.stderr:write(Name .. ": " .. Arguments[2] .. ": cannot open file\n")
      return true
    end

    local Bash = FilePtr:read("*a")
    FilePtr:close()

    local Lua = Crosh:Compile(Bash)
    Lua()
  elseif Name == "export" then
    for i = 2, #Arguments do
      local Variable, Value = Arguments[i]:match("^([%a_][%w_]*)=(.*)$")

      if Variable then
        if FFI.C.setenv(Variable, Value, 1) ~= 0 then
          io.stderr:write("export: failed to set " .. Variable .. "\n")
        end
      end
    end
  elseif Name == "unset" then
    for i = 2, #Arguments do
      FFI.C.unsetenv(Arguments[i])
    end
  elseif Name == "echo" then
    local Start = Arguments[2] == "-n" and 3 or 2
    local Output = {}

    for i = Start, #Arguments do
      table.insert(Output, Arguments[i])
    end

    io.write(table.concat(Output, " "))

    if Start ~= 3 then
      io.write("\n")
    end
  elseif Name == "printf" then
    local Format = Arguments[2] or ""

    local Values = {}

    for i = 3, #Arguments do
      table.insert(Values, Arguments[i])
    end

    io.write(string.format(Format, unpack(Values)))
  elseif Name == "true" then
    return true
  elseif Name == "false" then
    return false
  elseif Name == "read" then
    local Input = io.read("*l")

    if Input then
      local Variable = Arguments[2] or "REPLY"

      FFI.C.setenv(Variable, Input, 1)
    end
  elseif Name == "type" then
    for i = 2, #Arguments do
      local CommandName = Arguments[i]

      if
        CommandName == "cd"
        or CommandName == "pwd"
        or CommandName == "echo"
        or CommandName == "exit"
        or CommandName == "export"
        or CommandName == "unset"
        or CommandName == "read"
        or CommandName == "pushd"
        or CommandName == "popd"
        or CommandName == "dirs"
      then
        print(CommandName .. " is a shell builtin")
      else
        print(CommandName .. ": external command")
      end
    end
  elseif Name == "help" then
    print([[
Crosh builtins:

  cd       Change directory
  pwd      Print working directory
  pushd    Push directory
  popd     Pop directory
  dirs     Display directory stack
  echo     Print text
  printf   Print formatted text
  read     Read input
  export   Export environment variables
  unset    Unset environment variables
  eval     Evaluate commands
  source   Execute a script
  exec     Execute a program
  type     Describe a command
  true     Return success
  false    Return failure
  exit     Exit Crosh
  help     Display this help
]])
  else
    return CroshRuntime.Run(Command)
  end
end

while true do
  local Buffer = FFI.new("char[256]")
  FFI.C.gethostname(Buffer, 256)

  User = os.getenv("USER")
  Home = os.getenv("HOME")
  HostName = FFI.string(Buffer)
  local Path = RewritePath(os.getenv("PWD"))

  local ShellPrompt = string.format("[%s@%s %s] ", User, HostName, Path)
  io.write(ShellPrompt)

  local Bash = io.read()
  local Lua = Crosh:Compile(Bash)
  Lua()
end
