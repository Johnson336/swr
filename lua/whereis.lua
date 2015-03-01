-- whereis.lua


--[[

Whereis module - path finding
----------------------------

Author: Nick Gammon
Date:   7th July 2007

--]]

module (..., package.seeall)

-- Path finding algorithm, inspired by Ked and Indoum
--
--  28 August 2006

-- See: http://www.gammon.com.au/forum/bbshowpost.php?id=7306

-- loader for modules to load sub-modules, in the module context
function my_loader (name)
  setfenv (assert (loadfile (
           string.format (systeminfo.LUA_DIR .. "%s.lua", name))), _M) ()
end -- my_loader

-- file listing available places they can go (grouped by vnum range)
my_loader "whereis_destinations"

-- increase to allow longer paths
-- decrease in case path finding is taking up too much CPU
MAX_PARTICLES = 150

local function make_particle(curr_loc, path)
  particle_count = particle_count + 1
  local path = path or {}
  return {current_room=curr_loc, path=path}
end

-- we want a copy of the table, not just a copy of the pointer
local function dup_table (t)
local new_t = {}
  table.foreachi (t, 
     function (k, v) 
       table.insert (new_t, v) 
     end )
  return new_t
end -- dup_table

local function find_path (start, destination)

  particle_count = 0

  local explored_rooms, particles = {}, {}
  
  -- create particle for the initial room
  table.insert (particles, make_particle (start))
    
  while #particles > 0 do
  
    -- give birth to new lot of particles, based on where the existing ones lead
    new_generation = {}
    
    -- consider each active particle, see where it goes
    for i, part in ipairs(particles) do
    
      -- where this one can go (gives birth to one per exit)
      local exits = mud.room_exits (part.current_room, 4)  -- except locked ones 
      
      -- if room doesn't exist, forget it
      if exits then
       
        -- make one per possible exit
        for dir, dest in pairs(exits) do
                
          -- if we have been there before, drop it, don't need to reconsider
          if not explored_rooms[dest] then
              explored_rooms[dest] = true
              new_path = dup_table (part.path)
            table.insert(new_path, dir)
            
            -- if the destination is where we want to go, we now know how to get there
            if dest == destination then
              return new_path
            end -- found it!

            -- make a new particle in the new room
            table.insert(new_generation, make_particle(dest, new_path))
            
           end   -- not explored
        end  -- for
        
        -- bail out in case they are using up too much CPU
        if particle_count > MAX_PARTICLES then
          return nil
        end -- if too many
        
      end -- if room exists in table
      
    end -- doing each existing particle
    particles = new_generation
  end -- looping until path found

end -- function find_path


-- compound directions have to go in brackets for MUSHclient, and for clarity
local directions_fixup = {
  ne = "(ne)", 
  nw = "(nw)", 
  se = "(se)", 
  sw = "(sw)",
  }
  
-- work out the route from here to vnum
local function show_route (vnum)

  -- sanity check, otherwise it shows something like: w, e
  if mud.room () == vnum then
    send ("&YYou are already there!")
    return
  end -- in destination room
  
  -- generate the path
  local path = find_path (mud.room (), vnum)
  
  -- not found? oops
  if not path then
    if particle_count > MAX_PARTICLES then
      send ("&YThat room is too far away - try walking to somewhere more central first.")
    else
      send ("&YNo path to that room was found.")
    end
    return
  end  -- not found
  
  -- now convert from something like: u, n, e, s, s, w, w, ne
  -- to: u n e 2s 2w (ne)
  
  -- add a dummy entry, to force the last item out
  table.insert (path, false)  -- end of path marker
  
  local fixed_path = {}
  local old_direction
  local count = 0
  
  -- this groups multiple consecutive moves in the same direction
  for _, p in ipairs (path) do
    -- output previous batch
    if old_direction and p ~= old_direction then    
      if count > 1 then
        table.insert (fixed_path, 
                      string.format ("%i%s", 
                      count, 
                      directions_fixup [old_direction] or old_direction))
      else  
        table.insert (fixed_path, directions_fixup [old_direction] or old_direction)
      end  
      count = 0
    end -- changed direction
    old_direction = p
    count = count + 1
  end -- for
  
  -- now show speedwalk string
  fsend ("&YTo get to %s walk: %s",
         mud.room_name (vnum),
         table.concat (fixed_path, " "))
  
  -- debugging
--  fsend ("&WMade %i particles to calculate that.", particle_count)
         
end -- show_route

function have_destinations (room)
  for _, v in ipairs (known_destinations) do
     if room >= v.first_room and room <= v.last_room then
       return v.available_dests
     end
  end -- for
end -- have_destinations

-- command handler
function whereis (line)
   
  local room = mud.room ()  -- current room
  local zone_set = have_destinations (room) -- which zone they are in
 
  -- don't bother if they aren't in the room range
  if not zone_set then
    send ("&YThere are no directions available from your current location.")
    return
  end -- not in list
  
  -- parse what they entered
  local dest = string.match (line, "([%a %-%_]+)")

  -- process their input
  if dest then
    vnum = zone_set [string.lower (trim (dest))]
    if vnum then
      show_route (vnum)
    else
      fsend ([[
&WThe destination '%s' is not on the list. Type '&Ywhereis&W' to see 
all available destinations.
]], dest)
      return  
    end -- destination given
  else
    -- better sort into alphabetic order
    local t = {}
    for k, v in pairs (zone_set) do
      table.insert (t, k)
    end -- for
    table.sort (t)

    local example
    
    -- now show list
    fsend ("&W%15s : %s", "Destination", "Room name")
    send  (("-"):rep (55))
    for k, v in ipairs (t) do
      if not example then
        example = v
      end -- if
      fsend ("&Y%15s&W : %s", capitalize (v), mud.room_name (zone_set [v]))
    end -- for  

    if example then
      fsend (
       "\nType 'whereis [place]' to get a suggested route (eg. '&Ywhereis %s&W').",
           example)
    end -- if   
    
  end -- if 


end -- task
