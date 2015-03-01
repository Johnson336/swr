-- whereis_destinations.lua


--[[

Whereis module - table of known locations
-----------------------------------------

Author: Nick Gammon
Date:   8th July 2007

This file consists of one table: known_destinations

Inside are sub-tables, one for each "zone" that you want to give directions
around (eg. Darkhaven).

Each zone is given a first_room / last_room pair.

If the player is inside that range (inclusive) then the destinations listed 
in available_dests are available to them.

If they are outside all ranges then they cannot use "whereis" in that place.

The examples below are a long list for Darkhaven, and to illustrate the idea of
a secondary set of directions, a shorter one for Redferne's residence.

There is no limit to the number of zones you can incorporate in this way.

They are searched sequentially, so if you want to have a smaller subzone inside 
a main zone, put the subzone further up the list.

--]]

known_destinations = {

-- ------------------------------------------------------------------------------
  {  ---> destination set 1  (Darkhaven and the Academy)


  first_room = 10300,
  last_room = 21499,

  -- where we want them to go to
  
  -- make sure keys are LOWER CASE - as player input is forced to lower case
  
  available_dests = {
     butcher = 21057,
     academy = 21280,
     inn = 21069,
     baker = 21060 ,
     tavern = 21068,
     dairy = 21061,
     alchemist = 21054,
     weapons = 21062,
     square = 21000,
     magic = 21055,
     scrolls = 21051,
     repairs = 21058,
     clothing = 21066,
     headmistress = 10300,
     languages = 10306,
     skills = 10303,
     healer = 10319,
     battleground = 10368,
     fountain = 10300,
     tailor = 21066,
     ["south gate"] = 21074,
     ["north gate"] = 21100,
     ["west gate"] = 21088,
     ["east gate"] = 21113,
     ["cathedral altar"] = 21194,
     
     }  -- end of available_dests
     
     
   },  --> end of destination set 1 (Darkhaven and the Academy)
   
-- ------------------------------------------------------------------------------
   
  {  ---> destination set 2  (Redferne's residence)


  first_room = 7900,
  last_room = 7918,

  available_dests = {
     outside = 7900,
     kitchen = 7906,
     ["monster pen"] = 7913,
      
     }  -- end of available_dests
     
   },  --> end of destination set 2 (Redferne's residence)

-- ------------------------------------------------------------------------------


-- ------------------------------------------------------------------------------
   
  {  ---> destination set 3  (Immortal Command)


  first_room = 100,
  last_room = 115,

  available_dests = {
     command = 100,
     shipyard = 113,
     ["cole's shipyard"] = 114,
     ["post office"] = 102,
     office = 106,
     construction = 104,
      
     }  -- end of available_dests
     
   },  --> end of destination set 2 (Redferne's residence)

-- ------------------------------------------------------------------------------

   }  -- end of all known_destinations
   