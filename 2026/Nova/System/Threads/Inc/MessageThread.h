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

// ----------------------------------------------------------------------------
// Compiler-agnostic forced inline macro
// ----------------------------------------------------------------------------
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
    constexpr std::size_t QUEUE_DEPTH   = 50;
    constexpr uint32_t    STACK_SIZE    = 2048;  // match DEFAULT_STACK_SIZE idea
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
    MessageThread(const char*   name,
                  osPriority    priority,
                  uint32_t      stackSize,
                  uint32_t      tickDelayMs,
                  std::size_t   commandDepth,
                  std::size_t   statusDepth)
        : Thread(name, priority, stackSize)
    {
        // Set loop delay in base Thread
        setDelay(tickDelayMs);

        // Depths are fixed at DEPTH so the storage can be static; the runtime
        // depth params are kept for API compatibility but clamped to DEPTH.
        (void)commandDepth;
        (void)statusDepth;

        // Create inbox (command) queue: statically allocated
        _commandQueue = xQueueCreateStatic(DEPTH, sizeof(CommandMsg),
                                           _commandQueueStorage, &_commandQueueCb);
        configASSERT(_commandQueue);

        // Create outbox (status) queue: statically allocated
        _statusQueue = xQueueCreateStatic(DEPTH, sizeof(StatusMsg),
                                          _statusQueueStorage, &_statusQueueCb);
        configASSERT(_statusQueue);
    }

    // ------------------------------------------------------------------------
    // Constructors mirroring Thread base class
    // ------------------------------------------------------------------------

    /// Thread(const char* name)
    MessageThread(const char* name)
        : MessageThread(
              name,
              (osPriority)osPriorityNormal,      // default priority
              ThreadCfg::STACK_SIZE,             // default stack size
              ThreadCfg::TICK_DELAY_MS,          // default loop period
              ThreadCfg::QUEUE_DEPTH,            // default command depth
              ThreadCfg::QUEUE_DEPTH)            // default status depth
    {}



    /// Thread(const char* name, osPriority priority)
    MessageThread(const char* name, osPriority priority)
        : MessageThread(
              name,
              priority,
              ThreadCfg::STACK_SIZE,
              ThreadCfg::TICK_DELAY_MS,
              ThreadCfg::QUEUE_DEPTH,
              ThreadCfg::QUEUE_DEPTH)
    {}

    /// Thread(const char* name, uint32_t stackSize)
    MessageThread(const char* name, uint32_t stackSize)
        : MessageThread(
              name,
              (osPriority)osPriorityNormal,
              stackSize,
              ThreadCfg::TICK_DELAY_MS,
              ThreadCfg::QUEUE_DEPTH,
              ThreadCfg::QUEUE_DEPTH)
    {}

    /// Thread(const char* name, osPriority priority, uint32_t stackSize)
    MessageThread(const char* name, osPriority priority, uint32_t stackSize)
        : MessageThread(
              name,
              priority,
              stackSize,
              ThreadCfg::TICK_DELAY_MS,
              ThreadCfg::QUEUE_DEPTH,
              ThreadCfg::QUEUE_DEPTH)
    {}

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
    // Command Inbox API (ROS/Main -> Thread)
    // ------------------------------------------------------------------------

    /// Non-blocking push to the command inbox.
    ALWAYS_INLINE bool pushCommand(const CommandMsg& cmd) {
        if (_commandQueue) {
            return (xQueueSend(_commandQueue, &cmd, 0) == pdTRUE);
        }
        return false;
    }

    /// Non-blocking pop from command inbox.
    ALWAYS_INLINE bool popCommand(CommandMsg& cmd) {
        if (_commandQueue) {
            return (xQueueReceive(_commandQueue, &cmd, 0) == pdTRUE);
        }
        return false;
    }

    /// Blocking wait for a command (timeout in ticks).
    ALWAYS_INLINE bool waitCommand(CommandMsg& cmd, TickType_t timeout) {
        if (_commandQueue) {
            return (xQueueReceive(_commandQueue, &cmd, timeout) == pdTRUE);
        }
        return false;
    }

    // ------------------------------------------------------------------------
    // Status Outbox API (Thread -> ROS/Main)
    // ------------------------------------------------------------------------

    /// Non-blocking push to status outbox.
    ALWAYS_INLINE bool pushStatus(const StatusMsg& status) {
        if (_statusQueue) {
            return (xQueueSend(_statusQueue, &status, 0) == pdTRUE);
        }
        return false;
    }

    /// Non-blocking pop from status outbox.
    ALWAYS_INLINE bool popStatus(StatusMsg& status) {
        if (_statusQueue) {
            return (xQueueReceive(_statusQueue, &status, 0) == pdTRUE);
        }
        return false;
    }

    /// Blocking wait for status (timeout in ticks).
    ALWAYS_INLINE bool waitStatus(StatusMsg& status, TickType_t timeout) {
        if (_statusQueue) {
            return (xQueueReceive(_statusQueue, &status, timeout) == pdTRUE);
        }
        return false;
    }

protected:
    // Optional: if a derived thread ever needs raw handles.
    ALWAYS_INLINE QueueHandle_t commandQueueHandle() const { return _commandQueue; }
    ALWAYS_INLINE QueueHandle_t statusQueueHandle()  const { return _statusQueue; }
};
