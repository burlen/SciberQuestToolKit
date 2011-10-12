#ifndef DebugUtil_h
#define DebugUtil_h

/**
Attach a gdb session to the listed ranks using a named interface.
The available interfaces are: konsole, emacs, and xterm.
*/
void GdbAttachRanks(char *iface, char *cRanks);

#endif

