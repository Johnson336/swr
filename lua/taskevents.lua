-- taskevents.lua

-- Event handlers for the task system

--[[

  Event handlers are called from MakeEventHandler.

  They have a standard set of arguments:  
  
     task = main task (in all_tasks) (ie. task name, description)
     tc   = task for the character (in current_tasks)
     st = current subtask (in all_tasks)  (ie. subtask type, count)
     cst = character's subtask (ie. count, done flag)
     args = the arguments to the event (eg. vnum)

  
  Thus, for each current event, each subtask is visited, and if not done
  already, this event handler is called, being passed the whole task, the
  current subtask (from the fixed all_tasks table), plus the information
  from the player's subtask table, showing how far s/he is through it.
  
  Finally, the argument(s) to the event (eg. room, object vnum) is passed.

--]]

-- generic "update the count" function
-- reports new count, sets done if limit reached
function UpdateCount (task, tc, st, cst, count)

  cst.count = count  -- update player's task count
  if cst.count >= st.count then
    cst.done = true
  end -- if now complete
  fsend ("\n&YTask: %s --> %s: %i/%i", 
        task.name,
        st.description, 
        count, st.count)

end -- function UpdateCount

-- generic code to handle buying, equipping, removing, repairing objects
function GenericItemHandler (task, tc, st, cst, vnum, amount)

  -- check right item
  if st.item == vnum then
    local count = math.min (st.count, (cst.count or 0) + (amount or 1))  -- new count
    UpdateCount (task, tc, st, cst, count)  -- apply new count
  end -- right vnum

end -- GenericItemHandler

function event_possess (task, tc, st, cst, vnum, amount)

  -- I would like to check the vnum for efficiency, but if they buy a batch then
  -- they receive the bag (so we get the bag's vnum) and inside the bag are the items
  
  UpdateCount (task, tc, st, cst, mud.possessvnum (st.item, true))  -- apply new count

end -- event_possess

-- a mob was killed - was this a quest target?
function event_killed_mob (task, tc, st, cst, vnum)

  if st.vnum [vnum] then
    UpdateCount (task, tc, st, cst, (cst.count or 0) + 1)
  end -- right vnum

end -- event_killed_mob 


-- a room was entered - was this a quest target?
function event_entered_room (task, tc, st, cst, vnum)

  if st.vnum [vnum] then
     cst.done = true
  end -- right vnum

end -- event_entered_room 


function event_act (task, tc, st, cst, what)
  fsend ("act event: %s", what)
end -- event_act

-- we bribed a mob
function event_bribe (task, tc, st, cst, vnum, amount)
  if st.vnum [vnum] then
     if amount >= st.count then
       cst.done = true
     else
       mud.force (vnum, "mutter " .. mud.char_name ())
     end -- if
  end -- right vnum
  
end -- event_bribe

-- we gave something to a mob
function event_give (task, tc, st, cst, who, obj)

  -- check right mob(s) and right item
  if st.vnum [who] and st.item == obj then
    UpdateCount (task, tc, st, cst, (cst.count or 0) + 1)
  end -- right vnum
  
end -- event_give


