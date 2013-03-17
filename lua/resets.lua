-- Lua reset handling for SMAUG FUSS
--   Written by Nick Gammon
--   15th July 2007

-- See: http://www.gammon.com.au/forum/?id=8027

--     www.gammon.com.au

module (..., package.seeall)

-- debugging displays
SHOW_INVOKED_MOBS = false
SHOW_INVOKED_OBJECTS = false
SHOW_AREA_RESET = false

-- debugging display to show we have made a mob
function debug_show_mob_invoked (mob, room)
  if SHOW_INVOKED_MOBS then
    print (string.format ("Invoked mob %i (%s) in room %i (%s)", 
            mob, (mud.mob_name (mob)),
            room, (mud.room_name (room))))
  end -- if SHOW_INVOKED_MOBS
end -- debug_show_mob_invoked

-- debugging display to show we have made an object
function debug_show_object_invoked (obj, room)
  if SHOW_INVOKED_OBJECTS then
    print (string.format ("Invoked object %i (%s) in room %i (%s)", 
            obj, (mud.object_name (obj)),
            room, (mud.room_name (room))))
  end -- if SHOW_INVOKED_OBJECTS
end -- debug_show_object_invoked

-- invoke up to 'count' mobs, so we have no more than 'max' mobs in the area
function minv (mob, room, count, max, ...)
  local current = mud.mobinarea (mob, room) or 0  -- present count 
  local most = (max or 1) - current           -- how many we can have
  
  -- invoke the lot of them
  for i = 1, math.min ((count or 1), most) do
    debug_show_mob_invoked (mob, room)
    mud.minvoke (mob, room, ...)
  end -- invoking each one
  
end -- minv

-- invoke so we have 'count' objects in the nominated room
function oinv (obj, room, count)
  local current = mud.objinroom (obj, room) or 0  -- present count 
  local needed = (count or 1) - current                  -- how many we need
  
  -- invoke the lot of them
  for i = 1, needed do
    debug_show_object_invoked (obj, room)
    mud.obj_to_room (room, obj)  -- note different argument order
  end -- invoking each one
  
end -- oinv

-- put a mob in each room in room_list
function multiple_mobs (vnum, room_list, ...)
  for k, v in ipairs (room_list) do
     minv (vnum, v, ...)
  end -- for
end -- multiple_mobs

-- put an object in each room in room_list
function multiple_objects (vnum, room_list)
  for k, v in ipairs (room_list) do
     oinv (vnum, v)
  end -- for
end -- multiple_objects

-- reset area newacad.are
function reset_new_academy ()

  -- put the fountains in
  multiple_objects (10401,  -- fountain
    { 10300, 10305, 10319, 10325, 10326, 10327, 10328, 
      10329, 10330, 10364, 10366, 10374, 10382, 10386, 
      10401, 
     } )
     
  -- close various exits
  
  mud.set_exit_state (10300, "s", "c")  -- close door -- Darkhaven Academy
  mud.set_exit_state (10309, "n", "c")  -- close door -- Wandering Darkhaven Academy
  mud.set_exit_state (10310, "s", "c")  -- close door -- An Enclosed Room
  mud.set_exit_state (10312, "w", "c")  -- close door -- Lost in Darkhaven Academy 
  mud.set_exit_state (10313, "e", "c")  -- close door -- A Secret Alcove 
  mud.set_exit_state (10361, "u", "c")  -- close door -- A Final Test 
  mud.set_exit_state (10362, "d", "c")  -- close door -- In the middle of the eastern wall
  mud.set_exit_state (10382, "n", "c")  -- close door -- North-West Corner
  mud.set_exit_state (10386, "d", "c")  -- close door -- South-West Corner
  mud.set_exit_state (10392, "u", "c")  -- close door -- In a Humid Cavern


  -- main helper mobs

  minv (10399, 10300)  -- Tsythia Mistress Headmistress 
  minv (10340, 10303)  -- Domick Dom teacher 
  minv (10333, 10305)  -- healer spectre cage 
  minv (10394, 10306)  -- Abbigayle language teacher
  minv (10342, 10319)  -- Toric lord healer
  minv (10346, 10355)  -- Silvina receptionist woman
  minv (10343, 10357, 1, 1, -- Zurg sage storekeeper 
          {},   -- nothing equipped
          -- shop inventory
          { 10316, -- torch
            10317, -- dried rabbit meat
            10314, -- dragonskin skin water 
          } 
          ) 

  -- initial mob we are sent to kill
  minv (10345, 10308, 1, 1, { neck1 = 10345 } )  -- chadoyn cage, equip: Chadoyn bindings 
          
  -- assorted statues for each class
  
  minv (10352, 10318)  -- statue warrior cage
  minv (10354, 10320)  -- statue druid cage
  minv (10353, 10321)  -- statue vampire cage
  minv (10351, 10322)  -- statue thief cage 
  minv (10349, 10323)  -- statue mage cage 
  minv (10350, 10324)  -- statue cleric cage
  minv (10355, 10391)  -- statue ranger cage 
  minv (10400, 10400)  -- statue Augurer cage  
  
  -- healers for each class
  
  minv (10334, 10325)  -- healer spectre cage --> Chamber of Trials for Warriors Room
  minv (10335, 10326)  -- healer spectre cage --> Chamber of Trials for Druids Room
  minv (10336, 10327)  -- healer spectre cage --> Chamber of Trials for Vampires Room
  minv (10337, 10328)  -- healer spectre cage --> Chamber of Trials for Thieves Room
  minv (10338, 10329)  -- healer spectre cage --> Chamber of Trials for Mages Room
  minv (10339, 10330)  -- healer spectre cage --> Chamber of Trials for Clerics Room
  minv (10333, 10305)  -- healer spectre cage --> Chamber of Trials for Rangers
  minv (10401, 10401)  -- healer spectre cage --> Chamber of Trials for Augurers Room
  
  -- standard cage mob equipment
  
  local west_equip  = { neck1 = 10306,  -- collar metal academy
                        hands = 10319 } -- gloves kid academy
                        
  local north_equip = { waist = 10302,  -- belt leather academy
                        legs = 10303 }  -- leggings mail academy 
                        
  local south_equip = { head = 10304,   --  helmet iron academy 
                        about = 10305 } -- cape brocade academy
                        
  local east_equip  = { wrist1 = 10300, -- bracelet charm academy 
                        feet = 10301 }  -- combat boots academy
  
  -- Clerics cage mobs (2 per room)
  minv (10326, 10331, 2, 2, west_equip)   -- minotaur cage (west)
  minv (10305, 10332, 2, 2, north_equip)  -- naga cage     (north)
  minv (10312, 10333, 2, 2, south_equip)  -- gnoll cage    (south)
  minv (10319, 10334, 2, 2, east_equip)   -- kobold cage   (east)
  
  -- Mages cage mobs (2 per room)
  minv (10327, 10337, 2, 2, west_equip)   -- minotaur cage (west)
  minv (10306, 10335, 2, 2, north_equip)  -- naga cage     (north)
  minv (10313, 10336, 2, 2, south_equip)  -- gnoll cage    (south)
  minv (10320, 10338, 2, 2, east_equip)   -- kobold cage   (east)

  -- Warriors cage mobs (2 per room)
  minv (10328, 10341, 2, 2, west_equip)   -- minotaur cage (west)
  minv (10307, 10339, 2, 2, north_equip)  -- naga cage     (north)
  minv (10314, 10340, 2, 2, south_equip)  -- gnoll cage    (south)
  minv (10321, 10342, 2, 2, east_equip)   -- kobold cage   (east)

  -- Druids cage mobs (2 per room)
  minv (10329, 10345, 2, 2, west_equip)   -- minotaur cage (west)
  minv (10308, 10343, 2, 2, north_equip)  -- naga cage     (north)
  minv (10315, 10344, 2, 2, south_equip)  -- gnoll cage    (south)
  minv (10322, 10346, 2, 2, east_equip)   -- kobold cage   (east)

  -- Vampires cage mobs (2 per room)
  minv (10330, 10349, 2, 2, west_equip)   -- minotaur cage (west)
  minv (10309, 10347, 2, 2, north_equip)  -- naga cage     (north)
  minv (10316, 10348, 2, 2, south_equip)  -- gnoll cage    (south)
  minv (10323, 10350, 2, 2, east_equip)   -- kobold cage   (east)

  -- Thieves cage mobs (2 per room)
  minv (10331, 10353, 2, 2, west_equip)   -- minotaur cage (west)
  minv (10310, 10351, 2, 2, north_equip)  -- naga cage     (north)
  minv (10317, 10352, 2, 2, south_equip)  -- gnoll cage    (south)
  minv (10324, 10354, 2, 2, east_equip)   -- kobold cage   (east)

 -- Rangers cage mobs (2 per room)
  minv (10332, 10390, 2, 2, west_equip)   -- minotaur cage (west)
  minv (10311, 10389, 2, 2, north_equip)  -- naga cage     (north)
  minv (10318, 10387, 2, 2, south_equip)  -- gnoll cage    (south)
  minv (10325, 10388, 2, 2, east_equip)   -- kobold cage   (east)

  -- Augurers cage mobs (2 per room)
  minv (10405, 10405, 2, 2, west_equip)   -- minotaur cage (west)
  minv (10402, 10402, 2, 2, north_equip)  -- naga cage     (north)
  minv (10404, 10404, 2, 2, south_equip)  -- gnoll cage    (south)
  minv (10403, 10403, 2, 2, east_equip)   -- kobold cage   (east)

  
  -- Arena mobs - we invoke 1 or 2 at a time, up to a maximum of 4 alive
  
  multiple_mobs (10301, { 10365, 10367, 10368, 10375 }, 1, 4)  -- slug arena 
  multiple_mobs (10303, { 10363, 10370, 10377, 10381 }, 1, 4)  -- crawler arena 
  multiple_mobs (10300, { 10365, 10373, 10374, 10379 }, 1, 4)  -- wolf arena
  multiple_mobs (10302, { 10370, 10372 }, 2, 4)  -- naga arena
  multiple_mobs (10396, { 10395, 10396 }, 2, 4)  -- naga spirit arena
  minv (10397, 10397, 1, 1, { wield = 10318 } )  -- queen naga asp --> wielding 'snakebite'

  -- Lower tunnels
  
  minv (10415, 10407)  -- rat grey arena   --> A dark tunnel below the Battlegrounds
  minv (10409, 10408)  -- ant fire arena 
  minv (10418, 10409)  -- mole arena 
  minv (10417, 10411)  -- rat grey arena 
  minv (10411, 10412)  -- centipede arena 
  minv (10406, 10413)  -- fire ant arena   --> A hollow cavern deep underground
  minv (10416, 10414)  -- rat grey arena 
  minv (10412, 10415)  -- centipede arena 
  minv (10407, 10418)  -- ant fire arena 
  minv (10413, 10421)  -- centipede arena 
  minv (10420, 10422)  -- mole arena 
  minv (10410, 10423)  -- centipede arena
  minv (10408, 10424)  -- ant fire arena 
  minv (10414, 10425)  -- rat grey arena 
  minv (10419, 10426)  -- mole arena 
  minv (10421, 10427)  -- mole arena 
  
  -- New player advancers
  
  minv (10497, 10499)  -- hvak1 advancer   --> Chamber of the Watch Room
  minv (10498, 10499)  -- hvak2 advancer 
  
end -- reset_new_academy

-- table of areas we know how to reset (filename/function pairs)
available_resets = {
  --["newacad.are"] = reset_new_academy
  } -- end of areas we reset
  
-- called from reset.c to reset an area - argument is area file name
function reset_area (name)
  
  local f = available_resets [name]
  
  if f then
  
    if SHOW_AREA_RESET then
      mud.msg_game ("Lua - resetting area " .. name, at.reset)
      print ("Doing Lua reset of area", name)
    end -- if debug display
    
    f ()  -- call function to reset area
    return true  -- handled it
  end -- area found
   
  return false
  
end -- reset_area
