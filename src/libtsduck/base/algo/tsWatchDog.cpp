//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2023, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------

#include "tsWatchDog.h"
#include "tsGuardMutex.h"


//----------------------------------------------------------------------------
// Constructor and destructor.
//----------------------------------------------------------------------------

ts::WatchDog::WatchDog(ts::WatchDogHandlerInterface* handler, ts::MilliSecond timeout, int id, ts::Report& log) :
    _log(log),
    _watchDogId(id),
    _handler(handler),
    _timeout(timeout == 0 ? Infinite : timeout)
{
}

ts::WatchDog::~WatchDog()
{
    // Terminate the thread and wait for actual thread termination.
    // Does nothing if the thread has not been started.
    _terminate = true;
    _condition.signal();
    waitForTermination();
}


//----------------------------------------------------------------------------
// Replace the watchdog handler.
//----------------------------------------------------------------------------

void ts::WatchDog::setWatchDogHandler(WatchDogHandlerInterface* h)
{
    GuardMutex lock(_mutex);
    _handler = h;
}


//----------------------------------------------------------------------------
// Activate the watchdog. Must be called with mutex held.
//----------------------------------------------------------------------------

void ts::WatchDog::activate(GuardCondition& lock)
{
    if (_started) {
        // Watchdog thread already started, signal the condition.
        lock.signal();
    }
    else {
        // Start the watchdog thread.
        _started = true;
        Thread::start();
    }
}


//----------------------------------------------------------------------------
// Set a new timeout value.
//----------------------------------------------------------------------------

void ts::WatchDog::setTimeout(MilliSecond timeout, bool autoStart)
{
    GuardCondition lock(_mutex, _condition);
    _timeout = timeout == 0 ? Infinite : timeout;
    _active = autoStart;
    if (autoStart) {
        activate(lock);
    }
}


//----------------------------------------------------------------------------
// Restart the watchdog, the previous timeout is canceled.
//----------------------------------------------------------------------------

void ts::WatchDog::restart()
{
    GuardCondition lock(_mutex, _condition);
    _active = true;
    activate(lock);
}


//----------------------------------------------------------------------------
// Suspend the watchdog, the previous timeout is canceled.
//----------------------------------------------------------------------------

void ts::WatchDog::suspend()
{
    GuardCondition lock(_mutex, _condition);
    _active = false;
    // Signal the condition if the thread is started.
    // No need to activate the thread if not started.
    lock.signal();
}


//----------------------------------------------------------------------------
// Invoked in the context of the server thread.
//----------------------------------------------------------------------------

void ts::WatchDog::main()
{
    _log.debug(u"Watchdog thread started, id %d", {_watchDogId});

    while (!_terminate) {
        bool expired = false;
        WatchDogHandlerInterface* h = nullptr;

        // Wait for the condition to be signaled. Get protected data while under mutex protection.
        {
            GuardCondition lock(_mutex, _condition);
            expired = !lock.waitCondition(_active ? _timeout : Infinite);
            h = _handler;
        }

        // Handle the expiration. No longer under mutex protection to avoid deadlocks in handler.
        if (!_terminate && expired && h != nullptr) {
            _log.debug(u"Watchdog expired, id %d", {_watchDogId});
            h->handleWatchDogTimeout(*this);
        }
    }

    _log.debug(u"Watchdog thread completed, id %d", {_watchDogId});
}
