/******************************************************************************
 *
 *  File        : signal.h
 *  Description : Signals
 *
 *****************************************************************************/
#ifndef __SIGNAL_H__
#define __SIGNAL_H__


    // Most signals handlers can be overridden or ignored. However, some can't

    #define SIGHUP       1
    #define SIGINT       2
    #define SIGQUIT      3
    #define SIGILL       4
    #define SIGTRAP      5
    #define SIGABRT      6
    #define SIGUNUSED    7
    #define SIGGFPE      8
    #define SIGKILL      9      /* Cannot be overridden or ignored */
    #define SIGUSR1     10
    #define SIGSEGV     11
    #define SIGUSR2     12
    #define SIGPIPE     13
    #define SIGALRM     14
    #define SIGTERM     15
    #define SIGSTKFLT   16
    #define SIGCHLD     17
    #define SIGCONT     18
    #define SIGSTOP     19      /* Cannot be overridden or ignored */


#endif //__SIGNAL_H__
