/****************************************************************************
 * [S]imulated [M]edieval [A]dventure multi[U]ser [G]ame      |   \\._.//   *
 * -----------------------------------------------------------|   (0...0)   *
 * SMAUG 1.4 (C) 1994, 1995, 1996, 1998  by Derek Snider      |    ).:.(    *
 * -----------------------------------------------------------|    {o o}    *
 * SMAUG code team: Thoric, Altrag, Blodkai, Narn, Haus,      |   / ' ' \   *
 * Scryn, Rennard, Swordbearer, Gorog, Grishnakh, Nivek,      |~'~.VxvxV.~'~*
 * Tricops and Fireblade                                      |             *
 * ------------------------------------------------------------------------ *
 * Merc 2.1 Diku Mud improvments copyright (C) 1992, 1993 by Michael        *
 * Chastain, Michael Quan, and Mitchell Tse.                                *
 * Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,          *
 * Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.     *
 * ------------------------------------------------------------------------ *
 *			 Lua Scripting Module     by Nick Gammon                  			    *
 ****************************************************************************/
 
/*

Lua scripting written by Nick Gammon
Date: 8th July 2007

You are welcome to incorporate this code into your MUD codebases.

Post queries at: http://www.gammon.com.au/forum/

Description of functions in this file is at:

  http://www.gammon.com.au/forum/?id=8015

*/

#include <stdio.h>
#include <string.h>
#include <time.h>
#include "mud.h"

#include <lua5.1/lualib.h>
#include <lua5.1/lauxlib.h>

lua_State *L_mud = NULL;  /* Lua state for entire MUD */


/* Mersenne Twister stuff - see mt19937ar.c */

void init_genrand(unsigned long s);
void init_by_array(unsigned long init_key[], int key_length);
double genrand(void);

/* in lua_tables.c */

void add_lua_tables (lua_State *L);

#define CHARACTER_STATE "character.state"
#define CHARACTER_META "character.metadata"
#define MUD_LIBRARY "mud"
#define MT_LIBRARY "mt"

static char *dir_text[] = { "n", "e", "s", "w", "u", "d", "ne", "nw", "se", "sw", "?" };

// number of items in an array
#define NUMITEMS(arg) (sizeof (arg) / sizeof (arg [0]))

/* void RegisterLuaCommands (lua_State *L); */ /* Implemented in lua_commands.c */
LUALIB_API int luaopen_bits(lua_State *L);  /* Implemented in lua_bits.c */



static int optboolean (lua_State *L, const int narg, const int def) 
  {
  /* that argument not present, take default  */
  if (lua_gettop (L) < narg)
    return def;

  /* nil will default to the default  */
  if (lua_isnil (L, narg))
    return def;

  if (lua_isboolean (L, narg))
    return lua_toboolean (L, narg);

  return luaL_checknumber (L, narg) != 0;
}

static int check_vnum (lua_State *L)
  {

  int vnum = luaL_checknumber (L, 1);
  if ( vnum < 1 || vnum > MAX_VNUM )
    luaL_error (L, "Vnum %d is out of range 1 to %d", vnum, MAX_VNUM);

  return vnum;
  }  /* end of check_vnum */

  /* given a character pointer makes a userdata for it */
static void make_char_ud (lua_State *L, CHAR_DATA * ch)  
  {
  if (!ch)
    luaL_error (L, "make_char_ud called with NULL character");
  
  lua_pushlightuserdata(L, (void *)ch);    /* push value */
  luaL_getmetatable (L, CHARACTER_META);
  lua_setmetatable (L, -2);  /* set metatable for userdata */
  
  /* returns with userdata on the stack */
  }

#define check_character(L, arg)  \
   (CHAR_DATA *) luaL_checkudata (L, arg, CHARACTER_META)

/* Given a Lua state, return the character it belongs to */

CHAR_DATA * L_getchar (lua_State *L)
 {
 /* retrieve our character */
 
  CHAR_DATA * ch;
  
  /* retrieve the character */
  lua_pushstring(L, CHARACTER_STATE);  /* push address */
  lua_gettable(L, LUA_ENVIRONINDEX);  /* retrieve value */

  ch = (CHAR_DATA *) lua_touserdata(L, -1);  /* convert to data */

  if (!ch)  
    luaL_error (L, "No current character");

  lua_pop(L, 1);  /* pop result */

  return ch;
} /* end of L_getchar */
  
static ROOM_INDEX_DATA * L_find_room (lua_State *L, const int nArg)
{
 
 /* first, they might have actually specified the room vnum */
 if (lua_isnumber (L, nArg))
    {
    int vnum = luaL_checknumber (L, nArg);
    if ( vnum < 1 || vnum > MAX_VNUM )
      luaL_error (L, "Room vnum %d is out of range 1 to %d", vnum, MAX_VNUM);
     
    return get_room_index (vnum);
    }  
    
  /* if not, deduce from character's current room */ 
    
  return L_getchar (L)->in_room;
  
}  /* end of L_find_room */

/*

  find a character by: userdata / mob vnum / character name
  
  Possibilities are:
  
  (char_userdata, boolean)   --> search room/world for userdata, based on room
                                of current character
                                
  (char_userdata, room_vnum) --> search room for userdata, in room vnum
  
  (mob_vnum, boolean)        --> search room/world for mob vnum, based on room
                                 of current character

  (mob_vnum, room_vnum)      --> search room for mob vnum, in room vnum
  
  (name)                     --> search room for named player, then world


*/


static CHAR_DATA * L_find_character (lua_State *L)
{
  CHAR_DATA * ch = NULL;
  ROOM_INDEX_DATA * room = NULL;
  const char * name;
  CHAR_DATA *wch = NULL;
  int iArg1Type = lua_type (L, 1);
  int iArg2Type = lua_type (L, 2);
  
  /* characters/mobs can be specified by character userdata */
  if (iArg1Type == LUA_TLIGHTUSERDATA)
    {
    CHAR_DATA * ud_ch = check_character (L, 1);
    
    /* 2nd argument is number - must be room number to look in */
    
    if (iArg2Type == LUA_TNUMBER)
      room = L_find_room (L, 2); 
    else
      {  /* otherwise, look in character's room */
      ch = L_getchar (L);
      room = ch->in_room; 
      
      /* before reconnected event, we aren't in a room, but our character is still valid */
      /* also, self is a special, quick case */    
      if (ud_ch == ch)
        return ch;
      }

    if (!room)
      luaL_error (L, "No current room");

    /* for safety, make sure pointer is still valid */
    for ( wch = room->first_person; wch; wch = wch->next_in_room )
      if (wch == ud_ch)
        return ud_ch; 
        
     /* if 2nd argument is true, try the world */
    if (iArg2Type == LUA_TBOOLEAN && lua_toboolean (L, 2))
       {
       for( wch = first_char; wch; wch = wch->next ) 
         if (wch == ud_ch)
            return ud_ch; 
       }  /* end 2nd argument is true */

    return NULL;  /* pointer no longer valid */
    }
 
  /* given a number, assume a mob vnum */
  if (iArg1Type == LUA_TNUMBER)
    {
    int vnum = luaL_checknumber (L, 1);
      
   /* 2nd argument is number - must be room number to look in */
    
    if (iArg2Type == LUA_TNUMBER)
      room = L_find_room (L, 2); 
    else
      {  /* otherwise, look in character's room */
      ch = L_getchar (L);
      room = ch->in_room; 
      }

    if (!room)
      luaL_error (L, "No current room");
          
   /*
    * check the room for an exact match 
    */
    for( wch = room->first_person; wch; wch = wch->next_in_room )
     {
     if ( IS_NPC( wch ) && wch->pIndexData->vnum == vnum )
           return wch;
     }  /* end of for loop */

   /* if 2nd argument is true, try the world */
        
   if (iArg2Type == LUA_TBOOLEAN && lua_toboolean (L, 2))
     {
     for( wch = first_char; wch; wch = wch->next ) 
       {
       if ( IS_NPC( wch ) && wch->pIndexData->vnum == vnum )
             return wch;
       }  /* end of for loop */
     }  /* end 2nd argument is true */
     
    return NULL;  /* can't find it */   
    }  /* end of numeric mob wanted */

  /* not number or userdata, must want someone by name */
  
  ch = L_getchar (L);
  room = ch->in_room; 

  name = luaL_optstring (L, 1, "self");
  
  if (strcasecmp (name, "self") == 0)
    return ch;
  
  /*
  * check the room for an exact match 
  */
  for( wch = room->first_person; wch; wch = wch->next_in_room )
    if(!IS_NPC( wch ) && 
       ( strcasecmp ( name, wch->name ) == 0 ))
      break;  /* found it! */
  
  /*
  * check the world for an exact match 
  */
  if (!wch)
   {
   for( wch = first_char; wch; wch = wch->next )
      if(!IS_NPC( wch ) &&
          ( strcasecmp( name, wch->name ) == 0 ) )
        break;        
    }  /* end of checking entire world */   
  
  return wch;  /* use target character, not self */
   
}  /* end of L_find_character */


/* For debugging, show traceback information */

static void GetTracebackFunction (lua_State *L)
  {
  lua_pushliteral (L, LUA_DBLIBNAME);     /* "debug"   */
  lua_rawget      (L, LUA_GLOBALSINDEX);    /* get debug library   */

  if (!lua_istable (L, -1))
    {
    lua_pop (L, 2);   /* pop result and debug table  */
    lua_pushnil (L);
    return;
    }

  /* get debug.traceback  */
  lua_pushstring(L, "traceback");  
  lua_rawget    (L, -2);               /* get traceback function  */
  
  if (!lua_isfunction (L, -1))
    {
    lua_pop (L, 2);   /* pop result and debug table  */
    lua_pushnil (L);
    return;
    }

  lua_remove (L, -2);   /* remove debug table, leave traceback function  */
  }  /* end of GetTracebackFunction */

static int CallLuaWithTraceBack (lua_State *L, const int iArguments, const int iReturn)
  {

  int error;
  int base = lua_gettop (L) - iArguments;  /* function index */
  GetTracebackFunction (L);
  if (lua_isnil (L, -1))
    {
    lua_pop (L, 1);   /* pop non-existent function  */
    error = lua_pcall (L, iArguments, iReturn, 0);
    }  
  else
    {
    lua_insert (L, base);  /* put it under chunk and args */
    error = lua_pcall (L, iArguments, iReturn, base);
    lua_remove (L, base);  /* remove traceback function */
    }

  return error;
  }  /* end of CallLuaWithTraceBack  */

  
/* let scripters find our directories and file names */

#define INFO_STR_ITEM(arg) \
  lua_pushstring (L, arg);  \
  lua_setfield (L, -2, #arg)
  
#define INFO_NUM_ITEM(arg) \
  lua_pushnumber (L, arg);  \
  lua_setfield (L, -2, #arg)
    
static int L_system_info (lua_State *L)
{
  lua_newtable(L);  /* table for the info */
  
  /* directories */
  
  INFO_STR_ITEM (PLAYER_DIR);
  INFO_STR_ITEM (BACKUP_DIR);
  INFO_STR_ITEM (GOD_DIR);
  INFO_STR_ITEM (BOARD_DIR);
  INFO_STR_ITEM (CLAN_DIR);
  INFO_STR_ITEM (BUILD_DIR);
  INFO_STR_ITEM (SYSTEM_DIR);
  INFO_STR_ITEM (PROG_DIR);
  INFO_STR_ITEM (CORPSE_DIR);
  INFO_STR_ITEM (LUA_DIR);
  INFO_STR_ITEM (AREA_LIST);
  INFO_STR_ITEM (BAN_LIST);
  INFO_STR_ITEM (CLAN_LIST);
  INFO_STR_ITEM (GUILD_LIST);
  INFO_STR_ITEM (BOARD_FILE);
  INFO_STR_ITEM (SHUTDOWN_FILE);
  INFO_STR_ITEM (RIPSCREEN_FILE);
  INFO_STR_ITEM (RIPTITLE_FILE);
  INFO_STR_ITEM (ANSITITLE_FILE);
  INFO_STR_ITEM (ASCTITLE_FILE);
  INFO_STR_ITEM (BOOTLOG_FILE);
  INFO_STR_ITEM (IDEA_FILE);
  INFO_STR_ITEM (TYPO_FILE);
  INFO_STR_ITEM (LOG_FILE);
  INFO_STR_ITEM (WIZLIST_FILE);
  INFO_STR_ITEM (WHO_FILE);
  INFO_STR_ITEM (WEBWHO_FILE);
  INFO_STR_ITEM (SKILL_FILE);
  INFO_STR_ITEM (HERB_FILE);
  INFO_STR_ITEM (SOCIAL_FILE);
  INFO_STR_ITEM (COMMAND_FILE);
  INFO_STR_ITEM (LUA_STARTUP);
  INFO_STR_ITEM (LUA_MUD_STARTUP);
  
  /* other stuff */
  

  /* levels */
  INFO_NUM_ITEM (MAX_LEVEL);
  INFO_NUM_ITEM (MAX_CLAN);
  INFO_NUM_ITEM (MAX_HERB);
  INFO_NUM_ITEM (LEVEL_HERO);
  INFO_NUM_ITEM (LEVEL_IMMORTAL);
  INFO_NUM_ITEM (LEVEL_SUPREME);
  INFO_NUM_ITEM (LEVEL_INFINITE);
  INFO_NUM_ITEM (LEVEL_ETERNAL);
  INFO_NUM_ITEM (LEVEL_IMPLEMENTOR);
  INFO_NUM_ITEM (LEVEL_SUB_IMPLEM);
  INFO_NUM_ITEM (LEVEL_ASCENDANT);
  INFO_NUM_ITEM (LEVEL_GREATER);
  INFO_NUM_ITEM (LEVEL_GOD);
  INFO_NUM_ITEM (LEVEL_LESSER);
  INFO_NUM_ITEM (LEVEL_TRUEIMM);
  INFO_NUM_ITEM (LEVEL_DEMI);
  INFO_NUM_ITEM (LEVEL_SAVIOR);
  INFO_NUM_ITEM (LEVEL_CREATOR);
  INFO_NUM_ITEM (LEVEL_ACOLYTE);
  INFO_NUM_ITEM (LEVEL_NEOPHYTE);
  INFO_NUM_ITEM (LEVEL_AVATAR);
  INFO_NUM_ITEM (LEVEL_LOG);
  INFO_NUM_ITEM (LEVEL_HIGOD);

  /* other numbers */
  
  INFO_NUM_ITEM (PULSE_PER_SECOND);
  INFO_NUM_ITEM (PULSE_VIOLENCE	);
  INFO_NUM_ITEM (PULSE_MOBILE		);
  INFO_NUM_ITEM (PULSE_TICK		  );
  INFO_NUM_ITEM (PULSE_AREA			);
  INFO_NUM_ITEM (PULSE_AUCTION		);

  return 1;  /* the table itself */
}  /* end of L_system_info */

#define SYSDATA_STR_ITEM(arg) \
  if (sysdata.arg)  \
  {   \
  lua_pushstring (L, sysdata.arg);  \
  lua_setfield (L, -2, #arg); \
  }
  
#define SYSDATA_NUM_ITEM(arg) \
  lua_pushnumber (L, sysdata.arg);  \
  lua_setfield (L, -2, #arg)
  
#define SYSDATA_BOOL_ITEM(arg) \
  lua_pushboolean (L, sysdata.arg != 0);  \
  lua_setfield (L, -2, #arg)
  
static int L_sysdata (lua_State *L)
{
  lua_newtable(L);  /* table for the info */
    
  SYSDATA_NUM_ITEM (maxplayers);   /* Maximum players this boot   */
  SYSDATA_NUM_ITEM (alltimemax);   /* Maximum players ever   */
  SYSDATA_STR_ITEM (time_of_max);   /* Time of max ever */
  SYSDATA_BOOL_ITEM (NO_NAME_RESOLVING); /* Hostnames are not resolved  */
  SYSDATA_BOOL_ITEM (DENY_NEW_PLAYERS);  /* New players cannot connect  */
  SYSDATA_BOOL_ITEM (WAIT_FOR_AUTH);  /* New players must be auth'ed */
  SYSDATA_NUM_ITEM (read_all_mail); /* Read all player mail(was 54) */
  SYSDATA_NUM_ITEM (read_mail_free);   /* Read mail for free (was 51) */
  SYSDATA_NUM_ITEM (write_mail_free);  /* Write mail for free(was 51) */
  SYSDATA_NUM_ITEM (take_others_mail); /* Take others mail (was 54)   */
  SYSDATA_NUM_ITEM (muse_level); /* Level of muse channel */
  SYSDATA_NUM_ITEM (think_level);   /* Level of think channel LEVEL_HIGOD */
  SYSDATA_NUM_ITEM (build_level);   /* Level of build channel LEVEL_BUILD */
  SYSDATA_NUM_ITEM (log_level);  /* Level of log channel LEVEL LOG */
  SYSDATA_NUM_ITEM (level_modify_proto);  /* Level to modify prototype stuff LEVEL_LESSER */
  SYSDATA_NUM_ITEM (level_override_private); /* override private flag */
  SYSDATA_NUM_ITEM (level_mset_player);   /* Level to mset a player */
  SYSDATA_NUM_ITEM (stun_plr_vs_plr);  /* Stun mod player vs. player */
  SYSDATA_NUM_ITEM (stun_regular);  /* Stun difficult */
  SYSDATA_NUM_ITEM (dam_plr_vs_plr);   /* Damage mod player vs. player */
  SYSDATA_NUM_ITEM (dam_plr_vs_mob);   /* Damage mod player vs. mobile */
  SYSDATA_NUM_ITEM (dam_mob_vs_plr);   /* Damage mod mobile vs. player */
  SYSDATA_NUM_ITEM (dam_mob_vs_mob);   /* Damage mod mobile vs. mobile */
  SYSDATA_NUM_ITEM (level_getobjnotake);  /* Get objects without take flag */
  SYSDATA_NUM_ITEM (level_forcepc); /* The level at which you can use force on players. */
  SYSDATA_NUM_ITEM (max_sn);  /* Max skills */
  SYSDATA_STR_ITEM (guild_overseer);   /* Pointer to char containing the name of the */
  SYSDATA_STR_ITEM (guild_advisor); /* guild overseer and advisor. */
  SYSDATA_NUM_ITEM (save_flags);   /* Toggles for saving conditions */
  SYSDATA_NUM_ITEM (save_frequency);   /* How old to autosave someone */
    
  return 1;  /* the table itself */
}  /* end of L_sysdata */

#define CH_STR_ITEM(arg) \
  if (ch->arg)  \
  {   \
  lua_pushstring (L, ch->arg);  \
  lua_setfield (L, -2, #arg); \
  }
  
#define CH_NUM_ITEM(arg) \
  lua_pushnumber (L, ch->arg);  \
  lua_setfield (L, -2, #arg)
    
#define PC_STR_ITEM(arg) \
if (pc->arg)  \
  {   \
  lua_pushstring (L, pc->arg);  \
  lua_setfield (L, -2, #arg); \
  }
  
#define PC_NUM_ITEM(arg) \
  lua_pushnumber (L, pc->arg);  \
  lua_setfield (L, -2, #arg)
    
static int L_character_info (lua_State *L)
{
  CHAR_DATA * ch = L_find_character (L);
     
  if (!ch)
    return 0;
 
  PC_DATA *pc = ch->pcdata;
  
  lua_newtable(L);  /* table for the info */
  
  /* strings */
  
  CH_STR_ITEM (name);
  CH_STR_ITEM (short_descr);
  CH_STR_ITEM (long_descr);
  CH_STR_ITEM (description);
  
  /* numbers */
  
  CH_NUM_ITEM (num_fighting);
  CH_NUM_ITEM (substate);
  CH_NUM_ITEM (sex);
  CH_NUM_ITEM (main_ability);
  CH_NUM_ITEM (race);
  CH_NUM_ITEM (top_level);
  CH_NUM_ITEM (trust);
  CH_NUM_ITEM (played);
  CH_NUM_ITEM (logon);
  CH_NUM_ITEM (save_time);
  CH_NUM_ITEM (timer);
  CH_NUM_ITEM (wait);
  CH_NUM_ITEM (hit);
  CH_NUM_ITEM (max_hit);
  CH_NUM_ITEM (mana);
  CH_NUM_ITEM (max_mana);
  CH_NUM_ITEM (move);
  CH_NUM_ITEM (max_move);
  CH_NUM_ITEM (numattacks);
  CH_NUM_ITEM (gold);
  CH_NUM_ITEM (carry_weight);
  CH_NUM_ITEM (carry_number);
  CH_NUM_ITEM (xflags);
  CH_NUM_ITEM (immune);
  CH_NUM_ITEM (resistant);
  CH_NUM_ITEM (susceptible);
  CH_NUM_ITEM (speaks);
  CH_NUM_ITEM (speaking);
  CH_NUM_ITEM (saving_poison_death);
  CH_NUM_ITEM (saving_wand);
  CH_NUM_ITEM (saving_para_petri);
  CH_NUM_ITEM (saving_breath);
  CH_NUM_ITEM (saving_spell_staff);
  CH_NUM_ITEM (alignment);
  CH_NUM_ITEM (barenumdie);
  CH_NUM_ITEM (baresizedie);
  CH_NUM_ITEM (mobthac0);
  CH_NUM_ITEM (hitroll);
  CH_NUM_ITEM (damroll);
  CH_NUM_ITEM (hitplus);
  CH_NUM_ITEM (damplus);
  CH_NUM_ITEM (position);
  CH_NUM_ITEM (defposition);
  CH_NUM_ITEM (height);
  CH_NUM_ITEM (weight);
  CH_NUM_ITEM (armor);
  CH_NUM_ITEM (wimpy);
  CH_NUM_ITEM (deaf);
  CH_NUM_ITEM (perm_str);
  CH_NUM_ITEM (perm_int);
  CH_NUM_ITEM (perm_wis);
  CH_NUM_ITEM (perm_dex);
  CH_NUM_ITEM (perm_con);
  CH_NUM_ITEM (perm_cha);
  CH_NUM_ITEM (perm_lck);
  CH_NUM_ITEM (perm_frc);
  CH_NUM_ITEM (mod_str);
  CH_NUM_ITEM (mod_int);
  CH_NUM_ITEM (mod_wis);
  CH_NUM_ITEM (mod_dex);
  CH_NUM_ITEM (mod_con);
  CH_NUM_ITEM (mod_cha);
  CH_NUM_ITEM (mod_lck);
  CH_NUM_ITEM (mod_frc);
  CH_NUM_ITEM (mental_state);  
  CH_NUM_ITEM (emotional_state);  
     
  /* player characters have extra stuff (in "pc" sub table)  */
  if (pc)
  {
   lua_newtable(L);  /* table for the info */
      
   PC_STR_ITEM (homepage);
   PC_STR_ITEM (clan_name);
   PC_STR_ITEM (bamfin);
   PC_STR_ITEM (bamfout);
   PC_STR_ITEM (rank);
   PC_STR_ITEM (title);
   PC_STR_ITEM (bestowments);     
   PC_STR_ITEM (helled_by);
   PC_STR_ITEM (bio);  
   PC_STR_ITEM (authed_by); 
   PC_STR_ITEM (prompt);    
   PC_STR_ITEM (subprompt); 
   
   PC_NUM_ITEM (flags);     
   PC_NUM_ITEM (pkills);    
   PC_NUM_ITEM (pdeaths);   
   PC_NUM_ITEM (mkills);    
   PC_NUM_ITEM (mdeaths);   
   PC_NUM_ITEM (illegal_pk);   
   PC_NUM_ITEM (outcast_time); 
   PC_NUM_ITEM (restore_time); 
   PC_NUM_ITEM (r_range_lo); 
   PC_NUM_ITEM (r_range_hi);
   PC_NUM_ITEM (m_range_lo); 
   PC_NUM_ITEM (m_range_hi);
   PC_NUM_ITEM (o_range_lo); 
   PC_NUM_ITEM (o_range_hi);
   PC_NUM_ITEM (wizinvis);   
   PC_NUM_ITEM (min_snoop);  
   PC_NUM_ITEM (quest_number); 
   PC_NUM_ITEM (quest_curr);   
   PC_NUM_ITEM (quest_accum);  
   PC_NUM_ITEM (auth_state);
   PC_NUM_ITEM (release_date); 
   PC_NUM_ITEM (pagerlen);     
   lua_setfield (L, -2, "pc");  
  }
     
  return 1;  /* the table itself */
}  /* end of L_character_info */
 
#define OBJ_STR_ITEM(arg) \
if (obj->arg)  \
  {   \
  lua_pushstring (L, obj->arg);  \
  lua_setfield (L, -2, #arg); \
  }
  
#define OBJ_NUM_ITEM(arg) \
  lua_pushnumber (L, obj->arg);  \
  lua_setfield (L, -2, #arg)
  
  
static void build_inventory (lua_State *L, OBJ_DATA * obj );

static void add_object_item (lua_State *L, OBJ_DATA * obj, const int item)
  {
int i;
  lua_newtable(L);  /* table for the info */
          
  OBJ_STR_ITEM (name);
  OBJ_STR_ITEM (short_descr);
  OBJ_STR_ITEM (description);
  OBJ_STR_ITEM (action_desc);
   
  OBJ_NUM_ITEM (item_type);
  OBJ_NUM_ITEM (magic_flags);  
  OBJ_NUM_ITEM (wear_loc);
  OBJ_NUM_ITEM (weight);
  OBJ_NUM_ITEM (cost);
  OBJ_NUM_ITEM (level);
  OBJ_NUM_ITEM (timer);
  OBJ_NUM_ITEM (count);    
  OBJ_NUM_ITEM (serial);   
  OBJ_NUM_ITEM (room_vnum);
  
  if (obj->pIndexData)
    {
    lua_pushnumber (L, obj->pIndexData->vnum); 
    lua_setfield (L, -2, "vnum");
    }
    
  lua_newtable(L);  /* table for the values */
  
  /* do 6 values */
  for (i = 0; i < 6; i++)
    {
    lua_pushnumber (L, obj->value [i]); 
    lua_rawseti (L, -2, i + 1);       
    }
  lua_setfield (L, -2, "value");    
  
  if( obj->first_content )   /* node has a child? */
    {
    lua_newtable(L);  /* table for the inventory */
    build_inventory(L,  obj->first_content );
    lua_setfield (L, -2, "contents");
    }
  lua_rawseti (L, -2, item);         
  
    
  } /* end of add_object_item */
 
/* do one inventory level */
  
static void build_inventory (lua_State *L, OBJ_DATA * obj )
{
  int item = 1;
   
  for ( ; obj; obj = obj->next_content)
    {
      
    /* if carrying it, add to table */
    
    if( obj->wear_loc == WEAR_NONE)
      add_object_item (L, obj, item++);
      
  }  /* end of for loop */
  
}  /* end of build_inventory */

static int L_inventory (lua_State *L)
{
  CHAR_DATA * ch = L_find_character (L);
   
  if (!ch)
    return 0;
    
  lua_newtable(L);  /* table for the inventory */
    
  /* recursively build inventory */
  
  build_inventory (L, ch->first_carrying);
  
  return 1;  /* the table itself */
}  /* end of L_inventory */

static int L_level (lua_State *L)
{
  CHAR_DATA * ch = L_find_character (L);
   
  if (!ch)
    return 0;
  
  lua_pushnumber (L, ch->top_level);
  return 1;
}  /* end of L_level */
    
static int L_race (lua_State *L)
{
  CHAR_DATA * ch = L_find_character (L);
   
  if (!ch)
    return 0;
  
  lua_pushstring (L, get_race (ch));
  return 1;
}  /* end of L_race */

static int L_class (lua_State *L)
{
  CHAR_DATA * ch = L_find_character (L);
   
  if (!ch)
    return 0;
  
  lua_pushnumber (L, get_main_ability (ch));
  return 1;
}  /* end of L_class */
    
static int L_equipped (lua_State *L)
  {
  int item = 1;
  CHAR_DATA * ch = L_find_character (L);
  OBJ_DATA * obj;
  
  if (!ch)
    return 0;
    
  lua_newtable(L);  /* table for the inventory */
    
  for (obj = ch->first_carrying ; obj; obj = obj->next_content)
    {
    if (ch == obj->carried_by && 
        obj->wear_loc > WEAR_NONE)
      add_object_item (L, obj, item++);
     }  /* end of for loop */

  return 1;  /* the table itself */
}  /* end of L_equipped */

static bool check_inventory (CHAR_DATA * ch, OBJ_DATA * obj, const int vnum )
{
  /* check all siblings */   
  for ( ; obj; obj = obj->next_content)
    {
    if( obj->wear_loc == WEAR_NONE)
      {
      if (obj->pIndexData && obj->pIndexData->vnum == vnum)
        return TRUE;

      }  /* end of carrying it */

    if( obj->first_content )   /* node has a child? */
      {
      if (check_inventory(ch, obj->first_content, vnum ))
        return TRUE;
      }
    
  }  /* end of for loop */
  
  return FALSE;
}  /* end of check_inventory */

/* argument: vnum of item */

static int L_carryingvnum (lua_State *L)
{
  CHAR_DATA * ch = L_getchar (L);
   
  int vnum = check_vnum (L);
     
  /* recursively check inventory */
  
  lua_pushboolean (L, check_inventory (ch, ch->first_carrying, vnum));
  
  return 1;  /* the result */
}  /* end of L_carryingvnum */

static int count_inventory (CHAR_DATA * ch, OBJ_DATA * obj, 
                            const int vnum, const int recurse )
{
  int count = 0;    /* count at this level */
  
  /* check all siblings */   
  for ( ; obj; obj = obj->next_content)
    {
    if( obj->wear_loc == WEAR_NONE)
      {
      if (obj->pIndexData && obj->pIndexData->vnum == vnum)
        count += obj->count;
      }  /* end of carrying it */

    if (recurse)
      if( obj->first_content )   /* node has a child? */
        count += count_inventory(ch, obj->first_content, vnum, recurse);
    
  }  /* end of for loop */
  
  return count;
}  /* end of count_inventory */

/* argument: vnum of item */

static int L_possessvnum (lua_State *L)
{
  CHAR_DATA * ch = L_getchar (L);
   
  int vnum = check_vnum (L);
  int recurse = optboolean (L, 2, FALSE);
     
  /* recursively check inventory unless 2nd argument is false */
  
  lua_pushnumber (L, count_inventory (ch, ch->first_carrying, vnum, recurse));
  
  return 1;  /* the result */
}  /* end of L_possessvnum */

/* argument: location of item (eg. finger, legs, see item_w_flags) */
 
static int L_wearing (lua_State *L)
{
CHAR_DATA * ch = L_getchar (L);
OBJ_DATA * obj;
const char * sloc = luaL_checkstring (L, 1);  /* location, eg. finger */
int loc;

  /* translate location into a number */
  for (loc = 0; loc < MAX_WEAR; loc++)
   /* if (strcasecmp (sloc, wear_flags [loc]) == 0) */
       break;
     
   if (loc >= MAX_WEAR)
     luaL_error (L, "Bad wear location '%s' to 'wearing'", sloc);
    
   for (obj = ch->first_carrying ; obj; obj = obj->next_content)
    {
    if (ch == obj->carried_by && loc == obj->wear_loc)
       {
       lua_pushnumber (L, obj->pIndexData->vnum);
       return 1;
       }  /* end of if worn at desired location */
    }  /* end of for loop */

  lua_pushboolean (L, FALSE);
  return 1;  /* the result */
}  /* end of L_wearing */
    
/* argument: vnum of item */
 
static int L_wearingvnum (lua_State *L)
{
CHAR_DATA * ch = L_getchar (L);
OBJ_DATA * obj;
bool found = FALSE;

  int vnum = check_vnum (L);
      
  for (obj = ch->first_carrying ; obj; obj = obj->next_content)
    {
    if (ch == obj->carried_by && 
        obj->wear_loc > WEAR_NONE && 
        obj->pIndexData && obj->pIndexData->vnum == vnum)
       {
       found = TRUE;
       break;
       }  /* end of if carried this vnum */
    }  /* end of for loop */

  lua_pushboolean (L, found);
     
  return 1;  /* the result */
}  /* end of L_wearingvnum */

#define MOB_STR_ITEM(arg) \
if (mob->arg)  \
  {   \
  lua_pushstring (L, mob->arg);  \
  lua_setfield (L, -2, #arg); \
  }
  
#define MOB_NUM_ITEM(arg) \
  lua_pushnumber (L, mob->arg);  \
  lua_setfield (L, -2, #arg)
  
static int L_mob_info (lua_State *L)
  {
  MOB_INDEX_DATA *mob = get_mob_index( check_vnum (L) );
  if (!mob)
    return 0;
    
 lua_newtable(L);  /* table for the info */
  
  /* strings */
     
  MOB_STR_ITEM (player_name);
  MOB_STR_ITEM (short_descr);
  MOB_STR_ITEM (long_descr);
  MOB_STR_ITEM (description);
  
  MOB_NUM_ITEM (vnum);
  MOB_NUM_ITEM (count);
  MOB_NUM_ITEM (killed);
  MOB_NUM_ITEM (sex);
  MOB_NUM_ITEM (level);
  MOB_NUM_ITEM (alignment);
  MOB_NUM_ITEM (mobthac0);
  MOB_NUM_ITEM (ac);
  MOB_NUM_ITEM (hitnodice);
  MOB_NUM_ITEM (hitsizedice);
  MOB_NUM_ITEM (hitplus);
  MOB_NUM_ITEM (damnodice);
  MOB_NUM_ITEM (damsizedice);
  MOB_NUM_ITEM (damplus);
  MOB_NUM_ITEM (numattacks);
  MOB_NUM_ITEM (gold);
  MOB_NUM_ITEM (exp);
  MOB_NUM_ITEM (xflags);
  MOB_NUM_ITEM (immune);
  MOB_NUM_ITEM (resistant);
  MOB_NUM_ITEM (susceptible);
  MOB_NUM_ITEM (speaks);
  MOB_NUM_ITEM (speaking);
  MOB_NUM_ITEM (position);
  MOB_NUM_ITEM (defposition);
  MOB_NUM_ITEM (height);
  MOB_NUM_ITEM (weight);
  MOB_NUM_ITEM (race);
  MOB_NUM_ITEM (hitroll);
  MOB_NUM_ITEM (damroll);
  MOB_NUM_ITEM (perm_str);
  MOB_NUM_ITEM (perm_int);
  MOB_NUM_ITEM (perm_wis);
  MOB_NUM_ITEM (perm_dex);
  MOB_NUM_ITEM (perm_con);
  MOB_NUM_ITEM (perm_cha);
  MOB_NUM_ITEM (perm_lck);
  MOB_NUM_ITEM (perm_frc);
  MOB_NUM_ITEM (saving_poison_death);
  MOB_NUM_ITEM (saving_wand);
  MOB_NUM_ITEM (saving_para_petri);
  MOB_NUM_ITEM (saving_breath);
  MOB_NUM_ITEM (saving_spell_staff);

  return 1;  /* the table itself */ 
  }  /* end of L_mob_info */

  
static int L_object_info (lua_State *L)
  {
OBJ_INDEX_DATA *obj = get_obj_index ( check_vnum (L) );
int i;

  if (!obj)
    return 0;
    
 lua_newtable(L);  /* table for the info */
  
  /* strings */
     
   OBJ_STR_ITEM (name);
   OBJ_STR_ITEM (short_descr);
   OBJ_STR_ITEM (description);
   OBJ_STR_ITEM (action_desc);

   OBJ_NUM_ITEM (vnum);
   OBJ_NUM_ITEM (level);
   OBJ_NUM_ITEM (item_type);
   OBJ_NUM_ITEM (magic_flags);  
   OBJ_NUM_ITEM (count);
   OBJ_NUM_ITEM (weight);
   OBJ_NUM_ITEM (cost);
   OBJ_NUM_ITEM (serial);
   OBJ_NUM_ITEM (layers);

  lua_newtable(L);  /* table for the values */
  
  /* do 6 values */
  for (i = 0; i < 6; i++)
    {
    lua_pushnumber (L, obj->value [i]); 
    lua_rawseti (L, -2, i + 1);       
    }
  lua_setfield (L, -2, "value");    

   
  return 1;  /* the table itself */ 
  }  /* end of L_object_info */
  
static int L_object_name (lua_State *L)
  {
OBJ_INDEX_DATA *obj = get_obj_index ( check_vnum (L) );

  if (!obj)
    return 0;  

  lua_pushstring (L, obj->short_descr);
  lua_pushstring (L, obj->description);
  
  return 2;
} /* end of L_object_name */

static int L_mob_name (lua_State *L)
  {
MOB_INDEX_DATA *mob = get_mob_index ( check_vnum (L) );

  if (!mob)
    return 0;  

  lua_pushstring (L, mob->short_descr);
  lua_pushstring (L, mob->description);
  
  return 2;
} /* end of L_mob_name */

#define AREA_STR_ITEM(arg) \
if (area->arg)  \
  {   \
  lua_pushstring (L, area->arg);  \
  lua_setfield (L, -2, #arg); \
  }
  
#define AREA_NUM_ITEM(arg) \
  lua_pushnumber (L, area->arg);  \
  lua_setfield (L, -2, #arg)
    
        
static int L_area_info (lua_State *L)
{
  const char * name = luaL_checkstring (L, 1); /* area file name */
  
  AREA_DATA * area;
 
  for ( area = first_asort; area; area = area->next_sort )
    if (strcasecmp (area->filename, name) == 0)
      break;

  if (area == NULL) 
    return 0;  /* oops - area does not exist */
  
  lua_newtable(L);  /* table for the info */
  
  /* strings */
  
  AREA_STR_ITEM (name);
  AREA_STR_ITEM (filename);
  AREA_STR_ITEM (author);  
  AREA_STR_ITEM (resetmsg);   
 
  /* numbers */
  
  AREA_NUM_ITEM (flags);
  AREA_NUM_ITEM (status); 
  AREA_NUM_ITEM (age);
  AREA_NUM_ITEM (nplayer);
  AREA_NUM_ITEM (reset_frequency);
  AREA_NUM_ITEM (low_r_vnum);
  AREA_NUM_ITEM (hi_r_vnum);
  AREA_NUM_ITEM (low_o_vnum);
  AREA_NUM_ITEM (hi_o_vnum);
  AREA_NUM_ITEM (low_m_vnum);
  AREA_NUM_ITEM (hi_m_vnum);
  AREA_NUM_ITEM (low_soft_range);
  AREA_NUM_ITEM (hi_soft_range);
  AREA_NUM_ITEM (low_hard_range);
  AREA_NUM_ITEM (hi_hard_range);
  AREA_NUM_ITEM (max_players);
  AREA_NUM_ITEM (mkills);
  AREA_NUM_ITEM (mdeaths);
  AREA_NUM_ITEM (pkills);
  AREA_NUM_ITEM (pdeaths);
  AREA_NUM_ITEM (gold_looted);
  AREA_NUM_ITEM (illegal_pk);
  AREA_NUM_ITEM (high_economy);
  AREA_NUM_ITEM (low_economy);  
   
  return 1;  /* the table itself */
}  /* end of L_area_info */
  
static int L_area_list (lua_State *L)
{
 
  AREA_DATA * area;
  int count = 1;

  lua_newtable(L);  /* table for the info */
 
  for ( area = first_asort; area; area = area->next_sort )
    {
    lua_pushstring (L, area->filename);  
    lua_rawseti (L, -2, count++);  
    }
   
  return 1;  /* the table itself */
}  /* end of L_area_list */
  
#define ROOM_STR_ITEM(arg) \
if (room->arg)  \
  {   \
  lua_pushstring (L, room->arg);  \
  lua_setfield (L, -2, #arg); \
  }
  
#define ROOM_NUM_ITEM(arg) \
  lua_pushnumber (L, room->arg);  \
  lua_setfield (L, -2, #arg)
    
#define EXIT_STR_ITEM(arg) \
if (pexit->arg)  \
  {   \
  lua_pushstring (L, pexit->arg);  \
  lua_setfield (L, -2, #arg); \
  }
  
#define  EXIT_NUM_ITEM(arg) \
  lua_pushnumber (L, pexit->arg);  \
  lua_setfield (L, -2, #arg)
        
#define CONTENTS_STR_ITEM(arg) \
if (obj->arg)  \
  {   \
  lua_pushstring (L, obj->arg);  \
  lua_setfield (L, -2, #arg); \
  }
  
#define CONTENTS_NUM_ITEM(arg) \
  lua_pushnumber (L, obj->arg);  \
  lua_setfield (L, -2, #arg)
    
static int L_room_info (lua_State *L)
{
  ROOM_INDEX_DATA * room = L_find_room (L, 1);  /* which room */
  EXIT_DATA * pexit;
  OBJ_DATA * obj;
  int item = 1;
  
  if (room == NULL) 
    return 0;  /* oops - not in room or specified room does not exist */
  
  lua_newtable(L);  /* table for the info */
  
  /* strings */
  
  ROOM_STR_ITEM (name);
  ROOM_STR_ITEM (description);
  
  /* numbers */
  
  ROOM_NUM_ITEM (vnum);
  ROOM_NUM_ITEM (room_flags);
  ROOM_NUM_ITEM (light);   
  ROOM_NUM_ITEM (sector_type);
  ROOM_NUM_ITEM (tele_vnum);
  ROOM_NUM_ITEM (tele_delay);
  ROOM_NUM_ITEM (tunnel);  
       
  /* now do the exits */
  
  lua_newtable(L);  /* table for all exits */
  
  for( pexit = room->first_exit; pexit; pexit = pexit->next )
    {
    lua_newtable(L);  /* table for the info */
  
    EXIT_STR_ITEM (keyword);    
    EXIT_STR_ITEM (description);
        
    EXIT_NUM_ITEM (vnum);       
    EXIT_NUM_ITEM (rvnum);      
    EXIT_NUM_ITEM (exit_info);  
    EXIT_NUM_ITEM (key);        
    EXIT_NUM_ITEM (distance);     
    
    if (pexit->vdir < 0 || pexit->vdir > NUMITEMS (dir_text))
      continue;  /* can't get exit code */
      
    // key is exit name
    lua_setfield (L, -2, dir_text [pexit->vdir]) ;    
    
    }

  lua_setfield (L, -2, "exits");
  
 /* now do the contents */
  
  lua_newtable(L);  /* table for all objects */
    
  for( obj = room->first_content; obj; obj = obj->next_content )
    add_object_item (L, obj, item++);
    
  lua_setfield (L, -2, "contents"); 
   
  return 1;  /* the table itself */
}  /* end of L_room_info */

static int L_room_name (lua_State *L)
{
  ROOM_INDEX_DATA * room = L_find_room (L, 1);  /* which room */

  if (room == NULL) 
    return 0;  /* oops - not in room or specified room does not exist */
    
  lua_pushstring (L, room->name);
  lua_pushstring (L, room->description);
  return 2;
  } /* end of L_room_name */
  
static int L_room_exits (lua_State *L)
{
  ROOM_INDEX_DATA * room = L_find_room (L, 1);  /* which room */
  EXIT_DATA * pexit;
  int iExceptFlag = luaL_optnumber (L, 2, 0);  /* eg. except locked */
  
  if (room == NULL) 
    return 0;  /*  specified room does not exist */
       
  /* now do the exits */
  
  lua_newtable(L);  /* table for all exits */
  
  for( pexit = room->first_exit; pexit; pexit = pexit->next )
    {
    if ((pexit->exit_info & iExceptFlag) != 0)
      continue;  /* ignore, eg. locked exit */
    
    if (pexit->vdir < 0 || pexit->vdir > NUMITEMS (dir_text))
      continue;  /* can't get exit code */
      
    // key is exit name, value is exit direction
    lua_pushnumber (L, pexit->vnum);
    lua_setfield (L, -2, dir_text [pexit->vdir]) ;    
    }  /* end for loop */
   
  return 1;  /* the table itself */
}  /* end of L_room_exits */

static int L_send_to_char (lua_State *L)
{
  send_to_char(luaL_checkstring (L, 1), L_getchar (L));
  return 0;
}  /* end of L_send_to_char */

static int L_set_char_color (lua_State *L)
{
  int iColour = luaL_optnumber (L, 1, AT_PLAIN);
  
  if (iColour < 0 || iColour >= MAX_COLORS)
    luaL_error (L, "Colour %d is out of range 0 to %d", iColour, MAX_COLORS);
    
  set_char_color(iColour, L_getchar (L));
  return 0;
}  /* end of L_set_char_color */

/* act as if character had typed the command */
static int L_interpret (lua_State *L)
{
  char command [MAX_INPUT_LENGTH];  
  strncpy (command, luaL_checkstring (L, 1), sizeof (command));  
  command [sizeof (command) - 1] = 0;  
  
  interpret (L_getchar (L), command);
  return 0;
}  /* end of L_interpret */

/* force any character, or a mob in this room, to do something */
static int L_force (lua_State *L)
{
  CHAR_DATA * ch = L_find_character (L);
  int iCommand = 2;
  
  if (!ch)
    luaL_error (L, "Cannot find character/mob to force.");
    
 /* if character was (vnum, boolean) then command is 3rd argument */
 if ((lua_type (L, 1) == LUA_TNUMBER || lua_type (L, 1) == LUA_TLIGHTUSERDATA)
     && (lua_type (L, 2) == LUA_TBOOLEAN || lua_type (L, 2) == LUA_TNUMBER))
    iCommand++;
       
  char command [MAX_INPUT_LENGTH];  
  strncpy (command, luaL_checkstring (L, iCommand), sizeof (command));  
  command [sizeof (command) - 1] = 0;  
  
  interpret (ch, command);
  return 0;
}  /* end of L_force */

/* transfer character or mob to somewhere (default is here) */
static int L_transfer (lua_State *L)
{
CHAR_DATA * ch = L_getchar (L);
CHAR_DATA * victim = L_find_character (L);
int iDestination = 2;
ROOM_INDEX_DATA *location;
int vnum;
  
 /* if character was (vnum, boolean) then room is 3rd argument */
 if ((lua_type (L, 1) == LUA_TNUMBER || lua_type (L, 1) == LUA_TLIGHTUSERDATA)
     && (lua_type (L, 2) == LUA_TBOOLEAN || lua_type (L, 2) == LUA_TNUMBER))
     iDestination++;
  
 vnum = luaL_optnumber (L, iDestination, ch->in_room->vnum);

  if ( vnum < 1 || vnum > MAX_VNUM )
    luaL_error (L, "Room vnum %d is out of range 1 to %d", vnum, MAX_VNUM);
  
  if (!victim)
    luaL_error (L, "Cannot find character/mob to transfer.");
  
  location = get_room_index(vnum );
   
  if (!location)
    luaL_error (L, "Cannot find destination room: %d", vnum);
    
  if (victim->fighting)
      stop_fighting( victim, TRUE );
      
  char_from_room( victim );
  char_to_room( victim, location );
    
  return 0;
  } /* end of L_transfer */
  
/* stop the nominated character fighting */
static int L_stop_fighting (lua_State *L)
{
  CHAR_DATA * ch = L_find_character (L);
  
  if (!ch)
    luaL_error (L, "Cannot find character/mob to stop fighting.");
  
  if (ch->fighting)
      stop_fighting( ch, TRUE );
   
  return 0;
  } /* end of L_stop_fighting */

/* is the nominated character fighting? */
static int L_fighting (lua_State *L)
{
  CHAR_DATA * ch = L_find_character (L);
  
  if (!ch)
    luaL_error (L, "Cannot find character/mob.");
  
  lua_pushboolean (L, ch->fighting != NULL);
   
  return 1;
  } /* end of L_fighting */

  
static int L_gain_exp (lua_State *L)
{
  CHAR_DATA * ch = L_getchar (L); /* get character pointer */
  int iAbility, ability;
  
   ability = -1;
   for( iAbility = 0; iAbility < MAX_ABILITY; iAbility++ )
   {
      if( !str_prefix( luaL_checkstring (L, 2), ability_name[iAbility] ) )
      {
         ability = iAbility;
         break;
      }
   }  
  gain_exp (ch, luaL_checknumber (L, 1), ability);
  lua_pushnumber (L, ch->experience[ability]);
  return 1;  /* return new amount */
}  /* end of L_gain_exp */

static int L_gain_gold (lua_State *L)
{
  CHAR_DATA * ch = L_getchar (L); /* get character pointer */
  ch->gold += luaL_checknumber (L, 1);
  lua_pushnumber (L, ch->gold);
  return 1;
}  /* end of L_gain_gold */

/* remove an item from the character's top-level inventory */
static int L_destroy_item (lua_State *L)
{
  CHAR_DATA * ch = L_getchar (L); /* get character pointer */
  OBJ_DATA * obj;
  int vnum = check_vnum (L);
  int count = luaL_optnumber (L, 2, 1);
  
  int done = 0;    /* number we destroyed so far */
  
  /* check all siblings */   
  for (obj = ch->first_carrying; obj && done < count; obj = obj->next_content)
    {
    if( obj->wear_loc == WEAR_NONE)
      {
      if (obj->pIndexData && obj->pIndexData->vnum == vnum)
        {
        if( (done + obj->count) > count )   /* if too many, split them */
            split_obj( obj, count - done );
         done += obj->count;    /* we have destroyed this many */
         extract_obj ( obj );  /* remove it */
        }
      }  /* end of carrying it */
   
  }  /* end of for loop */
  
  /* return how many we destroyed */
  lua_pushnumber (L, done);
  return 1;
}  /* end of L_destroy_item */

/* remove an object from the room (vnum / room / count) */
static int L_purgeobj (lua_State *L)
{
int vnum = check_vnum (L);
ROOM_INDEX_DATA * room = L_find_room (L, 2);
int count = luaL_optnumber (L, 3, 1);
  
OBJ_DATA * obj;

int done = 0;    /* number we destroyed so far */

  for( obj = room->first_content; obj; obj = obj->next_content )
    {
    if (obj->pIndexData && obj->pIndexData->vnum == vnum)
      {
      if( (done + obj->count) > count )   /* if too many, split them */
          split_obj( obj, count - done );
       done += obj->count;    /* we have destroyed this many */
       extract_obj ( obj );  /* remove it */
      }
   
  }  /* end of for loop */
  
  /* return how many we destroyed */
  lua_pushnumber (L, done);
  return 1;
}  /* end of L_purgeobj */

static int L_purgemob (lua_State *L)
 {

CHAR_DATA * ch = L_find_character (L); /* get character pointer */

  if (!ch)
    return 0;
 
  if (!IS_NPC (ch))
    luaL_error (L, "Can only purge NPCs");
     
 extract_char (ch, TRUE);
 
 return 0;
 }  /* end of L_purgemob */
 
static int L_oinvoke (lua_State *L)
{
CHAR_DATA * ch = L_getchar (L); /* get character pointer */
OBJ_INDEX_DATA *pObjIndex;
OBJ_DATA * obj;
bool taken = FALSE;

  int vnum = check_vnum (L);
  int level = luaL_optnumber (L, 2, 1);
  if ( level < 1 || level > MAX_LEVEL )
    luaL_error (L, "Level %d is out of range 1 to %d", level, MAX_LEVEL);

  pObjIndex = get_obj_index ( vnum );

  if (!pObjIndex)
   luaL_error (L, "Cannot invoke object vnum %d - it does not exist", vnum);
   
   obj = create_object( pObjIndex, level );
   if( CAN_WEAR( obj, ITEM_TAKE ) )
     {
     obj = obj_to_char( obj, ch );
     taken = TRUE;
   }
   else
      obj = obj_to_room( obj, ch->in_room );
      
  lua_pushboolean (L, taken);
  
  return 1;  /* true if we took the item */
} /* end of L_oinvoke */
   
int generate_itemlevel( AREA_DATA * pArea, OBJ_INDEX_DATA * pObjIndex );

static OBJ_DATA * make_one_object (lua_State *L, 
                                   AREA_DATA * pArea, 
                                   const int shop,
                                   int level)
  {
OBJ_INDEX_DATA *pObjIndex;
OBJ_DATA * obj;
    
  if (!lua_isnumber (L, -1))
    luaL_error (L, "Object vnum must be numeric");
   
  const int ovnum = lua_tonumber (L, -1);
  
  pObjIndex = get_obj_index ( ovnum );
  if (!pObjIndex)
    luaL_error (L, "Object vnum %d does not exist", ovnum);

  if (level == -1)
    level = generate_itemlevel (pArea, pObjIndex );
    
  if (shop )
    {
    int olevel = generate_itemlevel (pArea, pObjIndex );
    obj = create_object( pObjIndex, olevel );
   /* xSET_BIT( obj->extra_flags, ITEM_INVENTORY ); shop inventory */
    }
  else
    obj = create_object( pObjIndex, number_fuzzy( level ) );
   
  obj->level = URANGE( 0, obj->level, LEVEL_AVATAR );

  lua_pop (L, 1);   /* remove object value   */

  return obj;
  }  /* end of make_one_object */

static OBJ_DATA * build_nested_object (lua_State *L, 
                                       AREA_DATA * pArea, 
                                       const int shop,
                                       const int level)
  {
   
  /* containers will be: { bag = 1234, 55, 66, 77 }
     That  is, bag vnum 1234 will contain objects: 55, 66, 77
  */
  
  if (lua_istable (L, -1))
    {
    size_t length = lua_objlen (L, -1);  /* size of table */
    OBJ_DATA * bag_obj;
    int i;
    
    /* get bag vnum */
    lua_getfield (L, -1, "bag");  
    if (lua_isnil (L, -1))
      luaL_error (L, "For nested inventory 'bag' (container) field must be present");
          
    /* make the bag */
    bag_obj = make_one_object (L, pArea, shop, level);  
      
    if (bag_obj->item_type != ITEM_CONTAINER)
      luaL_error (L, "Object container must be container type");
    
    for (i = 1; i <= length; i++)
      {
      OBJ_DATA * obj;

      lua_rawgeti (L, -1, i);  /* get vnum or table */
      
      obj = build_nested_object (L, pArea, shop, level);
                                         
      obj_to_obj( obj, bag_obj);
                                 
      }
          
    return bag_obj;
    }  /* end of container */

  /* non-nested object, just return it */
  return make_one_object (L, pArea, shop, level);  

  }  /* end of build_nested_object */


/* invoke a mob and equip it 

  arg1 = mob vnum
  arg2 = room to put it in
  arg3 = table of what to equip it with (eg. finger = vnum)
  arg4 = table of what it is to have in inventory

  returns userdata of new mob
*/

static int L_minvoke (lua_State *L)
{
int vnum = check_vnum (L);  /* mob vnum */
MOB_INDEX_DATA *pMobIndex = get_mob_index(vnum);
int room_vnum = luaL_checknumber (L, 2);
ROOM_INDEX_DATA *room =  L_find_room (L, 2);
CHAR_DATA * mob;
int level;

  if (!pMobIndex)
    luaL_error (L, "Cannot invoke mob vnum %d - mob does not exist", vnum);

  if (!room)
    luaL_error (L, "Cannot invoke mob vnum %d - room %d does not exist", vnum, room_vnum);
  
  mob = create_mobile( pMobIndex );
  char_to_room ( mob, room);

  level = URANGE ( 0, mob->top_level - 2, LEVEL_AVATAR );

  /* equip this mob (location/vnum pairs) */
  
  if (lua_istable (L, 3))
    {
    lua_pushnil (L);  // first key for traversal
    while (lua_next (L, 3) != 0)
      {
      OBJ_INDEX_DATA *pObjIndex;
      OBJ_DATA * obj;
      int loc;
      
      if (lua_type (L, -2) != LUA_TSTRING)
        luaL_error (L, "equipment location must be a string");
      if (lua_type (L, -1) != LUA_TNUMBER)
        luaL_error (L, "equipment vnum must be a number");
        
      const char * sLocation = lua_tostring (L, -2);
      const int ovnum = lua_tonumber (L, -1);

      pObjIndex = get_obj_index ( ovnum );
      if (!pObjIndex)
        luaL_error (L, "Cannot equip mob %d with object vnum %d - object does not exist",
                   vnum, ovnum);

      obj = create_object( pObjIndex, number_fuzzy( level ) );
      obj->level = URANGE( 0, obj->level, LEVEL_AVATAR );
                   
      if( CAN_WEAR( obj, ITEM_TAKE ) )
        obj = obj_to_char( obj, mob );
      else
        luaL_error (L, "Cannot equip mob %d with object vnum %d - object cannot be taken",
                     vnum, ovnum);
                   
      if (obj->carried_by != mob )
        luaL_error (L, "Cannot equip mob %d with object vnum %d - object was not carried",
                     vnum, ovnum);
                     
       /* translate location into a number */
        for (loc = 0; loc < MAX_WEAR; loc++)
          /*if (strcasecmp (sLocation, wear_flags [loc]) == 0)*/
             break;
           
       if (loc >= MAX_WEAR)
         luaL_error (L, "Bad wear location '%s', equipping mob %d with %d", 
                      sLocation, vnum, ovnum);
                   
      equip_char (mob, obj, loc);  /* equip with item */
      lua_pop (L, 1); // remove value, keep key for next iteration
      } // end of looping through table
    }  /* table of equipment present */

  /* put stuff in his inventory  (table of vnums) */
  
  if (lua_istable (L, 4))
    {
    size_t length = lua_objlen (L, 4);  /* size of table */
    int i;
      
    for (i = 1; i <= length; i++)
      {
      OBJ_DATA * obj;

      lua_rawgeti (L, 4, i);  /* get vnum or table */
      
      obj = build_nested_object (L, room->area, mob->pIndexData->pShop != NULL, level);
                                         
      if( CAN_WEAR( obj, ITEM_TAKE ) )
        obj = obj_to_char( obj, mob );
      else
        luaL_error (L, "Mob cannot take object");

      if (obj->carried_by != mob )
        luaL_error (L, "Mob cannot carry object");
                                  
      }
    }    /* end of having table of inventory */
         
  /* return userdata of new mob */
  make_char_ud (L, mob);
  return 1;  
} /* end of L_minvoke */

/* first arg: room, second arg: object vnum or table of items */
static int L_obj_to_room (lua_State *L)
{
ROOM_INDEX_DATA * room = L_find_room (L, 1);
  obj_to_room( build_nested_object (L, room->area, FALSE, -1), room );
  return 0;  
} /* end of L_obj_to_room */
   
static int L_set_exit_state (lua_State *L)
{
ROOM_INDEX_DATA * room = L_find_room (L, 1);
const char * sExit = luaL_checkstring (L, 2);
const char * sState = luaL_optstring (L, 3, "");
int vdir;
EXIT_DATA * pExit;

  if (!room)
    luaL_error (L, "Room does not exist", sExit);
  
  /* translate location into a number */
  for (vdir = 0; vdir < 11; vdir++)
    if (strcasecmp (sExit, dir_text [vdir]) == 0)
       break;

  if (vdir >= 11)
    luaL_error (L, "Invalid exit direction '%s'", sExit);
  
  pExit = get_exit (room, vdir);
  
  if (!pExit)
    luaL_error (L, "Exit '%s' does not exist", sExit);
  
  REMOVE_BIT( pExit->exit_info, EX_CLOSED );
  REMOVE_BIT( pExit->exit_info, EX_LOCKED );
  REMOVE_BIT( pExit->exit_info, EX_SECRET );

  if (strchr (sState, 'c') || strchr (sState, 'C'))
    SET_BIT( pExit->exit_info, EX_CLOSED );
  if (strchr (sState, 'l') || strchr (sState, 'L'))
    SET_BIT( pExit->exit_info, EX_LOCKED );
  if (strchr (sState, 's') || strchr (sState, 'S'))
    SET_BIT( pExit->exit_info, EX_SECRET );
    
  return 0;  
} /* end of L_set_exit */

static int L_randomize_exits (lua_State *L)
{
ROOM_INDEX_DATA * room = L_find_room (L, 1);
  
  if (!room)
    luaL_error (L, "Room does not exist");

  randomize_exits (room, luaL_checknumber (L, 2) - 1);
    
  return 0;  
} /* end of L_randomize_exits */

static int L_make_trap (lua_State *L)
{
ROOM_INDEX_DATA * room = L_find_room (L, 1);
int v0 = luaL_checknumber (L, 2);
int v1 = luaL_checknumber (L, 3);
int v2 = luaL_checknumber (L, 4);
int v3 = luaL_checknumber (L, 5);
  
  if (!room)
    luaL_error (L, "Room does not exist");

   /* don't if already there */
  /*if (count_obj_list (get_obj_index( OBJ_VNUM_TRAP ), room->first_content ) > 0) */
    return 0;
  
  obj_to_room (make_trap (v0, v1, v2, v3 ), room );
    
  return 0;  
} /* end of L_make_trap */

static int L_mobinworld (lua_State *L)
{
MOB_INDEX_DATA *m_index;
int world_count = 0;
  
  m_index = get_mob_index ( check_vnum (L) );

  if (m_index)
    world_count = m_index->count;
  
  if (world_count)
    lua_pushnumber (L, world_count);
  else
    lua_pushboolean (L, FALSE);
  return 1;
}  /* end of L_mobinworld */

static int L_mobinarea (lua_State *L)
{
CHAR_DATA *tmob;
MOB_INDEX_DATA *m_index;

int world_count = 0;
int found_count = 0;
int result = 0;
int vnum = check_vnum (L);
ROOM_INDEX_DATA *room = L_find_room (L, 2); /* second optional argument is the room */

  if (!room)
    return 0;
    
  m_index = get_mob_index ( vnum );

  if (m_index )
    world_count = m_index->count;
  
  for( tmob = first_char; tmob && found_count != world_count; tmob = tmob->next )
   {
   if( IS_NPC( tmob ) && tmob->pIndexData->vnum == vnum )
     {
      found_count++;
      if( tmob->in_room->area == room->area )
         result++;
     }
   }
   
  if (result)
    lua_pushnumber (L, result);
  else
    lua_pushboolean (L, FALSE);
  return 1;
}  /* end of L_mobinarea */

static int L_mobinroom (lua_State *L)
{
CHAR_DATA *tmob;
int result = 0;
int vnum = check_vnum (L);
ROOM_INDEX_DATA *room = L_find_room (L, 2); /* second optional argument is the room */

  if (!room) 
    return 0;  /* oops - not in room or specified room does not exist */

  for( tmob = room->first_person; tmob; tmob = tmob->next_in_room )
   {
   if ( IS_NPC( tmob ) && tmob->pIndexData->vnum == vnum )
         result++;
   }
   
  if (result)
    lua_pushnumber (L, result);
  else
    lua_pushboolean (L, FALSE);
  return 1;
}  /* end of L_mobinroom */

static int L_objinroom (lua_State *L)
{
OBJ_DATA * obj;
int result = 0;
int vnum = check_vnum (L);
ROOM_INDEX_DATA *room = L_find_room (L, 2); /* second optional argument is the room */

  if (!room) 
    return 0;  /* oops - not in room or specified room does not exist */

  for( obj = room->first_content; obj; obj = obj->next_content )
   {
   if ( obj->pIndexData->vnum == vnum )
         result++;
   }
   
  if (result)
    lua_pushnumber (L, result);
  else
    lua_pushboolean (L, FALSE);
  return 1;
}  /* end of L_objinroom */

static int L_players_in_room (lua_State *L)
{
  
ROOM_INDEX_DATA *room = L_find_room (L, 1); 
 
CHAR_DATA * wch;
int count = 1;
  
  if (!room) 
    return 0;  /* oops - not in room or specified room does not exist */
  
  lua_newtable(L);  /* table for the player name info */
  lua_newtable(L);  /* table for the player userdata info */
  
    
  for( wch = room->first_person; wch; wch = wch->next_in_room )
    if(!IS_NPC( wch ))
       {
       lua_pushstring (L, wch->name); 
       lua_rawseti (L, -3, count);  /* key is count */
       make_char_ud (L, wch);
       lua_rawseti (L, -2, count++);  /* key is count */
       }
   
  return 2;  /* first table is name, second is userdata */
}  /* end of L_players_in_room */

static int L_players_in_game (lua_State *L)
{
  
CHAR_DATA * wch;
int count = 1;
 
  lua_newtable(L);  /* table for the player name info */
  lua_newtable(L);  /* table for the player userdata info */
    
   for( wch = first_char; wch; wch = wch->next )
      if(!IS_NPC( wch ))
       {
       lua_pushstring (L, wch->name); 
       lua_rawseti (L, -3, count);  /* key is count */
       make_char_ud (L, wch);
       lua_rawseti (L, -2, count++);  /* key is count */
       }
   
  return 2;  /* first table is name, second is userdata */
}  /* end of L_players_in_game */


static int L_mobs_in_room (lua_State *L)
{
  
ROOM_INDEX_DATA *room = L_find_room (L, 1); 
CHAR_DATA * wch;
int count = 1;
  
  if (!room) 
    return 0;  /* oops - not in room or specified room does not exist */
  
  lua_newtable(L);  /* table for the mob vnum info */
  lua_newtable(L);  /* table for the mob userdata info */
    
  for( wch = room->first_person; wch; wch = wch->next_in_room )
    if(IS_NPC( wch ))
       {
       lua_pushnumber (L, wch->pIndexData->vnum); 
       lua_rawseti (L, -3, count);  /* key is count */
       make_char_ud (L, wch);
       lua_rawseti (L, -2, count++);  /* key is count */
       }
   
  return 2;
}  /* end of L_mobs_in_room */

static int L_room (lua_State *L)
{
  
CHAR_DATA * ch = L_find_character (L); /* get character pointer */

  if (!ch)
    return 0;
    
ROOM_INDEX_DATA * room = ch->in_room;  /* which room s/he is in */
 
  lua_pushnumber (L, room->vnum); 
  return 1;
}  /* end of L_room */

static int L_position (lua_State *L)
{
 
CHAR_DATA * ch = L_find_character (L); /* get character pointer */
const char * sPosition;

  if (!ch)
    return 0;
   
   switch ( ch->position )
   {
      case POS_DEAD:
         sPosition =  "slowly decomposing" ;
         break;
      case POS_MORTAL:
         sPosition =  "mortally wounded" ;
         break;
      case POS_INCAP:
         sPosition =  "incapacitated" ;
         break;
      case POS_STUNNED:
         sPosition =  "stunned" ;
         break;
      case POS_SLEEPING:
         sPosition =  "sleeping" ;
         break;
      case POS_RESTING:
         sPosition =  "resting" ;
         break;
      case POS_STANDING:
         sPosition =  "standing" ;
         break;
      case POS_FIGHTING:
         sPosition =  "fighting" ;
         break;
      case POS_MOUNTED:
         sPosition =  "mounted" ;
         break;
      case POS_SITTING:
         sPosition =  "sitting" ;
         break;

      default:
         sPosition =  "unknown" ;
         break;
   }
    
  lua_pushstring (L, sPosition);
  lua_pushnumber (L, ch->position); 
  return 2;
}  /* end of L_position */

static int L_char_name (lua_State *L)
{
  
CHAR_DATA * ch = L_find_character (L); /* get character pointer */

  if (!ch)
    return 0;
 
  lua_pushstring (L, ch->name); 
  return 1;
}  /* end of L_char_name */

static int L_npc (lua_State *L)
{
  
CHAR_DATA * ch = L_find_character (L); /* get character pointer */

  if (!ch)
    return 0;
 
  lua_pushboolean (L, IS_NPC (ch)); 
  return 1;
}  /* end of L_npc */

static int L_immortal (lua_State *L)
{
  
CHAR_DATA * ch = L_find_character (L); /* get character pointer */

  if (!ch)
    return 0;
 
  lua_pushboolean (L, IS_IMMORTAL (ch)); 
  return 1;
}  /* end of L_immortal */

static int L_char_exists (lua_State *L)
{
  
CHAR_DATA * ch = L_find_character (L); /* get character pointer */

  lua_pushboolean (L, ch != NULL);
  
  return 1;
}  /* end of L_char_exists */

static int L_msg_char (lua_State *L)
{
CHAR_DATA * ch;
int count = 0;
const char * name = luaL_checkstring (L, 1);
const char * sMessage = luaL_checkstring (L, 2);
int iColour = luaL_optnumber (L, 3, AT_PLAIN);

  if (iColour < 0 || iColour >= MAX_COLORS)
    luaL_error (L, "Colour %d is out of range 0 to %d", iColour, MAX_COLORS);

  for( ch = first_char; ch; ch = ch->next )
    if(!IS_NPC( ch ) &&
        ( strcasecmp( name, ch->name ) == 0 ) )
      break;        

  if (ch)
    {
    set_char_color (iColour, ch);
    send_to_char (sMessage, ch);
    send_to_char ("\r\n", ch);
    count++;
    }

  lua_pushnumber(L, count);
  
  return 1;
}  /* end of L_msg_char */

static int L_msg_room (lua_State *L)
{
luaL_checknumber (L, 1);  
ROOM_INDEX_DATA *room = L_find_room  (L, 1);
CHAR_DATA * ch;
int count = 0;
const char * sMessage = luaL_checkstring (L, 2);
int iColour = luaL_optnumber (L, 3, AT_PLAIN);

  if (iColour < 0 || iColour >= MAX_COLORS)
    luaL_error (L, "Colour %d is out of range 0 to %d", iColour, MAX_COLORS);

  if (room)
    {
    for( ch = room->first_person; ch; ch = ch->next_in_room )
     {
     if ( !IS_NPC( ch )  )
       {
        set_char_color (iColour, ch);
        send_to_char (sMessage, ch );
        send_to_char ("\r\n", ch);
        count++;         
       }   /* end character in room not NPC */
     }  /* end each character */
      
    }  /* end room found */

  lua_pushnumber(L, count);
  
  return 1;
}  /* end of L_msg_room */

static int L_msg_area (lua_State *L)
{
luaL_checknumber (L, 1);  
ROOM_INDEX_DATA *room = L_find_room  (L, 1);
CHAR_DATA * ch;
int count = 0;
const char * sMessage = luaL_checkstring (L, 2);
AREA_DATA *area;
int iColour = luaL_optnumber (L, 3, AT_PLAIN);

  if (iColour < 0 || iColour >= MAX_COLORS)
    luaL_error (L, "Colour %d is out of range 0 to %d", iColour, MAX_COLORS);

  if (room)
    {
    area = room->area;
    for( ch = first_char; ch; ch = ch->next)
     {
     if (ch->in_room->area == area && !IS_NPC( ch )  )
       {
        set_char_color (iColour, ch);
        send_to_char (sMessage, ch );
        send_to_char ("\r\n", ch);
        count++;         
       }   /* end character in area not NPC */
     }  /* end each character */
      
    }  /* end room found */

  lua_pushnumber(L, count);
  
  return 1;
}  /* end of L_msg_area */

static int L_msg_game (lua_State *L)
{
CHAR_DATA * ch;
int count = 0;
const char * sMessage = luaL_checkstring (L, 1);
int iColour = luaL_optnumber (L, 2, AT_PLAIN);

  if (iColour < 0 || iColour >= MAX_COLORS)
    luaL_error (L, "Colour %d is out of range 0 to %d", iColour, MAX_COLORS);


  for( ch = first_char; ch; ch = ch->next)
   {
   if (!IS_NPC( ch )  )
     {
     set_char_color (iColour, ch);
     send_to_char (sMessage, ch );
     send_to_char ("\r\n", ch);
     count++;         
     }   /* end character in game not NPC */
   }  /* end each character */

  lua_pushnumber(L, count);
  
  return 1;
}  /* end of L_msg_game */

static const struct luaL_reg mudlib [] = 
  {
  {"system_info", L_system_info},
  {"sysdata", L_sysdata},
  {"character_info", L_character_info},
  {"area_info", L_area_info},
  {"area_list", L_area_list},
  {"mob_info", L_mob_info},
  {"room_info", L_room_info},
  {"room_name", L_room_name},
  {"room_exits", L_room_exits},
  {"object_info", L_object_info},  /* object prototype */
  {"inventory", L_inventory},      /* invoked objects */
  {"equipped", L_equipped},        /* invoked objects */
  {"object_name", L_object_name},   /* short, long object name */
  {"mob_name", L_mob_name},   /* short, long mob name */
  {"players_in_room", L_players_in_room},  /* table of players in the room */
  {"players_in_game", L_players_in_game},  /* table of players in the game */
  {"mobs_in_room", L_mobs_in_room},  /* table of mobs in the room */
  {"level", L_level},                /* what is my level? */
  {"race", L_race},                  /* what is my race? */
  {"class", L_class},                /* what is my class? */
  {"room", L_room},                  /* what room am I in? */
  {"position", L_position},          /* character position */
  {"char_name", L_char_name},        /* what is my name? */
  {"char_exists", L_char_exists},    /* does character exist (now)? */
  
  /* do stuff to the character or others */
  
  {"send_to_char", L_send_to_char},  /* send text to character */
  {"set_char_color", L_set_char_color},  /* sets nominated output colour */
  {"interpret", L_interpret},        /* interpret command, as if we typed it */
  {"force", L_force},                /* force another character or mob to do something */
  {"transfer", L_transfer},          /* transfer arg1 to arg2 location */
      
  /* alter character state */
  
  {"gain_exp", L_gain_exp},         /* give character xp */
  {"gain_gold", L_gain_gold},       /* give character gold */
  {"stop_fighting", L_stop_fighting},  /* stop the character fighting */
  {"destroy_item", L_destroy_item},   /* destroy inventory item */
 
  /* make new things */
  
  {"oinvoke", L_oinvoke},           /* create an object for the player */
  {"obj_to_room", L_obj_to_room},   /* create an object for a room */
  {"minvoke", L_minvoke},           /* create a mobile instance */

/* get rid of stuff */
  
  {"purgemob", L_purgemob},           /* get rid of a mob */
  {"purgeobj", L_purgeobj},           /* get rid of an object from the room */

/* reset stuff */

  {"set_exit_state", L_set_exit_state},  /* close and/or lock exits */
  {"randomize_exits", L_randomize_exits},  /* randomize exits */
  {"make_trap", L_make_trap},           /* add trap object */
    
/* messages */

  {"msg_char", L_msg_char},           /* send to a particular character */
  {"msg_room", L_msg_room},           /* send to a room */
  {"msg_area", L_msg_area},           /* send to an area */
  {"msg_game", L_msg_game},           /* send to everyone */
    
  /* if checks */
  
  {"mobinworld", L_mobinworld},
  {"mobinarea", L_mobinarea},
  {"mobinroom", L_mobinroom},
  {"objinroom", L_objinroom},
  {"carryingvnum", L_carryingvnum},  /* are they carrying at least one of vnum? */
  {"possessvnum", L_possessvnum},    /* how many of vnum they possess */
  {"wearing", L_wearing},            /* do they have something equipped at this location? */
  {"wearingvnum", L_wearingvnum},    /* is vnum equipped? */
  {"fighting", L_fighting},
  {"npc", L_npc},
  {"immortal", L_immortal},
  
  
   {NULL, NULL}
  };  /* end of mudlib */
     
  
/* Mersenne Twister pseudo-random number generator */

static int L_mt_srand (lua_State *L)
{
int i;

/* allow for table of seeds */

  if (lua_istable (L, 1))
    {
    size_t length = lua_objlen (L, 1);  /* size of table */
    if (length == 0)
      luaL_error (L, "mt.srand table must not be empty");
    
    unsigned long * v = (unsigned long *) malloc (sizeof (unsigned long) * length);
    if (!v)
      luaL_error (L, "Cannot allocate memory for seeds table");

    for (i = 1; i <= length; i++)
      {
      lua_rawgeti (L, 1, i);  /* get number */
      if (!lua_isnumber (L, -1))
        {
        free (v);  /* get rid of table now */
        luaL_error (L, "mt.srand table must consist of numbers");
        }
      v [i - 1] = luaL_checknumber (L, -1);  
      lua_pop (L, 1);   /* remove value   */
      }    
    init_by_array (&v [0], length);
    free (v);  /* get rid of table now */
    }
  else
    init_genrand (luaL_checknumber (L, 1));
    
  return 0;
} /* end of L_mt_srand */
    
static int L_mt_rand (lua_State *L)
{
  lua_pushnumber (L, genrand ());
  return 1;
} /* end of L_mt_rand */

static const struct luaL_reg mtlib[] = {
 
  {"srand", L_mt_srand},  /* seed */
  {"rand", L_mt_rand},    /* generate */
  {NULL, NULL}
};

static int char2string (lua_State *L) 
  {
  lua_pushliteral (L, "mud_character");
  return 1;
  }
  
static const struct luaL_reg character_meta [] = 
  {
    {"__tostring", char2string},
    {NULL, NULL}
  };

static int RegisterLuaRoutines (lua_State *L)
  {
  time_t timer;
  time (&timer);

  lua_newtable (L);  /* environment */
  lua_replace (L, LUA_ENVIRONINDEX);

  /* this makes environment variable "character.state" by the pointer to our character */
  lua_settable(L, LUA_ENVIRONINDEX);

  /* register all mud.xxx routines */
  luaL_register (L, MUD_LIBRARY, mudlib);
  
  /* using interpret now
  RegisterLuaCommands (L);
  */

  luaopen_bits (L);     /* bit manipulation */
  luaL_register (L, MT_LIBRARY, mtlib);  /* Mersenne Twister */

  /* Marsenne Twister generator  */
  init_genrand (timer);
  
  /* colours, and other constants */
  add_lua_tables (L);

  /* meta table to identify character types */
  luaL_newmetatable(L, CHARACTER_META);
  luaL_register (L, NULL, character_meta);  /* give us a __tostring function */
  
  return 0;
  
}  /* end of RegisterLuaRoutines */

void open_lua  ( CHAR_DATA * ch)
  {
  lua_State *L = luaL_newstate ();   /* opens Lua */
  ch->L = L;
  
  if (ch->L == NULL)
    {
    bug ("Cannot open Lua state");
    return;  /* catastrophic failure */
    }

  luaL_openlibs (L);    /* open all standard libraries */

  /* call as Lua function because we need the environment  */
  lua_pushcfunction(L, RegisterLuaRoutines);
  lua_pushstring(L, CHARACTER_STATE);  /* push address */
  lua_pushlightuserdata(L, (void *)ch);    /* push value */
  lua_call(L, 2, 0);
 
  lua_getglobal (L, MUD_LIBRARY);
  make_char_ud (L, ch);
  lua_setfield (L, -2, "self");
  lua_pop (L, 1);  /* pop mud table */    

  /* run initialiation script */
  if (luaL_loadfile (L, LUA_STARTUP) ||
      CallLuaWithTraceBack (L, 0, 0))
      {
      bug ( "Error loading Lua startup file %s:\n %s", 
              LUA_STARTUP,
              lua_tostring(L, -1));
      }

  lua_settop (L, 0);    /* get rid of stuff lying around */
        
  }  /* end of open_lua */

void close_lua ( CHAR_DATA * ch)
  {
 
  if (ch->L == NULL)
   return;  /* never opened */

  lua_close (ch->L);
  ch->L = NULL;
   
  }/* end of close_lua */

static lua_State * find_lua_function (CHAR_DATA * ch, const char * fname)
  {
  lua_State *L = ch->L;
  
  if (!L || !fname)
    return NULL;  /* can't do it */
    
  /* find requested function */
  
  lua_getglobal (L, fname);  
  if (!lua_isfunction (L, -1))
    {
    lua_pop (L, 1);
    bug ("Warning: Lua script function '%s' does not exist", fname);
    return NULL;  /* not there */
    }
      
  return L;  
  }
  
extern char lastplayercmd[MAX_INPUT_LENGTH * 2];
  
static void call_lua_function (CHAR_DATA * ch, 
                                lua_State *L, 
                                const char * fname, 
                                const int nArgs)

{
  
  if (CallLuaWithTraceBack (L, nArgs, 0))
    {
     bug ("Error executing Lua function %s:\n %s\n** Last player command: %s", 
         fname, 
         lua_tostring(L, -1),
         lastplayercmd);
  
    if (ch && !IS_IMMORTAL(ch))
      {
      set_char_color( AT_YELLOW, ch );
      ch_printf (ch, "** A server scripting error occurred, please notify an administrator.\n"); 
      }  /* end of not immortal */
    
    
    lua_pop(L, 1);  /* pop error */
    
    }  /* end of error */
    
}  /* end of call_lua_function */


void call_lua (CHAR_DATA * ch, const char * fname, const char * argument)
  {
    
  int nArgs = 0;
  
  if (!ch || !fname)  /* note, argument is OPTIONAL */
    return;
      
  lua_State *L = find_lua_function (ch, fname);
   
  if (!L)
    return;
  
  /* if they want to send an argument, push it now */  
  if (argument)
    {
    lua_pushstring(L, argument);  /* push argument, if any */
    nArgs++;
    }
  
  call_lua_function (ch, L, fname, nArgs);
    
  }  /* end of call_lua */

void call_lua_num (CHAR_DATA * ch, const char * fname, const int argument)
  {
   
  if (!ch || !fname)
    return;
        
  lua_State *L = find_lua_function (ch, fname);
 
  if (!L)
    return;
       
  lua_pushnumber (L, argument);  
  
  call_lua_function (ch, L, fname, 1);
    
  }  /* end of call_lua_num */

/* call lua function with a victim name and numeric argument */
void call_lua_char_num (CHAR_DATA * ch, 
                        const char * fname, 
                        const char * victim, 
                        const int argument)
  {
  lua_State *L = find_lua_function (ch, fname);
 
  if (!L || !victim)
    return;
       
  lua_pushstring (L, victim);  /* push victim */
  lua_pushnumber (L, argument);  
  
  call_lua_function (ch, L, fname, 2);
    
  }  /* end of call_lua_char_num */

/* call lua function with a mob vnum and numeric argument */
void call_lua_mob_num (CHAR_DATA * ch, 
                        const char * fname, 
                        const int victim, 
                        const int argument)
  {
  lua_State *L = find_lua_function (ch, fname);
 
  if (!L)
    return;
       
  lua_pushnumber (L, victim);  /* push victim */
  lua_pushnumber (L, argument);  
  
  call_lua_function (ch, L, fname, 2);
    
  }  /* end of call_lua_mob_num */

  
static int RegisterGlobalLuaRoutines (lua_State *L)
  {
  time_t timer;
  time (&timer);

  /* register all mud.xxx routines */
  luaL_register (L, MUD_LIBRARY, mudlib);
  
  luaopen_bits (L);     /* bit manipulation */
  luaL_register (L, MT_LIBRARY, mtlib);  /* Mersenne Twister */

  /* Marsenne Twister generator  */
  init_genrand (timer);
  
  /* colours, and other constants */
  add_lua_tables (L);
  
  /* meta table to identify character types */
  luaL_newmetatable(L, CHARACTER_META);
  luaL_register (L, NULL, character_meta);  /* give us a __tostring function */
  
  return 0;
  
}  /* end of RegisterLuaRoutines */
  
void open_mud_lua (void)    /* set up Lua state - entire MUD */
  {
    
  L_mud = luaL_newstate ();   /* opens Lua */
  
  if (L_mud == NULL)
    {
    bug ("Cannot open Lua state");
    exit (1);  /* catastrophic failure */
    }

  luaL_openlibs (L_mud);    /* open all standard libraries */

  /* call as Lua function because we need the environment  */
  lua_pushcfunction(L_mud, RegisterGlobalLuaRoutines);
  lua_call(L_mud, 0, 0);
 

  /* run initialiation script */
  if (luaL_loadfile (L_mud, LUA_MUD_STARTUP) ||
      CallLuaWithTraceBack (L_mud, 0, 0))
      {
      bug ( "Error loading Lua startup file %s:\n %s", 
              LUA_MUD_STARTUP,
              lua_tostring(L_mud, -1));
      exit (1);  /* catastrophic failure */
      }

  lua_settop (L_mud, 0);    /* get rid of stuff lying around */    
    
  }  /* end of open_mud_lua */

void close_mud_lua (void)   /* close Lua state - entire MUD */  
{
  if (L_mud == NULL)
    return;  /* never opened */

  lua_close (L_mud);
  L_mud = NULL;   
}  /* end of close_mud_lua */

static int find_mud_lua_function (const char * fname)
  {
     
  if (!fname)
    return FALSE;
    
  /* find requested function */
  
  lua_getglobal (L_mud, fname);  
  if (!lua_isfunction (L_mud, -1))
    {
    lua_pop (L_mud, 1);
    bug ("Warning: Lua mud-wide script function '%s' does not exist", fname);
    return FALSE;  /* not there */
    }
      
  return TRUE;  
  }  /* end of find_mud_lua_function */
  
static int call_mud_lua_function (const char * fname, 
                                  const int nArgs)

{
  int iResult = 0;
 
  if (CallLuaWithTraceBack (L_mud, nArgs, 1))
    {
     bug ("Error executing mud-wide Lua function %s:\n %s\n** Last player command: %s", 
         fname, 
         lua_tostring(L_mud, -1),
         lastplayercmd);
    
    lua_pop(L_mud, 1);  /* pop error */
    
    }  /* end of error */
   
  if (lua_isnumber (L_mud, -1))
    iResult = luaL_checknumber (L_mud, -1);
  else if (lua_isboolean (L_mud, -1))
    iResult = lua_toboolean (L_mud, -1);

  return iResult;
}  /* end of call_lua_function */
  
int call_mud_lua (const char * fname, const char * argument)
  {
    
  int nArgs = 0;

  if (!argument || !fname)
    return FALSE;
  
  lua_settop (L_mud, 0);    /* get rid of stuff lying around */

  if (!find_mud_lua_function (fname))
    return 0;
  
  /* if they want to send an argument, push it now */  
  if (argument)
    {
    lua_pushstring(L_mud, argument);  /* push argument, if any */
    nArgs++;
    }
  
  return call_mud_lua_function (fname, nArgs);
    
  }  /* end of call_mud_lua */

/* call lua function with a string and numeric argument */
void call_mud_lua_char_num (const char * fname, 
                            const char * str, 
                            const int num)
  {
  if (!str || !fname)
    return;
       
  if (!find_mud_lua_function (fname))
    return;
 
  lua_pushstring (L_mud, str);  /* push string */
  lua_pushnumber (L_mud, num);  /* and number */
  
  call_mud_lua_function (fname, 2);
    
  }  /* end of call_mud_lua_char_num */
