// ======================================================================== //
// Copyright 2009-2015 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#include "taskscheduler_new.h"
#include "math/math.h"
#include "sys/sysinfo.h"

namespace embree
{
  TaskSchedulerNew::TaskSchedulerNew(size_t numThreads)
    : threadCount(numThreads), createThreads(true), terminate(false), anyTasksRunning(0), active(false)
  {
    for (size_t i=0; i<MAX_THREADS; i++)
      threadLocal[i] = NULL;

    if (numThreads == -1) {
      threadCount = 1;
      createThreads = false;
    }
    else if (numThreads == 0) {
      threadCount = getNumberOfLogicalThreads();
    }
  }
  
  TaskSchedulerNew::~TaskSchedulerNew() 
  {
    /* let all threads leave the thread loop */
    terminateThreadLoop();

    /* destroy all threads that we created */
    destroyThreads();
  }

  void TaskSchedulerNew::startThreads()
  {
    createThreads = false;
    for (size_t i=1; i<threadCount; i++)
      threads.push_back(std::thread([i,this]() { thread_loop(i); }));
  }

  void TaskSchedulerNew::terminateThreadLoop()
  {
    /* signal threads to terminate */
    mutex.lock();
    terminate = true;
    mutex.unlock();
    condition.notify_all();
  }

  void TaskSchedulerNew::destroyThreads() 
  {
    /* wait for threads to terminate */
    for (size_t i=0; i<threads.size(); i++) 
      threads[i].join();
  }

  void TaskSchedulerNew::join()
  {
    size_t threadIndex = atomic_add(&threadCount,1);
    assert(threadIndex < MAX_THREADS);
    thread_loop(threadIndex);
  }

  __thread TaskSchedulerNew::Thread* TaskSchedulerNew::thread_local_thread = NULL;

  TaskSchedulerNew::Thread* TaskSchedulerNew::thread() {
    return thread_local_thread;
  }

  void TaskSchedulerNew::thread_loop(size_t threadIndex) try 
  {
    /* allocate thread structure */
    Thread thread(threadIndex,this);
    threadLocal[threadIndex] = &thread;
    thread_local_thread = &thread;

    /* main thread loop */
    while (!terminate)
    {
      /* all threads are waiting inside some condition */
      {
        std::unique_lock<std::mutex> lock(mutex);
        condition.wait(lock, [&] () { return anyTasksRunning || terminate; });
      }
      if (terminate) break;
      
      /* work on available task */
      while (anyTasksRunning) 
      {
	if (thread.scheduler->steal_from_other_threads(thread)) 
	{
	  atomic_add(&anyTasksRunning,+1);
	  while (thread.tasks.execute_local(thread,NULL));
	  atomic_add(&anyTasksRunning,-1);
	}
      }
    }
  }
  catch (const std::exception& e) {
    std::cout << "Error: " << e.what() << std::endl; // FIXME: propagate to main thread
    exit(1);
  }

  bool TaskSchedulerNew::steal_from_other_threads(Thread& thread)
  {
    const size_t threadIndex = thread.threadIndex;
    const size_t threadCount = this->threadCount;

    for (size_t i=1; i<threadCount; i++) 
    //for (size_t i=1; i<5; i++) 
    {
      __pause_cpu();
      size_t otherThreadIndex = threadIndex+i;
      if (otherThreadIndex >= threadCount) otherThreadIndex -= threadCount;

      if (!threadLocal[otherThreadIndex])
        continue;
      
      if (threadLocal[otherThreadIndex]->tasks.steal(thread)) {
        return true;
      }
    }

    return false;
  }
}
