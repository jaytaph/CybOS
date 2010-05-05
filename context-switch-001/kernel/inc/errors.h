/******************************************************************************
 *
 *  File        : errors.h
 *  Description : Contains the global error codes returned by the functions.
 *                While it worked correctly for some time, I ran into a lot of
 *                trouble lately. A function can return a number but can also
 *                return a error-code. Therefor I had to change the error-codes
 *                from positive to negative values to make a destinction
 *                between them. We must use a external error_code which contains
 *                the error-code. This is simple. But i'm not sure how to deal
 *                with it when we reach at the point that the kernel can be
 *                entered by multiple tasks. The errorcode is a kernel-variable,
 *                and not a task-specific variable (or is it?). Anyways.. must
 *                fix later.. :-(
 *
 *
 *****************************************************************************/
#ifndef __ERRORS_H__
#define __ERRORS_H__

  // Global defines
  #define ERR_ERROR                      -1    // Error :-(
  #define ERR_OK                          1    // Everything allrighty
  #define ERR_NOT_IMPLEMENTED             2

  // Console defines
  #define ERR_CON_INVALID_CONSOLE       100    // Invalid console
//  #define ERR_CON_INACTIVE              100    // Inactive console
//  #define ERR_CON_ACTIVE                101    // Active console
//  #define ERR_CON_CREATE                102    // Error while creating con
//  #define ERR_CON_SIZE                  103    // ?
//  #define ERR_CON_TABLEFULL             104    // Console table is full
//  #define ERR_CON_KEYBUFFULL            105    // Keyboard buffer is full
//  #define ERR_CON_NAME_NOT_FOUND        106    // Name not found in table

//  #define ERR_MEM_NOT_ENOUGH_KMEM       300    // No more memory to allocate

//  void error_seterror (int error);
//  int  error_geterror (void);

#endif // __ERRORS_H__
