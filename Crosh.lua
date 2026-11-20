local Crosh = require("CroshCompiler")
local ffi = require("ffi")

local User = "kitty"
local HostName = "crosh"
local Home = "/home/kitty"

function string.starts(String, Start)
  return string.sub(String, 1, string.len(Start)) == Start
end

local function RewritePath(Path)
  if Path:starts(Home) then
    Path = "~" .. Path:sub(#Home + 1)
  end
  return Path
end

ffi.cdef([[
  int gethostname(char *name, size_t len);
]])

while true do
  local Buffer = ffi.new("char[256]")
  ffi.C.gethostname(Buffer, 256)

  User = os.getenv("USER")
  Home = os.getenv("HOME")
  HostName = ffi.string(Buffer)
  local Path = RewritePath(os.getenv("PWD"))

  local ShellPrompt = string.format("[%s@%s %s] ", User, HostName, Path)
  io.write(ShellPrompt)

  local Bash = io.read()
  local Lua = Crosh:Compile(Bash)
  Lua()
end
