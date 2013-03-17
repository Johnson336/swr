-- Lua startup file (MUD-wide) for SMAUG Fuss 
--   Written by Nick Gammon
--   16th July 2007

--     www.gammon.com.au


-- -----------------------------------------------------------------------------
--         HELPER FUNCTIONS
-- -----------------------------------------------------------------------------

-- trim leading and trailing spaces from a string
function trim (s)
  return (string.gsub (s, "^%s*(.-)%s*$", "%1"))
end -- trim

-- round "up" to absolute value, so we treat negative differently
--  that is, round (-1.5) will return -2
function round (x)
  if x >= 0 then
    return math.floor (x + 0.5)
  end  -- if positive

  return math.ceil (x - 0.5)
end -- function round

function convert_time_helper (secs)

  -- handle negative numbers
  local sign = ""
  if secs < 0 then
    secs = math.abs (secs)
    sign = "-"
  end -- if negative seconds
  
  -- weeks
  if secs >= (60 * 60 * 24 * 6.5) then
    return sign .. round (secs / (60 * 60 * 24 * 7)), "w"
  end -- 6.5 or more days
  
  -- days
  if secs >= (60 * 60 * 23.5) then
    return sign .. round (secs / (60 * 60 * 24)), "d"
  end -- 23.5 or more hours
  
  -- hours
  if secs >= (60 * 59.5) then
   return sign .. round (secs / (60 * 60)), "h"
  end -- 59.5 or more minutes
  
  -- minutes
  if secs >= 59.5 then
   return sign .. round (secs / 60), "m"
  end -- 59.5 or more seconds
  
  -- seconds
  return sign .. round (secs), "s"    
end -- function convert_time_helper

-- eg. 4m
function convert_time (secs)
  local n, u = convert_time_helper (secs)
  return n .. " " .. u 
end -- function convert_time

time_units = {
  w = "week",
  d = "day",
  h = "hour",
  m = "minute",
  s = "second"
  }
  
-- eg. 4 minutes
function convert_time_long (secs)
  local n, u = convert_time_helper (secs)
  local long_units = time_units [u]
  local s = ""
  if math.abs (n) ~= 1 then
    s = "s"
  end -- if not 1
  return n .. " " .. long_units .. s
end -- function convert_time_long

function capitalize (s)
  return string.upper (string.sub (s, 1, 1)) .. string.sub (s, 2)
end -- capitalize 

-- substitutions in descriptions
function fix_description (s, a1, a2, a3, a4, a5, a6, a7, a8, a9)
  local t = {
    ["$n"] = mud.char_name (),   -- $n = name
    ["$c"] = string.lower (mud.class ()),  -- $c = class
    ["$r"] = string.lower (mud.race ()),   -- $r = race
    
    -- some way to include a $ symbol
    ["$$"] = "$",
    
    -- argument substitution
    ["$1"] = a1 or "$1",
    ["$2"] = a2 or "$2",
    ["$3"] = a3 or "$3",
    ["$4"] = a4 or "$4",
    ["$5"] = a5 or "$5",
    ["$6"] = a6 or "$6",
    ["$7"] = a7 or "$7",
    ["$8"] = a8 or "$8",
    ["$9"] = a9 or "$9",
    }
    
  return (string.gsub (s, "$[%a%d$]", t))
end -- fix_description

function heading (title, colour)
  local hyphens = math.floor ((76 - #title) / 2)
  send (colour or "&Y", ("-"):rep (hyphens), " ", title, " ", ("-"):rep (hyphens))
end -- heading

-- a line of hyphens
function hyphens ()
  send (("-"):rep (79))
end -- hyphens
