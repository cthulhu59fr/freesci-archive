/***************************************************************************
 kernel.c Copyright (C) 1999 Christoph Reichenbach


 This program may be modified and copied freely according to the terms of
 the GNU general public license (GPL), as long as the above copyright
 notice and the licensing information contained herein are preserved.

 Please refer to www.gnu.org for licensing details.

 This work is provided AS IS, without warranty of any kind, expressed or
 implied, including but not limited to the warranties of merchantibility,
 noninfringement, and fitness for a specific purpose. The author will not
 be held liable for any damage caused by this work or derivatives of it.

 By using this source code, you agree to the licensing terms as stated
 above.


 Please contact the maintainer for bug reports or inquiries.

 Current Maintainer:

    Christoph Reichenbach (CJR) [jameson@linuxgames.com]

***************************************************************************/

#include <engine.h>


sci_kernel_function_t kfunct_mappers[] = {
  {"Load", kLoad},
  {"UnLoad", kUnLoad},
  {"GameIsRestarting", kGameIsRestarting },
  {"NewList", kNewList },
  {"GetSaveDir", kGetSaveDir },
  {"GetCWD", kGetCWD },
  {"SetCursor", kSetCursor },
  {"FindKey", kFindKey },
  {"NewNode", kNewNode },
  {"AddToFront", kAddToFront },
  {"AddToEnd", kAddToEnd },
  {"Show", kShow },
  {"PicNotValid", kPicNotValid },
  {"Random", kRandom },
  {"Abs", kAbs },
  {"Sqrt", kSqrt },
  {"OnControl", kOnControl },
  {"HaveMouse", kHaveMouse },
  {"GetAngle", kGetAngle },
  {"GetDistance", kGetDistance },
  {"LastNode", kLastNode },
  {"FirstNode", kFirstNode },
  {"NextNode", kNextNode },
  {"PrevNode", kPrevNode },
  {"NodeValue", kNodeValue },
  {"Clone", kClone },
  {"DisposeClone", kDisposeClone },
  {"ScriptID", kScriptID },
  {"MemoryInfo", kMemoryInfo },
  {"DrawPic", kDrawPic },
  {"DisposeList", kDisposeList },
  {"DisposeScript", kDisposeScript },
  {"GetPort", kGetPort },
  {"SetPort", kSetPort },
  {"NewWindow", kNewWindow },
  {"DisposeWindow", kDisposeWindow },
  {"IsObject", kIsObject },
  {"Format", kFormat },
  {"DrawStatus", kDrawStatus },
  {"DrawMenuBar", kDrawMenuBar },
  {"AddMenu", kAddMenu },
  {"SetMenu", kSetMenu },
  {"AddToPic", kAddToPic },
  {"CelWide", kCelWide },
  {"CelHigh", kCelHigh },
  {"Display", kDisplay },
  {"Animate", kAnimate },
  {"GetTime", kGetTime },
  {"DeleteKey", kDeleteKey },
  {"StrLen", kStrLen },
  {"GetFarText", kGetFarText },
  {"StrEnd", kStrEnd },
  {"StrCat", kStrCat },
  {"StrCmp", kStrCmp },
  {"StrCpy", kStrCpy },
  {"StrAt", kStrAt },
  {"ReadNumber", kReadNumber },
  {"DrawControl", kDrawControl },
  {"NumCels", kNumCels },
  {"NumLoops", kNumLoops },
  {"TextSize", kTextSize },
  {"InitBresen", kInitBresen },
  {"DoBresen", kDoBresen },
  {"CanBeHere", kCanBeHere },
  {"DrawCel", kDrawCel },
  {"DirLoop", kDirLoop },
  {"CoordPri", kCoordPri },
  {"PriCoord", kPriCoord },
  {"ValidPath", kValidPath },
  {"RespondsTo", kRespondsTo },
  {"FOpen", kFOpen },
  {"FPuts", kFPuts },
  {"FGets", kFGets },
  {"FClose", kFClose },
  {"TimesSin", kTimesSin },
  {"SinMult", kTimesSin },
  {"TimesCos", kTimesCos },
  {"CosMult", kTimesCos },
  {"MapKeyToDir", kMapKeyToDir },
  {"GlobalToLocal", kGlobalToLocal },
  {"LocalToGlobal", kLocalToGlobal },
  {"Wait", kWait },
  {"CosDiv", kCosDiv },
  {"SinDiv", kSinDiv },
  {"BaseSetter", kBaseSetter },
  {"Parse", kParse },
  {"ShakeScreen", kShakeScreen },
#ifdef _WIN32
  {"DeviceInfo", kDeviceInfo_Win32},
#else /* !_WIN32 */
  {"DeviceInfo", kDeviceInfo_Unix},
#endif
  {"HiliteControl", kHiliteControl},
  {"GetMenu", kGetMenu},
  {"MenuSelect", kMenuSelect},
  {"GetEvent", kGetEvent },
  {"CheckFreeSpace", kCheckFreeSpace },
  {"DoSound", kDoSound },
  {"SetSynonyms", kSetSynonyms },
  {"FlushResources", kFlushResources },
  {"SetDebug", kSetDebug },
  {"GetSaveFiles", kGetSaveFiles },
  {"CheckSaveGame", kCheckSaveGame },
  {"SaveGame", kSaveGame },
  {"RestoreGame", kRestoreGame },
  {"SetJump", kSetJump },
  {"EditControl", kEditControl },
  {"EmptyList", kEmptyList },
  {"AddAfter", kAddAfter },
  {"RestartGame", kRestartGame },
  {"SetNowSeen", kSetNowSeen },
  {"Graph", kGraph },
  {"TimesTan", kTimesTan },
  {"TimesCot", kTimesCot },

  /* Experimental functions */
  {"Said", kSaid },
  /* Special and NOP stuff */
  {"DoAvoider", kNOP },
  {SCRIPT_UNKNOWN_FUNCTION_STRING, k_Unknown },
  {0,0} /* Terminator */
};



#define SCI_MAPPED_UNKNOWN_KFUNCTIONS_NR 0x72

static kfunct * unknown_function_map[SCI_MAPPED_UNKNOWN_KFUNCTIONS_NR] = { /* Map for unknown kernel functions */
/*0x00*/ kLoad,
/*0x01*/ kUnLoad,
/*0x02*/ kScriptID,
/*0x03*/ kDisposeScript,
/*0x04*/ kClone,
/*0x05*/ kDisposeClone,
/*0x06*/ kIsObject,
/*0x07*/ kRespondsTo,
/*0x08*/ kDrawPic,
/*0x09*/ kShow,
/*0x0a*/ kPicNotValid,
/*0x0b*/ kAnimate,
/*0x0c*/ kSetNowSeen,
/*0x0d*/ kNumLoops,
/*0x0e*/ kNumCels,
/*0x0f*/ kCelWide,
/*0x10*/ kCelHigh,
/*0x11*/ kDrawCel,
/*0x12*/ kAddToPic,
/*0x13*/ kNewWindow,
/*0x14*/ kGetPort,
/*0x15*/ kSetPort,
/*0x16*/ kDisposeWindow,
/*0x17*/ kDrawControl,
/*0x18*/ kHiliteControl,
/*0x19*/ kEditControl,
/*0x1a*/ kTextSize,
/*0x1b*/ kDisplay,
/*0x1c*/ kGetEvent,
/*0x1d*/ kGlobalToLocal,
/*0x1e*/ kLocalToGlobal,
/*0x1f*/ kMapKeyToDir,
/*0x20*/ kDrawMenuBar,
/*0x21*/ kMenuSelect,
/*0x22*/ kAddMenu,
/*0x23*/ kDrawStatus,
/*0x24*/ kParse,
/*0x25*/ kSaid,
/*0x26*/ kSetSynonyms,
/*0x27*/ kHaveMouse,
/*0x28*/ kSetCursor,
/*0x29*/ kFOpen,
/*0x2a*/ kFPuts,
/*0x2b*/ kFGets,
/*0x2c*/ kFClose,
/*0x2d*/ kSaveGame,
/*0x2e*/ kRestoreGame,
/*0x2f*/ kRestartGame,
/*0x30*/ kGameIsRestarting,
/*0x31*/ kDoSound,
/*0x32*/ kNewList,
/*0x33*/ kDisposeList,
/*0x34*/ kNewNode,
/*0x35*/ kFirstNode,
/*0x36*/ kLastNode,
/*0x37*/ kEmptyList,
/*0x38*/ kNextNode,
/*0x39*/ kPrevNode,
/*0x3a*/ kNodeValue,
/*0x3b*/ kAddAfter, /* AddAfter */
/*0x3c*/ kAddToFront,
/*0x3d*/ kAddToEnd,
/*0x3e*/ kFindKey,
/*0x3f*/ kDeleteKey,
/*0x40*/ kRandom,
/*0x41*/ kAbs,
/*0x42*/ kSqrt,
/*0x43*/ kGetAngle,
/*0x44*/ kGetDistance,
/*0x45*/ kWait,
/*0x46*/ kGetTime,
/*0x47*/ kStrEnd,
/*0x48*/ kStrCat,
/*0x49*/ kStrCmp,
/*0x4a*/ kStrLen,
/*0x4b*/ kStrCpy,
/*0x4c*/ kFormat,
/*0x4d*/ kGetFarText,
/*0x4e*/ kReadNumber,
/*0x4f*/ kBaseSetter,
/*0x50*/ kDirLoop,
/*0x51*/ kCanBeHere,
/*0x52*/ kOnControl,
/*0x53*/ kInitBresen,
/*0x54*/ kDoBresen,
/*0x55*/ kNOP, /* DoAvoider */
/*0x56*/ kSetJump,
/*0x57*/ kSetDebug,
/*0x58*/ NULL, /* kInspectObj */
/*0x59*/ NULL, /* ShowSends */
/*0x5a*/ NULL, /* ShowObjs */
/*0x5b*/ NULL, /* ShowFree */
/*0x5c*/ kMemoryInfo,
/*0x5d*/ NULL, /* StackUsage */
/*0x5e*/ NULL, /* Profiler */
/*0x5f*/ kGetMenu,
/*0x60*/ kSetMenu,
/*0x61*/ kGetSaveFiles,
/*0x62*/ kGetCWD,
/*0x63*/ kCheckFreeSpace,
/*0x64*/ kValidPath,
/*0x65*/ kCoordPri,
/*0x66*/ kStrAt,
#ifdef _WIN32
/*0x67*/ kDeviceInfo_Win32,
#else
/*0x67*/ kDeviceInfo_Unix,
#endif
/*0x68*/ kGetSaveDir,
/*0x69*/ kCheckSaveGame,
/*0x6a*/ kShakeScreen,
/*0x6b*/ kFlushResources,
/*0x6c*/ kTimesSin,
/*0x6d*/ kTimesCos,
/*0x6e*/ NULL,
/*0x6f*/ NULL,
/*0x70*/ kGraph,
/*0x71*/ kJoystick
};



const char *SCIk_Debug_Names[SCIk_DEBUG_MODES] = {
  "Stubs",
  "Lists and nodes",
  "Graphics",
  "Character handling",
  "Memory management",
  "Function parameter checks",
  "Bresenham algorithms",
  "Audio subsystem",
  "System graphics driver",
  "Base setter results",
  "Parser",
  "Menu handling",
  "Said specs",
  "File I/O"
};


/******************** Kernel Oops ********************/

int
kernel_oops(state_t *s, char *file, int line, char *reason)
{
  sciprintf("Kernel Oops in file %s, line %d: %s\n", file, line, reason);
  fprintf(stderr,"Kernel Oops in file %s, line %d: %s\n", file, line, reason);
  script_debug_flag = script_error_flag = 1;
  return 0;
}


/* Allocates a set amount of memory for a specified use and returns a handle to it. */
int
kalloc(state_t *s, int type, int space)
{
  int seeker = 0;

  while ((seeker < MAX_HUNK_BLOCKS) && (s->hunk[seeker].size))
    seeker++;

  if (seeker == MAX_HUNK_BLOCKS)
    KERNEL_OOPS("Out of hunk handles! Try increasing MAX_HUNK_BLOCKS in engine.h");
  else {
    s->hunk[seeker].data = g_malloc(s->hunk[seeker].size = space);
    s->hunk[seeker].type = type;
  }

  SCIkdebug(SCIkMEM, "Allocated %d at hunk %04x\n", space, seeker | (sci_memory << 11));

  return (seeker | (sci_memory << 11));
}


/* Returns a pointer to the memory indicated by the specified handle */
byte *
kmem(state_t *s, int handle)
{
  if ((handle >> 11) != sci_memory) {
    SCIkwarn(SCIkERROR, "Error: kmem() without a handle (%04x)\n", handle);
    return 0;
  }

  handle &= 0x7ff;

  if ((handle < 0) || (handle >= MAX_HUNK_BLOCKS)) {
    SCIkwarn(SCIkERROR, "Error: kmem() with invalid handle\n");
    return 0;
  }

  return s->hunk[handle & 0x7ff].data;
}

/* Frees the specified handle. Returns 0 on success, 1 otherwise. */
int
kfree(state_t *s, int handle)
{
  if ((handle >> 11) != sci_memory) {
    SCIkwarn(SCIkERROR, "Error: Attempt to kfree() non-handle\n");
    return 1;
  }

  SCIkdebug(SCIkMEM, "Freeing hunk %04x\n", handle);

  handle &= 0x7ff;

  if ((handle < 0) || (handle >= MAX_HUNK_BLOCKS)) {
    SCIkwarn(SCIkERROR, "Error: Attempt to kfree() with invalid handle\n");
    return 1;
  }

  if (s->hunk[handle].size == 0) {
    SCIkwarn(SCIkERROR, "Error: Attempt to kfree() non-allocated memory\n");
    return 1;
  }

  g_free(s->hunk[handle].data);
  s->hunk[handle].size = 0;

  return 0;
}


/*****************************************/
/************* Kernel functions **********/
/*****************************************/

char *old_save_dir;

void
kRestartGame(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  old_save_dir=strdup(s->heap+s->save_dir+2);
  s->restarting_flags |= SCI_GAME_IS_RESTARTING_NOW;
  s->restarting_flags &= ~SCI_GAME_WAS_RESTARTED_AT_LEAST_ONCE; /* This appears to help */
  script_abort_flag = 1; /* Force vm to abort ASAP */
}


/* kGameIsRestarting():
** Returns the restarting_flag in acc
*/
void
kGameIsRestarting(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  CHECK_THIS_KERNEL_FUNCTION;
  s->acc = (s->restarting_flags & SCI_GAME_WAS_RESTARTED);

  if ((old_save_dir)&&(s->save_dir))
    {
      strcpy(s->heap + s->save_dir + 2, old_save_dir);
      free(old_save_dir);
      old_save_dir = NULL;
    }
  if (argc) {/* Only happens during replay */
    if (!PARAM(0)) /* Set restarting flag */
      s->restarting_flags &= ~SCI_GAME_WAS_RESTARTED;
  }
}

void
kHaveMouse(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  CHECK_THIS_KERNEL_FUNCTION;

  s->acc = s->have_mouse_flag;
}



void
kMemoryInfo(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  switch (PARAM(0)) {
  case 0: s->acc = heap_meminfo(s->_heap); break;
  case 1: s->acc = heap_largest(s->_heap); break;
  case 2: /* Largest available hunk memory block */
  case 3: s->acc = 0xffff; break; /* Total amount of hunk memory */
  default: SCIkwarn(SCIkWARNING, "Unknown MemoryInfo operation: %04x\n", PARAM(0));
  }
}


void
k_Unknown(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  kfunct *funct = (funct_nr >= SCI_MAPPED_UNKNOWN_KFUNCTIONS_NR)? NULL : unknown_function_map[funct_nr];

  if (!funct) {
    CHECK_THIS_KERNEL_FUNCTION;
    SCIkwarn(SCIkSTUB, "Unhandled Unknown function %04x\n", funct_nr);
  } else funct(s, funct_nr, argc, argp);
}


void
kFlushResources(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  /* Nothing to do */
}

void
kSetDebug(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  sciprintf("Debug mode activated\n");

  script_debug_flag = 1; /* Enter debug mode */
  _debug_seeking = _debug_step_running = 0;
}

void
kGetTime(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  struct tm* loc_time;
  GTimeVal time_prec;
  time_t the_time;

  if (argc) { /* Get seconds since last am/pm switch */
    the_time = time(NULL);
    loc_time = localtime(&the_time);
    s->acc = loc_time->tm_sec + loc_time->tm_min * 60 + (loc_time->tm_hour % 12) * 3600;
  } else { /* Get time since game started */
    g_get_current_time (&time_prec);
    s-> acc = ((time_prec.tv_usec - s->game_start_time.tv_usec) * 60 / 1000000) +
      (time_prec.tv_sec - s->game_start_time.tv_sec) * 60;
  }
}


void
kstub(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  int i;

  SCIkwarn(SCIkWARNING, "Unimplemented syscall: %s[%x](", s->kernel_names[funct_nr], funct_nr);

  for (i = 0; i < argc; i++) {
    sciprintf("%04x", 0xffff & PARAM(i));
    if (i+1 < argc) sciprintf(", ");
  }
  sciprintf(")\n");
}


void
kNOP(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  CHECK_THIS_KERNEL_FUNCTION;
  SCIkwarn(SCIkWARNING, "Warning: Kernel function 0x%02x invoked: NOP\n");
}


void
script_map_kernel(state_t *s)
{
  int functnr;
  int mapped = 0;

  s->kfunct_table = g_malloc(sizeof(kfunct *) * (s->kernel_names_nr + 1));

  for (functnr = 0; functnr < s->kernel_names_nr; functnr++) {
    int seeker, found = -1;

    for (seeker = 0; (found == -1) && kfunct_mappers[seeker].functname; seeker++)
      if (strcmp(kfunct_mappers[seeker].functname, s->kernel_names[functnr]) == 0) {
	found = seeker; /* Found a kernel function with the same name! */
	mapped++;
      }

    if (found == -1) {

      sciprintf("Warning: Kernel function %s[%x] unmapped\n", s->kernel_names[functnr], functnr);
      s->kfunct_table[functnr] = kstub;

    } else s->kfunct_table[functnr] = kfunct_mappers[found].kernel_function;

  } /* for all functions requesting to be mapped */

  sciprintf("Mapped %d of %d kernel functions.\n", mapped, s->kernel_names_nr);

}
