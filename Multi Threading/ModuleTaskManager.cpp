#include "ModuleTaskManager.h"
#include <iterator>


void ModuleTaskManager::threadMain()
{
	while (true)
	{
		// TODO 3:
		// - Wait for new tasks to arrive

		{//Protect the consulting of the Scheduled Tasks queue
			std::unique_lock <std::mutex> lock(mtx);

			while (scheduledTasks.empty())
			{
				event.wait(lock);
			}
		
			// - Retrieve a task from scheduledTasks
			Task* t = scheduledTasks.front();
			// - Execute it
			t->execute();
		   // - Insert it into finishedTasks
			finishedTasks.push(t);
			scheduledTasks.pop();			//Taking task out of the scheduled Queue
		}
		
	}
}

bool ModuleTaskManager::init()
{
	// TODO 1: Create threads (they have to execute threadMain())
	for (int i = 0; i < MAX_THREADS; ++i)
	{
		threads[i] = std::thread (&ModuleTaskManager::threadMain, this);
	}

	return true;
}

bool ModuleTaskManager::update()
{
	// TODO 4: Dispatch all finished tasks to their owner module (use Module::onTaskFinished() callback)
	while (!finishedTasks.empty())	//While queue has elements
	{
		(*finishedTasks.front()).owner->onTaskFinished(finishedTasks.front());	//Call onTaskFinished, then pop

		finishedTasks.pop();
	}

	return true;
}

bool ModuleTaskManager::cleanUp()
{
	// TODO 5: Notify all threads to finish and join them
	//Loop all threads calling join
	for (int i = 0; i < MAX_THREADS; ++i)
	{
		threads[i].join();
	}

	return true;
}

void ModuleTaskManager::scheduleTask(Task *task, Module *owner)
{
	{
		std::unique_lock <std::mutex> lock(mtx);
		task->owner = owner;

		// TODO 2: Insert the task into scheduledTasks so it is executed by some thread
		scheduledTasks.push(task);
		event.notify_one();
	}
}
