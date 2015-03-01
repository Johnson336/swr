-- tasks.lua


--[[

Task system main processing
----------------------------

Author: Nick Gammon
Date:   6th July 2007

See:  http://www.gammon.com.au/forum/?id=8014  (task file definition)
      http://www.gammon.com.au/forum/?id=8013  (example session)
      
--]]

module (..., package.seeall)

current_tasks = {}
completed_tasks = {}
task_flags = {}

MAX_TASKS = 20

-- loader for modules to load sub-modules, in the module context
function my_loader (name)
  setfenv (assert (loadfile (
           string.format (systeminfo.LUA_DIR .. "%s.lua", name))), _M) ()
end -- my_loader

my_loader "tasklist"
my_loader "taskevents"
my_loader "taskhints"

function saving (f)
   f:write ("\n--  Current tasks\n\n")
   f:write ((serialize.save ("tasks.current_tasks", current_tasks)), "\n")
   f:write ("\n--  Completed tasks\n\n")
   f:write ((serialize.save ("tasks.completed_tasks", completed_tasks)), "\n")
   f:write ("\n--  Various other things\n\n")
   f:write ((serialize.save ("tasks.task_flags", task_flags)), "\n")
end -- saving

function find_mobs_in_room ()
   -- see what mobs are here (quest giver? quest receiver?)
   local mobs = mud.mobs_in_room ()
   mobs_in_this_room = {}
   for _, vnum in ipairs (mobs) do
     mobs_in_this_room [vnum] = true
   end -- for each mob in this room
end -- find_mobs_in_room

function list_tasks ()
  local count = 0
  local example, example2, example3
  local can_complete_here = 0
  local failed_count = 0
    
  for id, t in pairs (current_tasks) do
    local task = all_tasks [id]
    if task then
      if count == 0 then
        heading ("Current tasks")
        example = id
      end -- if first
      
      local c = ""
      if failed (id) then
        c = " (Failed)"
        if failed_count == 0 then        
          example3 = id
        end -- if first failed
        failed_count = failed_count + 1
      else
        local done, here = completed (id, true) 
        if done then
          c = " (Completed)"
          if here then
            c = c .. " *"
            if can_complete_here == 0 then
              example2 = id
            end -- if first can complete
            can_complete_here = can_complete_here + 1
          end -- completable in this room
        end -- if completed
      end -- if failed/completed
      fsend ("[%-10s] %s%s", id, task.name, c)
      count = count + 1
    end -- task still exists
  end -- for all tasks

  if can_complete_here > 0 then
    send ((" "):rep (13), "* = can finish this task here")    
  end -- can complete some
  
  if count == 0 then
    send ("&YNo current tasks.")
  else
    send ("&G")  
    send (  "Type: 'task show    [id]' to show task details " ..
          "(eg. 'task show " .. example .. "')")
    if can_complete_here > 0 then
      send ("Type: 'task finish  [id]' to finish a task " ..
            "(eg. 'task finish " .. example2 .. "')")
   
    end -- if can finish some here
    if failed_count > 0 then
      send ("Type: 'task abandon [id]' to abandon a task " ..
            "(eg. 'task abandon " .. example3 .. "')")
   
    end -- if some have failed
  end -- if

end -- function list_tasks

-- see if this task is available
function available (id)
  if current_tasks [id] or 
     completed_tasks [id] then
     return false
  end -- current or already done
  
  local task = all_tasks [id]
  if not task then
    return false
  end -- if task doesn'task exist 
  
  if  task.min_level and char_info.top_level < task.min_level then
    return false
  end -- if too low level

  if task.max_level and char_info.top_level > task.max_level then
    return false
  end -- if too high level

  if not mobs_in_this_room [task.giver] then
    return false
  end -- quest giver not here
  
  -- function will test stuff like prerequisites
  if task.available then
    if task.available () ~= false then
      return task.giver
    else
      return false
    end -- ok by this function
  end -- available test
  
  -- otherwise, looks OK
  return task.giver
end -- available

-- see if this task can be completed here
-- if any_room is true, shows completed even if not at quest-giver

function completed (id, any_room)
local tc = current_tasks [id]  -- character's task

  if tc then
 
    if tc.timeout and os.time () >= tc.timeout then
      tc.failed = true
    end -- quest timed out

    -- need to know quest receiver
    local task = all_tasks [id]
    
    if not task then
      return false
    end -- if task doesn'task exist
  
    -- see if this one can be completed here
    -- quest receiver must be here
    if mobs_in_this_room [task.receiver] or any_room then
      -- check all subtasks completed 
      for k, st in ipairs (task.subtasks) do
        if st.type == "possess" then
          tc.subtasks [k].done = mud.possessvnum (st.item, true) >= st.count
        end -- possession required
        if not tc.subtasks [k].done then
          return false
        end -- not done
      end -- for each subtask
      return task.receiver, mobs_in_this_room [task.receiver]
   end -- quest giver here  
  end -- a current task
  
  return false
  
end -- completed
    
function failed (id)
local tc = current_tasks [id]  -- character's task

  if tc and tc.failed then
    return true
  end -- a current task
  
  return false
  
end -- failed
  
-- show available tasks
function available_tasks ()
  local count = 0
  local example 
  for id, t in pairs (all_tasks) do
    if available (id) then
      if count == 0 then
        heading ("Available tasks")
        example = id
      end -- if first
      fsend ("[%-10s] %s", id, t.name)
      count = count + 1
    end -- if available  
  end -- for all tasks
  if count == 0 then
    send ("&YNo tasks available here.")
  else
    send ("&G")  
    send ("Type: 'task show   [id]' to show task details " ..
          "(eg. 'task show " .. example .. "')")
    send ("Type: 'task accept [id]' to accept a task " ..
          "(eg. 'task accept " .. example .. "')")
  end -- if
  
end -- function available_tasks

function get_mob_name_and_room (vnum)

  local minfo = mud.mob_info (vnum)
  if not minfo then
    return "no mob " .. vnum
  end -- doesn't exist

  if mud.mobinworld (vnum) > 1 then
    return capitalize (minfo.short_descr)
  end
  
  local room = mud.room (vnum, true)  -- which room is it in?

  return capitalize (minfo.short_descr) .. " in " .. mud.room_name (room)
 
end -- get_mob_name_and_room

tasks_that_have_counts = {
  killmob = true,
  give = true,
  get = true,
  buy = true,
  wear = true,
  drop = true,
  repair = true,
  use = true,
  possess = true,
  }
  
function show_current_task (id)
  t = all_tasks [id]
  tc = current_tasks [id]  -- character's task

  fsend ("&Y\n%s", t.name)
  fsend ("&W" .. fix_description (t.description))
  
  for k, st in ipairs (t.subtasks) do
    local c = ""
    local extra = ""
    -- update possession count, in case they lost item
    if st.type == "possess" then
      tc.subtasks [k].count = mud.possessvnum (st.item, true)
      tc.subtasks [k].done = tc.subtasks [k].count >= st.count
    end -- possession required
    -- see if done (eg. mob killed)
    if tc.subtasks [k].done then
      c = " (Completed)"
    end -- if done
    -- see if failed (eg. we were supposed to keep a mob alive)
    if tc.subtasks [k].failed then
      c = " (Failed)"
    end -- if failed
    
    if tasks_that_have_counts [st.type] then
      extra = string.format (": %i/%i", tc.subtasks [k].count or 0, st.count)
    end -- if killmob
    
    fsend ("&Y  %s%s%s", st.description, extra, c)
    
  end -- for each subtask

  if tc.timeout and os.time () >= tc.timeout then
    tc.failed = true
  end -- quest timed out
  
  if tc.failed then
    fsend ("&Y\nTask has failed. Type 'task abandon %s' to abandon it.", id)
  elseif tc.timeout and os.time () < tc.timeout then
    fsend ("&Y\nTimed task: you have %s to complete, and hand in, this task.", convert_time_long (os.difftime (tc.timeout, os.time ())))  
  end -- failed quest
  
  -- in case they are wondering what to do ...
  if not tc.failed and #t.subtasks == 0 then
    fsend ("&Y (There is nothing to do except hand this task in)")
  end -- if
  
  fsend ("&W\nTask giver: %s", get_mob_name_and_room (t.giver))
  if t.giver ~= t.receiver then
    fsend ("Hand in to: %s", get_mob_name_and_room (t.receiver))
  end -- different receiver  
end -- show_current_task

function show_available_task (id)

  t = all_tasks [id]
  
  fsend ("&Y\n%s", t.name)
  fsend ("&W" .. fix_description (t.description))

  send ("&G")  
  send ("Type: 'task accept [id]' to accept a task " ..
        "(eg. 'task accept " .. id .. "')")
  
end -- show_available_task

function show_task (id)
  id = string.lower (trim (id))
  
  if id == "" then
    send ("&YEnter: task show [id]")
    send ("To see your list of current tasks, type 'task list'")
    return
  end -- no id
 
  if not all_tasks [id] then
    fsend ("&YThere is no task named '%s'.", id)
    return
  end -- task doesn't exist
  
  heading (string.format ("Task %s", id))
 
  if current_tasks [id] then
    show_current_task (id)
    return
  end
  
  if available (id) then
    show_available_task (id)
    return
  end
  
  fsend ("&YThere is no current or available task named '%s'.", id)
  
end -- show_task

function accept_task (id)
  id = string.lower (trim (id))
 
  if mud.position () ~= "standing" then
    send ("&YYou must be standing to do that.")
    return
  end -- if
  
  if id == "" then
    send ("&YEnter: task accept [id]")
    send ("To see a list of available tasks, type 'task available'")
    return
  end -- no id

  if current_tasks [id] then
    send ("&YYou are already on that task!")
    return
  end
  
  if completed_tasks [id] then
    send ("&YYou have already finished that task!")
    return
  end
  
  -- count what we are doing
  local count = 0
  for k in pairs (current_tasks) do
    count = count + 1
  end -- for
  
  -- limit them to not be ridiculous
  if count >= MAX_TASKS then
    fsend ("&YYou cannot accept more than %i tasks at once.", MAX_TASKS)
    return
  end
  
  if available (id) then
    t = all_tasks [id]  -- get task info
    f = t.accept  -- accepting function
    if f then
      if f () == false then
        return
      end -- cannot accept it (eg. inventory full)
    end -- accept function exists
    current_tasks [id] = { acquired_time = os.time () }  -- add to current tasks
    local tc = current_tasks [id]
    
    -- when it times out, if ever
    if t.time_limit then
      tc.timeout = os.time () + (t.time_limit * 60) -- time is in minutes
    end -- a time limit
    
    -- create appropriate subtasks tables
    tc.subtasks = {}
    for i = 1, #t.subtasks do
      table.insert (tc.subtasks, { done = false })
    end -- for each subtask
    fsend ("&YAccepted task: %s", t.name)
    
    -- hints on what to do now
    task_hints_accepted (t.giver, id)

    return
  end
  
  fsend ("&YThere is no available task named '%s'.", id)
  
end -- accept_task


function finish_task (id)
  id = string.lower (trim (id))
  
  if mud.position () ~= "standing" then
    send ("&YYou must be standing to do that.")
    return
  end -- if
    
  if id == "" then
    send ("&YEnter: task finish [id]")
    send ("To see your list of current tasks, type 'task list'")
    return
  end -- no id

  if completed_tasks [id] then
    send ("&YYou have already finished that task!")
    return
  end
  
  if not current_tasks [id] then
    fsend ("&YYou are not on task '%s'!", id)
    return
  end

  task = all_tasks [id]  -- get task info
  
  if not task then
    fsend ("&YThere is no task named '%s'.", id)
    return
  end -- task doesn'task exist
  
  if not completed (id, true) then
    send ("&YThat task is not completed.")
    return
  end -- for each subtask
  
  if not mobs_in_this_room [task.receiver] then
    send ("&YYou need to visit the mob that will accept your task completion.")
    return
  end -- quest giver not here
  
  -- check "possess" items in inventory and not in bags
  for k, st in ipairs (task.subtasks) do
    if st.type == "possess" then
      if mud.possessvnum (st.item) < st.count then
        local count = ""
        if st.count > 1 then
          count = string.format ("(%i) ", st.count)
        end -- more than one        
        fsend ("&YTask item '%s' %sneeds to be in your immediate inventory (not in bags).",
               mud.object_name (st.item), count)      
        return
      end -- not in top-level inventory
    end -- possession required
  end -- for each subtask
  
  f = task.complete  -- completing function
  if f then
    if f () == false then
      return
    end -- cannot complete it (eg. inventory full)
  end -- accept function exists
    
  -- delete "possess" items from inventory
  for k, st in ipairs (task.subtasks) do
    if st.type == "possess" then
      local count = ""
      if st.count > 1 then
        count = string.format ("(%i) ", st.count)
      end -- more than one        
      mud.destroy_item (st.item, st.count)
      fsend ("&YItem '%s' %sremoved from inventory.",
             mud.object_name (st.item), count)      
    end -- possession required
  end -- for each subtask
  
  fsend ("&YFinished task: %s", task.name)
 
  current_tasks [id] = nil  -- not a current task now
  completed_tasks [id] = os.time ()  -- note completed time
  
end -- finish_task

function abandon_task (id)
  id = string.lower (trim (id))
  
  if id == "" then
    send ("&YEnter: task abandon [id]")
    send ("To see your list of current tasks, type 'task list'")
    return
  end -- no id

  if completed_tasks [id] then
    send ("&YYou have already finished that task!")
    return
  end
  
  if not current_tasks [id] then
    fsend ("&YYou are not on task '%s'!", id)
    return
  end

  t = all_tasks [id]  -- get task info
  if t then  
    f = t.abandon  -- abandoning function
    if f then
      f ()
    end -- abandon function exists
  end -- task exists
      
  fsend ("&YAbandoned task: %s", t.name)
 
  current_tasks [id] = nil  -- not a current task now
  
end -- abandon_task

function task_history ()
  local count = 0
  local tbl = {}

  -- get ready to sort
  for id in pairs (completed_tasks) do
    table.insert (tbl, id)
  end -- for

  -- sort into completion time order
  table.sort (tbl, function (k1, k2)
      return completed_tasks [k1] < completed_tasks [k2]
      end -- function
      ) -- end
      
  for _, id in ipairs (tbl) do
     if all_tasks [id] then
       if count == 0 then
          heading ("Finished tasks")
        end -- if first
        
        fsend ("[%-10s] %s", id, all_tasks [id].name)
        count = count + 1  
     end -- task exists
  end -- for

  if count == 0 then
    send ("&YNo finished tasks.")
  end -- if  
end -- task_history

task_commands = {
  list = list_tasks,
  available = available_tasks,
  av = available_tasks,
  show = show_task,
  accept = accept_task,
  finish = finish_task,
  abandon = abandon_task,
  history = task_history
  }
  
-- command handler
function task (line)
 -- need to know about this player
 char_info = mud.character_info ()
 
 find_mobs_in_room ()
  
 -- parse what they entered
 cmd, arg = string.match (line, "(%a+)(.*)")
 
 -- look up handler
 f = task_commands [string.lower (cmd or "")]
 if f then
   f (arg)
 else
   send [[&Y
task list           --> list current tasks
task available      --> list available tasks
task show [id]      --> show details about a task (current or available)
task accept [id]    --> accept a task (agree to do it)
task finish [id]    --> finish a task (hand it in) 
task abandon [id]   --> abandon a task
task history        --> show all finished tasks
]]
end

end -- task

function task_xp (n)
  fsend ("&YYou gain %i xp!", n)
  mud.gain_exp (n)
end -- task_xp

function task_item (vnum, level)
  mud.oinvoke (vnum, level)
  fsend ("&YYou receive item: %s", mud.object_name (vnum))
end -- task_item

function task_gold (n)
  fsend ("&YYou receive %i gold!", n)
  mud.gain_gold (n)
end -- task_gold

function looking (vnum)
 -- need to know about this player
 char_info = mud.character_info ()

  -- we need to know if quest-giver or quest-receiver is here
  find_mobs_in_room ()

  local available_count = 0
  local completed_count = 0
  local giver_vnum, receiver_vnum, available_id, completed_id
  
  for id, t in pairs (all_tasks) do
  
    local vnum = available (id)  -- who will give the task
    
    -- see if each task is available
    if vnum then
      available_count = available_count + 1
      giver_vnum = vnum
      available_id = id
    end -- if available  
    
    vnum = completed (id)
    
    -- see if this one can be completed here
    if vnum then
      completed_count = completed_count + 1
      receiver_vnum = vnum
      completed_id = id
    end -- if completed
   
  end -- for all tasks
  
  if available_count == 1 then
    send ("&YThere is a task available here. Type 'task available' to see it.")
  elseif available_count > 1 then
    fsend ("&YThere are %i tasks available here. Type 'task available' to see them.", 
           available_count)
  end -- if
  
  if completed_count == 1 then
    send ("&YYou can finish a task here. Type 'task list' to see it.")
  elseif completed_count > 1 then
    fsend ("&YYou can finish %i tasks here. Type 'task list' to see them.", 
           completed_count)
  end -- if

  -- give hints
  if available_count > 0 then
    task_hints_available (giver_vnum, available_id)
  end -- if some tasks available

  if completed_count > 0 then
    task_hints_completable (receiver_vnum, completed_id)
  end -- if some tasks can be completed

  
end -- looking

-- called once per minute
function char_update (task, tc, st, cst)

  if not task_flags.completed_hint then
    task_hints_completed ()
  end -- see if we need to tell them what to do

  if not task_flags.whereis_hint then
    task_hints_whereis ()
  end -- see if we need to tell them how to get there

  -- check all current tasks
  for id, tc in pairs (current_tasks) do
    local task = all_tasks [id]  -- get main task info
    if task and not tc.failed then
     if tc.timeout and not tc.failed then
       
       if os.time () >= tc.timeout then
         tc.failed = true
         fsend ("\n&YTask: %s --> Time is up. (Failed)", 
                    task.name)
       else
         fsend ("\n&YTask: %s --> %s to go.", 
                      task.name,
                      convert_time_long (os.difftime (tc.timeout, os.time ())))  
       end -- quest timed out
     end -- have a timeout
   end -- not failed already
 end -- checking all current tasks
  
end -- char_update

--[[

  This generates a function to handle an event.
  
  The generated function:
  
    * checks all current tasks
    * if each one has not failed then:
      * it checks all subtasks for that task
      * if each one is not already complete then:
        * it checks the event type matches what the subtask wants (eg. "give")
        * if the correct event type, it passes it to the nominated handler (eg. event_give)
        * if the event is *now* completed we notify the player, 
          and call a completion handler
    
  We also get a chance to call a "main" function which is not specific to a particular
  task, eg. we also want to handle entering a room, or killing a mob, outside of other
  tasks.
  
--]]

function MakeEventHandler (event_type, event_func, main_func)
  return function (...)
    -- check all current tasks
    for id, tc in pairs (current_tasks) do
      local task = all_tasks [id]  -- get main task info
     
      if task and not tc.failed then
        -- pass this event to all subtasks
        for i, st in ipairs (task.subtasks) do 
          if not tc.subtasks [i].done and   -- don'task bother if subtask complete
            st.type == event_type then      -- check correct event type
            event_func (task, tc, st, tc.subtasks [i], ...)  -- handle the event
            -- if subtask done now, tell them
            if tc.subtasks [i].done then
              fsend ("&YTask: %s --> %s. (Objective complete)", 
                                task.name,
                                st.description)  
              -- custom sub-task completion function  
              if st.complete then
                st.complete ()
              end -- completion function        
              if completed (id, true) then
                fsend ("&YTask: %s (Task complete)", 
                                  task.name)                
              end -- whole task completed
            end -- is now done
          end -- not done already and is correct event type
        end -- checking each subtask
      end -- task not failed already
    end -- for checking each task
  
    -- for stuff like entering room, we may want to check for quest givers
    if main_func then
      main_func (...)
    end -- global handler
  end -- anonymous event handler function
end -- MakeEventHandler

-- insert event handlers into global table
table.insert (handlers.act, MakeEventHandler ("act", event_act))
table.insert (handlers.killed_mob, MakeEventHandler ("killmob", event_killed_mob))
table.insert (handlers.entered_room, MakeEventHandler ("visitroom", event_entered_room))
table.insert (handlers.bribe, MakeEventHandler ("bribe", event_bribe))
table.insert (handlers.give, MakeEventHandler ("give", event_give))
table.insert (handlers.got_object, MakeEventHandler ("get", GenericItemHandler))
table.insert (handlers.got_object, MakeEventHandler ("possess", event_possess))
table.insert (handlers.buy, MakeEventHandler ("buy", GenericItemHandler))
table.insert (handlers.wear, MakeEventHandler ("wear", GenericItemHandler))
table.insert (handlers.drop, MakeEventHandler ("drop", GenericItemHandler))
table.insert (handlers.repair, MakeEventHandler ("repair", GenericItemHandler))
table.insert (handlers.use, MakeEventHandler ("use", GenericItemHandler))

-- these are not specific to subtasks
table.insert (handlers.char_update, char_update)  -- check for timeouts
table.insert (handlers.looking, looking)          -- check for tasks in this room

-- standard saving routine
table.insert (handlers.saving, saving)

--[[ 

  Tasks that require you to kill mobs might list a "set" of mobs.
  Eg. you might not care exactly which sort of gnoll was killed.
  Thus, you list a table of vnums. This routine turns it from something like:
  
  vnums = { 10312, 10313, 10314 }
  
  to
  
  vnum = { [10312] = true, [10313] = true, [10314] = true }

  This permits a fast table lookup.
  
--]]

-- make vnum lookups
for _, t in pairs (all_tasks) do

  -- make empty subtasks, if task has none
  t.subtasks = t.subtasks or {}
  -- force description
  t.description = t.description or ""
  
  -- make vnum table from vnums table
  for _, st in ipairs (t.subtasks) do
    if st.vnums then
      st.vnum = {}  -- empty table
      for _, v in ipairs (st.vnums) do
        st.vnum [v] = true
      end -- each vnum
    end -- if vnum list
    
    -- ensure that subtasks have a count
    st.count = st.count or 1
  end -- each subtask
  
  -- assume task giver will receive it, unless specified otherwise
  if not t.receiver then
    t.receiver = t.giver
  end -- if no receiver, assume task giver
  
end -- for all tasks

