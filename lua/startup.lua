-- Lua startup file for SMAUG Fuss 

--   This is the PER-PLAYER file (one script state is made for each player)

--   Written by Nick Gammon
--   5th July 2007

--     www.gammon.com.au

os.setlocale ("", "time")

-- get directory, file names
systeminfo = mud.system_info ()

-- table of event handlers
-- each module can use table.insert to insert their own handler
handlers = {
    saving = {},        -- saving is special - we only open the file once
    reconnected = {},   -- reconnected is special - first we load player state
    }

function MakeHandler (name)
  return function (...)
    for _, f in ipairs (handlers [name]) do
      f (...)  -- call each handler
    end -- for
  end -- anonymous function
end -- MakeHandler
   
-- install handlers into handlers table AFTER this code
--  (this code below creates the handlers table)
for _, name in ipairs {
  "new_player", 
  "entered_game",
  "speech",
  "entered_room",
  "killed_mob",
  "got_object",
  "lost_object",
  "looking",
  "char_update",
  "act",
  "give",
  "bribe",
  "buy",
  "wear",
  "drop",
  "repair",
  "advance_level",
  "use",
  } do
  _G [name] = MakeHandler (name)  -- make a function handler
  handlers [name] = {}  -- make empty handlers table
end -- for loop

-- we will require from Lua directory
package.path = systeminfo.LUA_DIR .. "?.lua"

require "tprint"
require "serialize"
require "utilities"

-- -----------------------------------------------------------------------------
--        Sending functions - leave up here in case modules need them
-- -----------------------------------------------------------------------------

-- send to character, concats multiple arguments, appends cr/newline
function send (...)
  mud.send_to_char (table.concat {...} .. "\r\n")
end -- send

-- formatted send (first argument is 'string.format' style)
function fsend (s, ...)
  mud.send_to_char (string.format (s, ...) .. "\r\n")
end -- send

-- send to character, concats multiple arguments, does not append cr/newline
function send_nocr (...)
  mud.send_to_char (table.concat {...})
end -- send_nocr

-- install other stuff here (like task system)

task    = require ("tasks").task  -- module - provides  'task' command handler
whereis = require ("whereis").whereis -- module - provides 'whereis' command handler

-- standard loading/saving of player state file
function get_file_name ()
local charinfo = mud.character_info ()
  return systeminfo.PLAYER_DIR .. 
        string.lower (string.sub (charinfo.name, 1, 1)) .. 
        "/" ..
        charinfo.name ..
        ".lua"
end -- get_file_name

-- called to save a player
function saving ()
local charinfo = mud.character_info ()
local fname = get_file_name ()

   local f = assert (io.open (fname, "w"))
   f:write (os.date ("-- Saved at: %c\n\n"))
   f:write (string.format ("-- Extra save file for %s\n\n", charinfo.name))

   for _, func in ipairs (handlers.saving) do
      func (f)  -- call each handler to write to this file
   end -- for
   
   f:close ()

end -- saving

function entered_game (name)
 if name == "Cole" then
    send ("Character info ...")
    tprint (mud.character_info ())
    send ("Mud info ...")
    tprint (mud.sysdata ())
    tprint (mud.system_info ())
  end -- if Cole
end

-- after player reconnects, load his/her state file
function reconnected (name)

  local fname = get_file_name ()
  local f = loadfile (fname)
  
  if f then
    f ()  -- execute it
  else
    io.stderr:write ("Warning - no state file: " .. fname .. "\n")
  end 
   
  for _, func in ipairs (handlers.reconnected) do
    func (name)  -- call each handler to let them know
  end -- for
 
end -- reconnected

