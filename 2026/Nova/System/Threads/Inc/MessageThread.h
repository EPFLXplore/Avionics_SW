/*
 * MicroRosThread.h
 *
 *  Created on: 05 Dec 2025
 *      Author: Pedro Conde
 */
#pragma once

#include "Thread.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "packets.h"

#include <cstddef>      // std::size_t
#include <type_traits>  // std::is_trivially_copyable

/* Compiler-agnostic forced inline macro because without it inline is more a recommendation to the compiler than an obligation*/
#if defined(__GNUC__)
  #define ALWAYS_INLINE __attribute__((always_inline)) inline
#else
  #define ALWAYS_INLINE inline
#endif

// ----------------------------------------------------------------------------
// Default Configuration Constants
// ----------------------------------------------------------------------------
namespace ThreadCfg {
    constexpr uint32_t    TICK_DELAY_MS = 10;
    constexpr std::size_t QUEUE_DEPTH   = 5;
    /* 4096, matching Thread::STACK_BYTES. Asking for less does NOT save memory:
     * _stackBuffer is statically sized at 4 KB per thread whatever we request,
     * so 2048 left half of every thread's stack allocated and unreachable. */
    constexpr uint32_t    STACK_SIZE    = 4096;
}



/**
 * @brief Placeholder struct for MessageThread parameters that are not used.
 * For example, a thread that only publishes status can use MessageThread<EmptyMessage, MyStatus>.
 */
struct EmptyMessage {};

// ----------------------------------------------------------------------------
// MessageThread Template
// ----------------------------------------------------------------------------
template <typename CommandMsg, typename StatusMsg>
class MessageThread : public Thread {
    static_assert(std::is_trivially_copyable<CommandMsg>::value,
                  "CommandMsg must be trivially copyable to be used in FreeRTOS queues.");
    static_assert(std::is_trivially_copyable<StatusMsg>::value,
                  "StatusMsg must be trivially copyable to be used in FreeRTOS queues.");

private:
    // Fixed compile-time depth so the queue storage can be statically allocated.
    static constexpr std::size_t DEPTH = ThreadCfg::QUEUE_DEPTH;

    QueueHandle_t _commandQueue = nullptr;
    QueueHandle_t _statusQueue  = nullptr;

    // Static queue storage: no heap.
    StaticQueue_t _commandQueueCb{};
    StaticQueue_t _statusQueueCb{};
    uint8_t       _commandQueueStorage[DEPTH * sizeof(CommandMsg)]{};
    uint8_t       _statusQueueStorage[DEPTH * sizeof(StatusMsg)]{};

public:
    // ------------------------------------------------------------------------
    // "Full" constructor (internal workhorse)
    // ------------------------------------------------------------------------
    MessageThread(const char* name, osPriority priority, uint32_t stackSize, uint32_t tickDelayMs)
        : Thread(name, priority, stackSize)
    {
        // Set loop delay in base Thread
        setDelay(tickDelayMs);

        // Create inbox (command) queue: statically allocated
        _commandQueue = xQueueCreateStatic(DEPTH, sizeof(CommandMsg), _commandQueueStorage, &_commandQueueCb);
        configASSERT(_commandQueue);

        // Create outbox (status) queue: statically allocated
        _statusQueue = xQueueCreateStatic(DEPTH, sizeof(StatusMsg), _statusQueueStorage, &_statusQueueCb);
        configASSERT(_statusQueue);
    }

    // ------------------------------------------------------------------------
    // Constructors mirroring Thread base class
    // ------------------------------------------------------------------------

    /* Each fills in what the caller left out and forwards to the workhorse
     * above; the defaults are ThreadCfg's, never spelled out twice. */

    /// Thread(const char* name)
    MessageThread(const char* name)
        : MessageThread(name, (osPriority)osPriorityNormal, ThreadCfg::STACK_SIZE, ThreadCfg::TICK_DELAY_MS) {}

    /// Thread(const char* name, osPriority priority)
    MessageThread(const char* name, osPriority priority)
        : MessageThread(name, priority, ThreadCfg::STACK_SIZE, ThreadCfg::TICK_DELAY_MS) {}

    /// Thread(const char* name, uint32_t stackSize)
    MessageThread(const char* name, uint32_t stackSize)
        : MessageThread(name, (osPriority)osPriorityNormal, stackSize, ThreadCfg::TICK_DELAY_MS) {}

    /// Thread(const char* name, osPriority priority, uint32_t stackSize)
    MessageThread(const char* name, osPriority priority, uint32_t stackSize)
        : MessageThread(name, priority, stackSize, ThreadCfg::TICK_DELAY_MS) {}

    // ------------------------------------------------------------------------
    // Destructor
    // ------------------------------------------------------------------------
    virtual ~MessageThread() {
        if (_commandQueue) {
            vQueueDelete(_commandQueue);
            _commandQueue = nullptr;
        }
        if (_statusQueue) {
            vQueueDelete(_statusQueue);
            _statusQueue = nullptr;
        }
    }

    // ------------------------------------------------------------------------
    // Queue ends, split by WHO owns them
    // ------------------------------------------------------------------------
    //
    // Two queues, two directions, four ends - and each end has exactly one
    // legitimate caller:
    //
    //   _commandQueue   wire -> thread    push: SerialThread   pop: this thread
    //   _statusQueue    thread -> wire    push: this thread    pop: SerialThread
    //
    // All four used to be public, so nothing but discipline stopped a worker
    // from calling popStatus() and eating its own telemetry before the wire saw
    // it, or SerialThread from calling popCommand() and stealing a command from
    // the thread it was addressed to. Neither happens today; neither would have
    // been caught by the compiler if it started to.
    //
    // The access control below IS the direction rule, enforced at compile time:
    //   private + friend SerialThread -> the WIRE side. Only the wire owner.
    //   protected                     -> the WORKER side. Only the derived
    //                                    thread, and (because protected access
    //                                    is only granted through an object of
    //                                    the derived type) only on its OWN
    //                                    queues, never another thread's.
    // A call in the wrong direction is now a compile error, not a bug to find.

private:
    /* The one wire owner. It is the only thing outside this class hierarchy that
     * may touch a queue, and it may only touch the two ends that face the USB
     * link. Naming it here rather than leaving the methods public is what makes
     * "SerialThread owns the wire" a rule instead of a convention. */
    friend class SerialThread;

    /// Wire side. Non-blocking push to the command inbox (SerialThread only).
    ALWAYS_INLINE bool pushCommand(const CommandMsg& cmd) {
        if (_commandQueue) {
            return (xQueueSend(_commandQueue, &cmd, 0) == pdTRUE);
        }
        return false;
    }

    /// Wire side. Non-blocking pop from the status outbox (SerialThread only).
    ALWAYS_INLINE bool popStatus(StatusMsg& status) {
        if (_statusQueue) {
            return (xQueueReceive(_statusQueue, &status, 0) == pdTRUE);
        }
        return false;
    }

    /// Wire side. Blocking wait for status (SerialThread only, timeout in ticks).
    ALWAYS_INLINE bool waitStatus(StatusMsg& status, TickType_t timeout) {
        if (_statusQueue) {
            return (xQueueReceive(_statusQueue, &status, timeout) == pdTRUE);
        }
        return false;
    }

protected:
    /// Worker side. Non-blocking pop from this thread's command inbox.
    ALWAYS_INLINE bool popCommand(CommandMsg& cmd) {
        if (_commandQueue) {
            return (xQueueReceive(_commandQueue, &cmd, 0) == pdTRUE);
        }
        return false;
    }

    /// Worker side. Blocking wait for a command (timeout in ticks).
    ALWAYS_INLINE bool waitCommand(CommandMsg& cmd, TickType_t timeout) {
        if (_commandQueue) {
            return (xQueueReceive(_commandQueue, &cmd, timeout) == pdTRUE);
        }
        return false;
    }

    /// Worker side. Non-blocking push to this thread's status outbox.
    ALWAYS_INLINE bool pushStatus(const StatusMsg& status) {
        if (_statusQueue) {
            return (xQueueSend(_statusQueue, &status, 0) == pdTRUE);
        }
        return false;
    }

    // Optional: if a derived thread ever needs raw handles.
    ALWAYS_INLINE QueueHandle_t commandQueueHandle() const { return _commandQueue; }
    ALWAYS_INLINE QueueHandle_t statusQueueHandle()  const { return _statusQueue; }
};
