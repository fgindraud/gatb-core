/*****************************************************************************
 *   GATB : Genome Assembly Tool Box                                         *
 *   Copyright (c) 2013                                                      *
 *                                                                           *
 *   GATB is free software; you can redistribute it and/or modify it under   *
 *   the CECILL version 2 License, that is compatible with the GNU General   *
 *   Public License                                                          *
 *                                                                           *
 *   This program is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the            *
 *   CECILL version 2 License for more details.                              *
 *****************************************************************************/

/** \file System.hpp
 *  \date 01/03/2013
 *  \author edrezen
 *  \brief Entry point class for accessing operating system operations
 */

#ifndef _GATB_CORE_SYSTEM_IMPL_SYSTEM_HPP_
#define _GATB_CORE_SYSTEM_IMPL_SYSTEM_HPP_

/********************************************************************************/

#include <gatb/system/impl/MemoryCommon.hpp>
#include <gatb/system/impl/TimeCommon.hpp>
#include <gatb/system/impl/SystemInfoCommon.hpp>
#include <gatb/system/impl/FileSystemLinux.hpp>
#include <gatb/system/impl/FileSystemMacos.hpp>
#include <gatb/system/impl/ThreadLinux.hpp>
#include <gatb/system/impl/ThreadMacos.hpp>

/********************************************************************************/
namespace gatb      {
namespace core      {
namespace system    {
/** \brief Implementation of Operating System abstraction layer */
namespace impl      {

/********************************************************************************/

#if 0
    #define MALLOC      System::memory().malloc
    #define CALLOC      System::memory().calloc
    #define REALLOC     System::memory().realloc
    #define FREE        System::memory().free
#else
    #define MALLOC      malloc
    #define CALLOC      calloc
    #define REALLOC     realloc
    #define FREE        free
#endif

/********************************************************************************/

 /** \brief Entry point providing access to operating system resources.
  *
  *  The IResource class provides a unique entry point for accessing different kinds
  *  of operating system resources (threads, time, file, etc...).
  *
  *  Normally, the client should use such an instance for getting OS resources
  *  instead of directly call system defendant functions. This is important because it
  *  will ease the build of client tools for different OS/architecture; the only thing
  *  to do is to use a specific instance of IResource for matching the correct OS.
  */
class System
 {
 public:

    /********************************************************************************/
    /** Access for info methods. */
    static ISystemInfo&  info  ()
    {
#ifdef __linux__
        static SystemInfoLinux instance;  return instance;
#endif

#ifdef __APPLE__
        static SystemInfoMacos instance;  return instance;
#endif

#ifdef __WINDOWS__
        #warning "TO BE DONE..."
#endif
    }

    /********************************************************************************/
     /** Access for time methods. */
     static ITime&  time ()
     {
#ifdef __linux__
        static TimeSystem instance (ITime::MSEC);  return instance;
#endif

#ifdef __APPLE__
        static TimeSystem instance (ITime::MSEC);  return instance;
#endif

#ifdef __WINDOWS__
        #warning "TO BE DONE..."
#endif
     }

     /********************************************************************************/
     /** Access for file methods. */
     static IFileSystem&     file    ()
     {
#ifdef __linux__
        static FileSystemLinux instance;  return instance;
#endif

#ifdef __APPLE__
        static FileSystemMacos instance;  return instance;
#endif

#ifdef __WINDOWS__
        #warning "TO BE DONE..."
#endif
     }

     /********************************************************************************/
     /** Access for memory methods. */
     static IMemory&         memory  ()
     {
#ifdef __linux__
        static MemorySizeStore instance (MemoryAllocatorStdlib::singleton(), MemoryOperationsCommon::singleton());  return instance;
#endif

#ifdef __APPLE__
        static MemorySizeStore instance (MemoryAllocatorStdlib::singleton(), MemoryOperationsCommon::singleton());  return instance;
#endif

#ifdef __WINDOWS__
        #warning "TO BE DONE..."
#endif
     }

     /********************************************************************************/
     /** Access for thread methods. */
     static IThreadFactory&  thread  ()
     {
#ifdef __linux__
        static ThreadFactoryLinux instance;  return instance;
#endif

#ifdef __APPLE__
        static ThreadFactoryMacos instance;  return instance;
#endif

#ifdef __WINDOWS__
        #warning "TO BE DONE..."
#endif
     }
};

/********************************************************************************/
} } } } /* end of namespaces. */
/********************************************************************************/

#endif /* _GATB_SYSTEM_IMPL_SYSTEM_HPP_ */