-- taskhints.lua


--[[

Task system hints
-----------------

Author: Nick Gammon
Date:   6th July 2007

This is called in various places to give a once-off hint to the player. Once 
the hint is given a flag is set, so they don't get the same hint again.

--]]

whereis_module = require "whereis"

function task_hints_available (vnum)

  if task_flags.available_hint then
    return
  end -- if done this already
  
  local minfo = mud.mob_info (vnum)
  if not minfo then
    return 
  end -- doesn't exist
  
  local name = capitalize (minfo.short_descr)
  
  send ("\n&W", name, " tells you:")
  hyphens ()
  send (fix_description ([[
Greetings, $n. I am a task-giver (or should that be task-master?). 
When you looked around you noticed a message about 'tasks available here'.
Completing tasks can earn you gold, experience, items, and glory!
You may consider some tasks too humble for a noble $c such as you. 
However, when you complete (and hand in) a task, that can unlock further 
tasks in a chain. Each one can lead to further rewards, and experience.
To see what you can do with tasks, type '&Ytask&W' to see a list of commands.
You can see a list of available tasks by typing '&Ytask available&W'.
Noble $r, I look forwards to your accepting my task, and having
fun completing it.]]))
  hyphens ()

  mud.force (vnum, "smile " .. mud.char_name ())
  
  task_flags.available_hint = true

end -- task_hints_available

function task_hints_accepted (vnum, id)

  if task_flags.accepted_hint then
    return
  end -- if done this already
  
  local minfo = mud.mob_info (vnum)
  if not minfo then
    return 
  end -- doesn't exist
  
  local name = capitalize (minfo.short_descr)
  
  send ("\n&W", name, " tells you:")
  hyphens ()
  send (fix_description ([[
Thank you, $n. You have accepted my task. You can now type '&Ytask list&W' 
to see a list of your current tasks (you can have more than one task 
outstanding at once). To remind yourself of the task objectives, 
type '&Ytask show $1&W'. Once all of the objectives are complete (you 
will know because you will see see "(Complete)" next to them), you can hand 
that task in for the task rewards, and possibly further tasks in that chain.]], 
  id))
  hyphens ()

  mud.force (vnum, "bow " .. mud.char_name ())
  
  task_flags.accepted_hint = true

end -- task_hints_accepted


function task_hints_completed ()
  -- need to know about this player
  char_info = mud.character_info ()

  local count = 0
  local a_completed_task
  
  for id, t in pairs (current_tasks) do
    local done, here = completed (id, true) 
    if done then 
      count = count + 1
      a_completed_task = id
    end -- this one done 
  end -- for all current tasks
  
  if count == 0 then
    return
  end  -- s/he hasn't completed anything
    
  local minfo = mud.mob_info (all_tasks [a_completed_task].receiver)
  if not minfo then
    return 
  end -- doesn't exist
  
  local name = capitalize (minfo.short_descr)
  
  send ("\n&W", name, " whispers:")
  hyphens ()
  send (fix_description ([[
Hello, $n. I notice you have completed one of your tasks.
In order to get your reward you need to find me, and finish this task 
by typing '&Ytask finish $1&W'. When you do this I will reward you
with the appropriate rewards for this task. 
I look forwards to seeing you, $n.]], 
  a_completed_task))
  hyphens ()

  task_flags.completed_hint = true


end -- task_hints_completed


function task_hints_whereis ()

  local level = mud.level ()
  local room = mud.room ()
  
  -- don't bother for high levels
  if level and level >= 10 then
   task_flags.whereis_hint = true
   return
  end -- if high level character
 
  -- don't know their room? can't help
  if not room then
    return
  end
    
  -- not in a known range? can't help
  if not whereis_module.have_destinations (room) then
    return
  end
 
  local name
  
  local minfo = mud.mob_info (10399) -- Mistress Tsythia
  if minfo then
    name = capitalize (minfo.short_descr)
  else
    name = "Someone"  -- that mob not in this game
  end -- if
  
  send ("\n&W", name, " whispers:")
  hyphens ()
  send (fix_description ([[
Hello, $n. Are you feeling lost? Not sure where everything is yet?
While you are in Darkhaven you can find important locations by typing
'&Ywhereis&W'. This will give you a list of places you might care to visit.
You can find the exact directions to walk by specifying the place you want
to go, for example '&Ywhereis repairs&W'.]]))
  hyphens ()

  task_flags.whereis_hint = true
  
end -- task_hints_whereis

function task_hints_completable (vnum, id)
  
  if task_flags.finished_hint then
    return
  end -- if done this already

  local minfo = mud.mob_info (vnum)
  if not minfo then
    return 
  end -- doesn't exist
  
  local name = capitalize (minfo.short_descr)
  
  send ("\n&W", name, " tells you:")
  hyphens ()
  send (fix_description ([[
Greetings, $n. I notice you have completed one of your tasks.
In order to get your reward you must finish (hand in) this task by 
typing '&Ytask finish $1&W'. When you do this I will reward you
for handing in this task. 
Thanks for completing the task, $n.
Once you have finished the task, try typing '&Ytask available&W', to see
if further tasks have become available (as part of a chain of tasks).]], 
  id))
  hyphens ()

  mud.force (vnum, "thank " .. mud.char_name ())
  
  task_flags.finished_hint = true

end -- task_hints_completable