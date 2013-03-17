-- Lua startup file (MUD-wide) for SMAUG Fuss 
--   Written by Nick Gammon
--   15th July 2007

-- See: http://www.gammon.com.au/forum/?id=8027

--     www.gammon.com.au

os.setlocale ("", "time")

-- get directory, file names
systeminfo = mud.system_info ()

-- we will require from Lua directory
package.path = systeminfo.LUA_DIR .. "?.lua"

require "tprint"
require "serialize"
require "utilities"

-- fudging so this version tprint will work
send_nocr = io.write
send = print

-- called from comm.c when MUD is starting (just before game_loop)
function starting_up ()
  print ("MUD startup - Lua extension called")
end -- starting_up

-- called from comm.c when MUD is closing down (just after game_loop)
function shutting_down ()
  print ("MUD shutdown - Lua extension called")
end -- shutting_down

-- function called when a mob is killed (from handler.c)
function killed_mob (who, what)
   print (string.format ("%s just killed %i", who, what))
end -- killed_mob

reset_area = require ("resets").reset_area  -- area resets

print ("Mud-wide Lua module started up.")