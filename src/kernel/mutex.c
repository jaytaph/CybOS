/******************************************************************************
 *
 *  File        : mutex.c
 *  Description : Mutual Exclusion functions
 *
 *****************************************************************************/
#include "mutex.h"

  // Found in mutexasm.asm
  extern int mutex_bts_asm (int *mutex_addr);
  extern int mutex_btr_asm (int *mutex_addr);

/*****************************************************************************
 * Tests mutex if it is locked, and if not locks the mutex. Note that this is
 * done with the BTS instruction which TEST and SETS it in the same instruction,
 * which makes it impossible to interrupt by another process which also uses
 * the same mutex.
 *
 * Note: Mutexes / Mutices (plural) can only be 1 or 0. There is no option
 *       of using 8 different mutexes in 1 byte. This is done to hold things
 *       fairly simple but it can be changed easily (but I don't see any reason
 *       yet why I should do that).
 *
 * In: address of the mutex
 *
 * Out:  0 mutex was free and is now locked by you. Continue with
 *         critical section
 *       1 mutex is already locked. Do not enter critical section.
 *
 */
int mutex_lock (int *mutex_addr)
{
  // Damn, can't seem to get the BTS function to work properly inside AT&T
  // style assembler :'(
  return mutex_bts_asm (mutex_addr);
}

/*****************************************************************************
 * Unlocks the mutex by setting it to 0 trough a atomic instruction.
 *
 * In: address of the mutex
 *
 * Out:  1 mutex was locked and is now freed by you.
 *       0 mutex is already unlocked.
 *
 */
int mutex_unlock (int *mutex_addr)
{
  return mutex_btr_asm (mutex_addr);
}

