local Crosh = require("CroshCompiler")
local CroshRuntime = require("CroshRuntime")
local FFI = require("ffi")

local User = "kitty"
local HostName = "crosh"
local Home = "/home/kitty"

function string.starts(String, Start)
  return string.sub(String, 1, string.len(Start)) == Start
end

local function NormalizePath(Path)
  local Buffer = FFI.new("char[4096]")

  if FFI.C.realpath(Path, Buffer) == nil then
    return Path
  end

  return FFI.string(Buffer)
end

local function RewritePath(Path)
  Path = NormalizePath(Path)

  if Path:starts(Home) then
    Path = "~" .. Path:sub(#Home + 1)
  end

  return Path
end

FFI.cdef([[
  int gethostname(char *name, size_t len);
  char *realpath(const char *path, char *resolved_path);
]])

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
