-- tasklist.lua


--[[

All tasks in the task system
----------------------------

Author: Nick Gammon
Date:   6th July 2007

See:  http://www.gammon.com.au/forum/?id=8014  (task file definition)
      http://www.gammon.com.au/forum/?id=8013  (example session)
      

This is a table of all tasks in the system, keyed by alphanumeric task id.


WARNING! Task ids must be lower case, as we force player input into lower case

Fields:

  name - Summary to show in task list (keep to below about 50 characters) (required)
  
  description - Multiple line description of task purpose.
  
  giver - vnum of task giver mob (required)
  
  receiver - vnum of task to receive the task hand-in
             if omitted, task accepted by task-giver
             
  min_level - minimum level you need to be to see this task
              if omitted, no minimum level
  
  max_level - maximum level you can be to see this task
              if omitted, no maximum level
              
  available - function that returns false if task is not available
              if omitted, task is always available
              you could test for completion of earlier tasks, player class, etc.
              
  accept - function called when task is accepted
           if omitted, nothing special is done   
           returns false if task can not be accepted
           (might return false if you need to put something in inventory, and it is full)          

  complete - function called when task is completed (finished / handed in)
            if omitted, nothing special is done             
            returns false if task can not be finished
            (might return false if you need to put something in inventory, and it is full)          

  abandon - function called when task is abandoned
            if omitted, nothing special is done             
            (might remove task items from inventory)
            
  time_limit - time limit in minutes - task must be completed in that time (real time)
            if omitted, no time limit             
            
  subtasks - table of subtasks which need to be completed - see below
             if omitted, there are no subtasks, and task can be handed in immediately
             subtasks are keyed by number and are shown in order
             
             
SUBTASKS table item fields
--------------------------

     description - sub task description (keep reasonably short - probably 50 characters max)
                   Task listing shows stuff like: 
                       Buy some bread: 1/1 (Completed)
                   Thus there needs to be room for a couple of leading spaces,
                   counts (eg. 20/30) and the word "(Completed)"
                   
                   
     type - type of subtask
     
     * killmob - must kill 'count' mobs in vnums table
     * visitroom - must visit one of the rooms in the vnums table
     * bribe - must bribe one of the mobs in the vnums table 'count' coins
     * give - must give 'count' objects of vnum 'item' to one of the mobs in the vnums table
     * get      - must get 'count' objects of vnum 'item' (into inventory)
     * buy      - must buy 'count' objects of vnum 'item' 
     * wear     - must wear 'count' objects of vnum 'item' 
     * drop     - must drop 'count' objects of vnum 'item' 
     * repair   - must repair 'count' objects of vnum 'item' 
     * use      - must use 'count' objects of vnum 'item'
     * possess  - must have 'count' objects of vnum 'item' in inventory
             
     count - number of items to get/buy/drop etc., count of mobs to kill, size of bribe
             if omitted, defaults to 1
             
     vnums - table of vnums of mobs (turned into vnum lookup table)
             
     item - vnum of item to get/buy/drop etc. (required, where applicable)
     
     complete - function that is called when this subtask is completed
                if omitted, nothing special is done
                
     Note: 'get' counts when you obtain the item, even if you subsequently lose it.
           'possess' counts possession of the item, giving it away before handing in will fail the subtask. "Possess" items are destroyed on task completion.
           
     
--]]

all_tasks = {}

all_tasks.academy1 = {

  -- name of task - shown in task list
  name = "Find teacher Domick",

  -- description - shown when you do "task show x"
  description = [[
Mistress Tsythia wants you to visit Domick in The Laboratory of Skills 
and Spells.

Please give him this note ('&Ygive note domick&W').

To find Domick, go North from Mistress Tsythia and then East.

If you have become lost, type '&Ywhereis skills&W' to get directions to him.

You will be rewarded with some armour, and 100 gold coins.

]],

  giver = 10399,    -- vnum of task giver: Mistress Tsythia
  
  receiver = 10340, -- vnum of task receiver: Domick
  
  min_level = 1,    -- minimum level to attain task
  
  max_level = 5,    -- maximum level to attain task
  
  time_limit = nil,  -- time limit in minutes
  
  -- is task available? (check prerequisites, class, etc.)
  available = function () 
   end,
  
    -- do stuff for accepting (like put things in inventory)
  accept = function ()
    task_item (36)  -- a note
    end,  -- end accept
  
  -- do stuff for abandoning (like removing task items)
  abandon = function ()
    end,  -- end accept

  -- do stuff for completing (like task rewards)
  complete = function ()
      task_xp (20)
      task_item (10308)  -- a plate of armour
      task_gold (100)
      send ("&WDomick tells you:")
      hyphens ()
      send ([[
To see what is in your inventory: &Yinven&W
  (or just type '&Yi&W')
To see how much gold you have:    &Ygold&W
You can now wear this armour:     &Ywear armour&W
You can see what you are wearing: &Yequip&W
If you want to remove it:         &Yremove armour&W
]])
      hyphens ()
      end, -- complete function
      
 -- table of subtasks 
 -- each subtask is its own table
 -- required fields: description / type
 
 subtasks = {  -- table of sub tasks
 
     {  -- subtask 1
     description = "Give note to Domick",
     type = "give",
     vnums = { 10340 }, 
     item = 36
     },  -- end subtask

     },   -- end subtasks table

   }   -- end of task academy1      
     


all_tasks.academy2 = {

 -- name of task - shown in task list
  name = "Visit the headmistress",

  -- description - shown when you do "task show x"
  description = [[
Domick would like you to return to Mistress Tsythia. She will have
a task for you to test your fighting skills. As a reward you will
receive a weapon suitable for starting a fight.

]],

  giver = 10340,    -- vnum of task giver: Domick
  
  receiver = 10399, -- vnum of task receiver: Mistress Tsythia
  
  min_level = 1,    -- minimum level to attain task
  
  max_level = 5,    -- maximum level to attain task
  
  time_limit = nil,  -- time limit in minutes
  
  -- it task available? (check prerequisites, class, etc.)
  available = function () 
   end,
  
    -- do stuff for accepting (like put things in inventory)
  accept = function ()
    end,  -- end accept
  
  -- do stuff for abandoning (like removing task items)
  abandon = function ()
    end,  -- end accept

  -- do stuff for completing (like task rewards)
  complete = function ()
      item_vnum = {
        thief = 10312,  -- dagger
        mage = 10312,   -- dagger
        vampire = 10312,-- dagger
        augurer = 10312,-- dagger
        cleric = 10315, -- mace
        druid = 10315,  -- mace
        warrior = 10313, -- sword
        ranger = 10313, -- sword
        paladin = 10313, -- sword
        }
        
      task_xp (20)
      
      task_item (item_vnum [string.lower (mud.class ())] or 10312)  -- appropriate weapon
      task_gold (100)
      send ("&WMistress Tsythia tells you: 'Don't forget to wear your new item'\n")
      send ("&WMistress Tsythia tells you: 'Once you have done that, I have another task for you.'\n")
      end, -- complete function
      
 -- table of subtasks 
 -- each subtask is its own table
 -- required fields: description / type
 
 subtasks = {  -- table of sub tasks

     },   -- end subtasks table

   }   -- end of task academy2      
     
   


all_tasks.academy3 = {

 -- name of task - shown in task list
  name = "The Chadoyn must die!",

  -- description - shown when you do "task show x"
  description = [[
Mistress Tsythia would like you to head north, and find the Chadoyn
to kill. Return once it is slain.

]],

  giver = 10399,    -- vnum of task giver:  Mistress Tsythia
  
  -- receiver = 10399, -- vnum of task receiver: Mistress Tsythia
  
  min_level = 1,    -- minimum level to attain task
  
  max_level = 5,    -- maximum level to attain task
  
  time_limit = nil,  -- time limit in minutes
  
  -- it task available? (check prerequisites, class, etc.)
  available = function () 
   return completed_tasks.academy2 ~= nil
   end,
  
    -- do stuff for accepting (like put things in inventory)
  accept = function ()
    end,  -- end accept
  
  -- do stuff for abandoning (like removing task items)
  abandon = function ()
    end,  -- end accept

  -- do stuff for completing (like task rewards)
  complete = function ()
      task_xp (20)
      task_item (10311)  -- an eye of enlightenment
      task_gold (500)
      end, -- complete function
      
 -- table of subtasks 
 -- each subtask is its own table
 -- required fields: description / type
 
 subtasks = {  -- table of sub tasks

    {  -- subtask 1
     description = "Chadoyn slain",
     type = "killmob",
     vnums = { 10345 }, 
     },  -- end subtask
 
     },   -- end subtasks table

   }   -- end of task academy3      
     


all_tasks.academy4 = {

 -- name of task - shown in task list
  name = "Getting your hands dirty",

  -- description - shown when you do "task show x"
  description = [[
Mistress Tsythia would like you to head for the battlegrounds, and
kill 4 wolves, 3 naga and 2 carrion crawlers.

Your reward for doing this is 1000 gold.

]],

  giver = 10399,    -- vnum of task giver:  Mistress Tsythia
  
  -- receiver = 10399, -- vnum of task receiver: Mistress Tsythia
  
  min_level = 1,    -- minimum level to attain task
  
  max_level = 5,    -- maximum level to attain task
  
  time_limit = nil,  -- time limit in minutes
  
  -- it task available? (check prerequisites, class, etc.)
  available = function () 
   return completed_tasks.academy3 ~= nil
   end,
  
    -- do stuff for accepting (like put things in inventory)
  accept = function ()
    end,  -- end accept
  
  -- do stuff for abandoning (like removing task items)
  abandon = function ()
    end,  -- end accept

  -- do stuff for completing (like task rewards)
  complete = function ()
      task_xp (5000)
      task_gold (1000)
      end, -- complete function
      
 -- table of subtasks 
 -- each subtask is its own table
 -- required fields: description / type
 
 subtasks = {  -- table of sub tasks

     {  -- subtask 1
     description = "Naga slain",
     type = "killmob",
     vnums = { 10302 }, 
     count = 3,
     },  -- end subtask
  
    {  -- subtask 2
     description = "Wolf slain",
     type = "killmob",
     vnums = { 10300 }, 
     count = 4,
     },  -- end subtask
 
      {  -- subtask 3
     description = "Carrior crawler slain",
     type = "killmob",
     vnums = { 10303 }, 
     count = 2,
     },  -- end subtask
  
     },   -- end subtasks table

   }   -- end of task academy4      
     
   
 
all_tasks.academy5 = {

 -- name of task - shown in task list
  name = "Find some food",

  -- description - shown when you do "task show x"
  description = [[
Mistress Tsythia would like you to go up and east to buy some
meat to eat for sustenance during your adventures.

You will be rewarded with a dragonskin - useful for holding water.
]],

  giver = 10399,    -- vnum of task giver:  Mistress Tsythia
  
  receiver = 21029, -- vnum of task receiver: the storekeeper
  
  min_level = 1,    -- minimum level to attain task
  
  max_level = 5,    -- maximum level to attain task
  
  time_limit = nil,  -- time limit in minutes
  
  -- it task available? (check prerequisites, class, etc.)
  available = function () 
   end,
  
    -- do stuff for accepting (like put things in inventory)
  accept = function ()
    end,  -- end accept
  
  -- do stuff for abandoning (like removing task items)
  abandon = function ()
    end,  -- end accept

  -- do stuff for completing (like task rewards)
  complete = function ()
      task_xp (2000)
      task_item (10314)
      task_gold (200)
      end, -- complete function
      
 -- table of subtasks 
 -- each subtask is its own table
 -- required fields: description / type
 
 subtasks = {  -- table of sub tasks

    {  -- subtask 1
     description = "Meat purchased",
     type = "buy",
     item = 10317,
     },  -- end subtask
  
     },   -- end subtasks table

   }   -- end of task academy5      
     

all_tasks.academy6 = {

 -- name of task - shown in task list
  name = "Bring me bread",

  -- description - shown when you do "task show x"
  description = [[
Mistress Tsythia would like you to find 3 loaves of bread and bring
them to her.
]],

  giver = 10399,    -- vnum of task giver:  Mistress Tsythia
  
--  receiver = 10399, -- vnum of task receiver: Mistress Tsythia
  
  min_level = 1,    -- minimum level to attain task
  
  max_level = 5,    -- maximum level to attain task
  
  time_limit = nil,  -- time limit in minutes
  
  -- it task available? (check prerequisites, class, etc.)
  available = function () 
   end,
  
    -- do stuff for accepting (like put things in inventory)
  accept = function ()
    end,  -- end accept
  
  -- do stuff for abandoning (like removing task items)
  abandon = function ()
    end,  -- end accept

  -- do stuff for completing (like task rewards)
  complete = function ()
      task_xp (1000)
      task_gold (200)
      end, -- complete function
      
 -- table of subtasks 
 -- each subtask is its own table
 -- required fields: description / type
 
 subtasks = {  -- table of sub tasks

    {  -- subtask 1
     description = "Bread obtained",
     type = "possess",
     item = 21021,
     count = 3,
     },  -- end subtask
  
     },   -- end subtasks table

   }   -- end of task academy6
     

   
   

all_tasks.immcommand1 = {

  -- name of task - shown in task list
  name = "Visit the Immortal Shipyard",

  -- description - shown when you do "task show x"
  description = [[
  The Immortal complex is a large and confusing place! Get
  better acquainted with it by exploring the Immortal Shipyard.
]],

  giver = 17,    -- vnum of task giver: Mistress Tsythia
  
  
  min_level = 1,    -- minimum level to attain task
  
  max_level = 105,
  
  time_limit = nil,  -- time limit in minutes
  
  -- is task available? (check prerequisites, class, etc.)
  available = function () 
   end,

   accept = function ()
     end,  -- end accept
  
  -- do stuff for abandoning (like removing task items)
  abandon = function ()
    end,  -- end accept

  -- do stuff for completing (like task rewards)
  complete = function ()
      send ("&CCongratulations, you found the Immortal Shipyard!")
      end, -- complete function
      
   -- table of subtasks 
 -- each subtask is its own table
 -- required fields: description / type
 
 subtasks = {  -- table of sub tasks
 
     {  -- subtask 1
     description = "Visit the immortal Shipyard",
     type = "visitroom",
     vnums = { 113 }
     }  -- end subtask

     }


   }   -- end of task immcommand1      
     