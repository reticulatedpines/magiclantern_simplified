/* Minimal example of clang's thread safety analysis in C
 * see http://clang.llvm.org/docs/ThreadSafetyAnalysis.html
 */

#include "mutex.h"

struct semaphore { } CAPABILITY("mutex");

void take_semaphore(struct semaphore * sem) ACQUIRE(sem) NO_THREAD_SAFETY_ANALYSIS
{
}

void give_semaphore(struct semaphore * sem) RELEASE(sem) NO_THREAD_SAFETY_ANALYSIS
{
}

struct semaphore * mux;
int x GUARDED_BY(mux);

void EXCLUDES(mux) inc()
{
    take_semaphore(mux);
    x++;
    give_semaphore(mux);
}

void EXCLUDES(mux) dec()
{
    //take_semaphore(mux);
    x--;
    //inc();
    //give_semaphore(mux);
}
