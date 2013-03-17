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
 *			 Lua Constants Tables     by Nick Gammon                  			    *
 ****************************************************************************/
 
/*

Lua scripting written by Nick Gammon
Date: 8th July 2007

You are welcome to incorporate this code into your MUD codebases.

Post queries at: http://www.gammon.com.au/forum/


*/


#include <stdio.h>
#include <string.h>
#include "mud.h"

#include <lualib.h>
#include <lauxlib.h>

#define AT_TABLE "at"
#define OBJ_ITEM_TYPE_TABLE "itype"

typedef struct { const char* key; int val; } flags_pair;

static int MakeFlagsTable (lua_State *L, 
                           const char *name,
                           const flags_pair *arr)
{
  const flags_pair *p;
  lua_newtable(L);
  for(p=arr; p->key != NULL; p++) {
    lua_pushstring(L, p->key);
    lua_pushnumber(L, p->val);
    lua_rawset(L, -3);
  }
  lua_setglobal (L, name);
  return 1;
}

/* we can use at.orange (for example) as an argument to mud.set_char_color */

static flags_pair at_colours_table[] =
{
   { "black",        AT_BLACK        },  { "rmname",   AT_RMNAME   },
   { "blood",        AT_BLOOD        },  { "rmdesc",   AT_RMDESC   },
   { "dgreen",       AT_DGREEN       },  { "object",   AT_OBJECT   },
   { "orange",       AT_ORANGE       },  { "person",   AT_PERSON   },
   { "dblue",        AT_DBLUE        },  { "list",     AT_LIST     },
   { "purple",       AT_PURPLE       },  { "bye",      AT_BYE      },
   { "cyan",         AT_CYAN         },  { "gold",     AT_GOLD     },
   { "grey",         AT_GREY         },  { "gtell",    AT_GTELL    },
   { "dgrey",        AT_DGREY        },  { "note",     AT_NOTE     },
   { "red",          AT_RED          },  { "hungry",   AT_HUNGRY   },
   { "green",        AT_GREEN        },  { "thirsty",  AT_THIRSTY  },
   { "yellow",       AT_YELLOW       },  { "fire",     AT_FIRE     },
   { "blue",         AT_BLUE         },  { "sober",    AT_SOBER    },
   { "pink",         AT_PINK         },  { "wearoff",  AT_WEAROFF  },
   { "lblue",        AT_LBLUE        },  { "exits",    AT_EXITS    },
   { "white",        AT_WHITE        },  { "score",    AT_SCORE    },
   { "blink",        AT_BLINK        },  { "reset",    AT_RESET    },
     { "log",      AT_LOG      },
     { "diemsg",   AT_DIEMSG   },
     { "wartalk",  AT_WARTALK  },
     { "arena",    AT_ARENA    },
     { "muse",     AT_MUSE     },
     { "think",    AT_THINK    },
     { "aflags",   AT_AFLAGS   },
     { "who",      AT_WHO      },
     { "racetalk", AT_RACETALK },
     { "ignore",   AT_IGNORE   },
     { "whisper",  AT_WHISPER  },
     { "divider",  AT_DIVIDER  },
     { "morph",    AT_MORPH    },
     { "shout",    AT_SHOUT    },
     { "rflags",   AT_RFLAGS   },
     { "stype",    AT_STYPE    },
   { "plain",        AT_PLAIN        },  { "aname",    AT_ANAME    },
   { "action",       AT_ACTION       },  { "auction",  AT_AUCTION  },
   { "say",          AT_SAY          },  { "score2",   AT_SCORE2   },
   { "gossip",       AT_GOSSIP       },  { "score3",   AT_SCORE3   },
   { "yell",         AT_YELL         },  { "score4",   AT_SCORE4   },
   { "tell",         AT_TELL         },  { "who2",     AT_WHO2     },
   { "hit",          AT_HIT          },  { "who3",     AT_WHO3     },
   { "hitme",        AT_HITME        },  { "who4",     AT_WHO4     },
   { "immort",       AT_IMMORT       },  { "intermud", AT_INTERMUD },
   { "hurt",         AT_HURT         },  { "help",     AT_HELP     },
   { "falling",      AT_FALLING      },  { "who5",     AT_WHO5     },
   { "danger",       AT_DANGER       },  { "score5",   AT_SCORE5   },
   { "magic",        AT_MAGIC        },  { "who6",     AT_WHO6     },
   { "consider",     AT_CONSIDER     },  { "who7",     AT_WHO7     },
   { "report",       AT_REPORT       },  { "prac",     AT_PRAC     },
   { "poison",       AT_POISON       },  { "prac2",    AT_PRAC2    },
   { "social",       AT_SOCIAL       },  { "prac3",    AT_PRAC3    },
   { "dying",        AT_DYING        },  { "prac4",    AT_PRAC4    },
   { "dead",         AT_DEAD         },  { "mxpprompt",AT_MXPPROMPT},
   { "skill",        AT_SKILL        },  { "guildtalk",AT_GUILDTALK},
   { "carnage",      AT_CARNAGE      },  
   { "damage",       AT_DAMAGE       },  
   { "flee",         AT_FLEE         },  
  
   { NULL, 0 }
};

/* object types */

static flags_pair object_types_table[] =
{

{ "armor",       ITEM_ARMOR      }, 
{ "blood",       ITEM_BLOOD      }, { "money",          ITEM_MONEY         },
{ "bloodstain",  ITEM_BLOODSTAIN }, { "none",           ITEM_NONE          },
{ "boat",        ITEM_BOAT       }, { "note",           ITEM_NOTE          },
{ "book",        ITEM_BOOK       }, 
{ "button",      ITEM_BUTTON     }, { "oil",            ITEM_OIL           },
 { "oldtrap",        ITEM_OLDTRAP       },
{ "container",   ITEM_CONTAINER  }, { "paper",          ITEM_PAPER         },
 { "pen",            ITEM_PEN           },
{ "corpse_npc",  ITEM_CORPSE_NPC }, { "pill",           ITEM_PILL          },
{ "corpse_pc",   ITEM_CORPSE_PC  }, { "pipe",           ITEM_PIPE          },
{ "dial",        ITEM_DIAL       }, { "portal",         ITEM_PORTAL        },
{ "disease",     ITEM_DISEASE    }, { "potion",         ITEM_POTION        },
{ "drink_con",   ITEM_DRINK_CON  }, 
 { "pullchain",      ITEM_PULLCHAIN     },
 { "quiver",         ITEM_QUIVER        },
{ "fire",        ITEM_FIRE       }, { "rune",           ITEM_RUNE          },
{ "fireweapon",  ITEM_FIREWEAPON }, { "runepouch",      ITEM_RUNEPOUCH     },
{ "food",        ITEM_FOOD       }, { "salve",          ITEM_SALVE         },
{ "fountain",    ITEM_FOUNTAIN   }, { "scraps",         ITEM_SCRAPS        },
{ "fuel",        ITEM_FUEL       }, { "scroll",         ITEM_SCROLL        },
{ "furniture",   ITEM_FURNITURE  }, { "shovel",         ITEM_SHOVEL        },
{ "herb",        ITEM_HERB       }, { "spike",          ITEM_SPIKE         },
{ "herb_con",    ITEM_HERB_CON   }, { "staff",          ITEM_STAFF         },
{ "incense",     ITEM_INCENSE    }, { "switch",         ITEM_SWITCH        },
{ "key",         ITEM_KEY        }, { "tinder",         ITEM_TINDER        },
 { "trap",           ITEM_TRAP          },
{ "lever",       ITEM_LEVER      }, { "trash",          ITEM_TRASH         },
{ "light",       ITEM_LIGHT      }, { "treasure",       ITEM_TREASURE      },
{ "lockpick",    ITEM_LOCKPICK   }, { "wand",           ITEM_WAND          },
{ "map",         ITEM_MAP        }, { "weapon",         ITEM_WEAPON        },
{ "match",       ITEM_MATCH      }, { "worn",           ITEM_WORN          },
{ "missile",     ITEM_MISSILE    },
 
   { NULL, 0 }
};

void add_lua_tables (lua_State *L)
  {
  
  /* AT colours */
  MakeFlagsTable (L, AT_TABLE, at_colours_table);

  /* Object item types */
  MakeFlagsTable (L, OBJ_ITEM_TYPE_TABLE, object_types_table);
  
  }
