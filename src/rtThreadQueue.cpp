/*

 pxCore Copyright 2005-2018 John Robinson

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

*/

// rtThreadQueue.h

#include "rtThreadQueue.h"
#include "pxTimer.h"

using namespace std;

rtThreadQueue::rtThreadQueue(){}
rtThreadQueue::~rtThreadQueue() {}

rtError rtThreadQueue::addTask(rtThreadTaskCB t, void* context, void* data)
{
  mTaskMutex.lock();
  rtThreadQueueEntry entry;
  entry.task = t;
  entry.context = context;
  entry.data = data;
  mTasks.push_back(entry);
  mTaskMutex.unlock();

  return RT_OK;
}

template<class It, class Pr>
It removeIf(It first, It last, Pr pred)
{
  It result = first;
  for (; first != last; ++first)
    if (!pred(*first))
      *result++ = move(*first);
  return result;
}

rtError rtThreadQueue::removeAllTasksForObject(void* context, rtThreadQueueRemoveTaskCB cb)
{
  mTaskMutex.lock();
  mTasks.erase(
    removeIf(
      mTasks.begin(),
      mTasks.end(),
      [context, cb](const rtThreadQueueEntry& e)
      {
        return (e.context == context) && (!cb || cb(e.context, e.data));
      }
    ),
    mTasks.end()
  );
  mTaskMutex.unlock();

  return RT_OK;
}

rtError rtThreadQueue::process(double maxSeconds)
{
  bool done = false;
  double start = pxSeconds();
  do
  {
    rtThreadQueueEntry entry;
    mTaskMutex.lock();
    if (!mTasks.empty())
    {
      entry = mTasks.front();
      mTasks.pop_front();
    }
    else done = true;
    mTaskMutex.unlock();
    
    if (!done)
    {
      entry.task(entry.context,entry.data);  
      if (maxSeconds > 0)
        done = (pxSeconds()-start) >= maxSeconds;
    }
  } while (!done);

  return RT_OK;
}
