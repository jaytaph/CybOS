/******************************************************************************
 *
 *  File        : errors.h
 *  Description : Error resolve functions for the kernel. Each task holds its
 *                own error code.
 *
 *****************************************************************************/
#include "schedule.h"
#include "errors.h"

/********************************************************************
 * Sets the error inside the current task
 */
void error_seterror (int error)
{
  _current_task->error = error;
}

/********************************************************************
 * Returns current error of the current task
 *
 * Out: current error status
 */
int error_geterror (void)
{
  return _current_task->error;
}

