/*
    SPDX-License-Identifier: MIT
    Copyright (c) 2026 EasiSoft (github.com/EasiSoft)

    File: signal.hpp
    Version: v0.1.0-alpha.1
    Author: RealRanger (github.com/RealRanger)
    Description: Minimal signal library for safe event handling.

*/

#ifndef SIGNAL_HPP
#define SIGNAL_HPP

// CONFIG
#ifndef SIGNAL_USE_EXCEPTIONS
#define SIGNAL_USE_EXCEPTIONS 0
#endif

#ifndef SIGNAL_DEBUG
#define SIGNAL_DEBUG 1
#endif

// IMPLEMENTATION
#if SIGNAL_DEBUG
    #include <iostream>
#endif
#include <cstddef>
#include <vector>    
#include <functional>

namespace easi {

// Forward declarations
template<typename Signal>
class Connection;

// Private
namespace core {

template<typename... Args>
struct Event {
    // Dirty events are skipped until the next emit cycle.
    bool dirty;
    std::function<void(Args...)> callback;
};

} // namespace core

// Public
// Set the owner to the class that creates the signal 
// so it is able to call emit().
template<typename Owner, typename... Args>
class Signal {
    friend Owner;

public:
    explicit Signal() = default;

    /**
     * @brief Connects a callback to the signal.
     *
     * If called during emission, the callback is deferred until the next cycle.
     *
     * @param callback Function to invoke when the signal is emitted.
     * @return A Connection object that can later disconnect the callback.
     */
    Connection<Signal> connect(std::function<void(Args...)> callback) {
        if (is_emitting) {
            event_vector.push_back(core::Event<Args...>{true, callback});
        } else {
            event_vector.push_back(core::Event<Args...>{false, callback});
        }

        size_t index = event_vector.size() - 1;
        std::function<void(size_t)> sig_disconnect = [this](size_t index) {
            disconnect(index);
        };

        return Connection(*this, index, sig_disconnect);
    }

private:
    /**
     * @brief Emits all connected callbacks with the given arguments.
     *
     * Skips "dirty" events until the next cycle, ensuring safe iteration
     * while connections are added or removed during emission.
     *
     * @param args Arguments forwarded to each callback.
     */
    void emit(Args... args) {
        is_emitting = true ;

        for (auto& e : event_vector) {
            if (!e.callback) {
                continue;
            }

            if (e.dirty) {
                dirty_event_vector.push_back(e);
                continue;
            }

            e.callback(args...);
        }

        is_emitting = false;

        for (auto& e : dirty_event_vector) {
            e.dirty = false;
        }
        dirty_event_vector.clear();
    }

    /**
     * @brief Disconnects a callback from the signal.
     *
     * Marks the event as inactive without removing it, so vector indices
     * remain stable during emission.
     *
     * @param index Position of the callback in the event vector.
     */
    void disconnect(size_t index) {
        // Avoid shifting the vector index by setting to inactive instead of removing.
        event_vector[index].callback = nullptr;
    }

    bool is_emitting = false;
    std::vector<core::Event<Args...>> event_vector;
    std::vector<core::Event<Args...>> dirty_event_vector;
};

// Each signal type is defined as a template, so we must know
// the specific type at compile time to handle it correctly.
template<typename Signal>
class Connection {
public:
    explicit Connection(Signal& signal, size_t index, std::function<void(size_t)> disconnect)
        : signal(signal), index(index), sig_disconnect(disconnect), is_connected(true) {}
    
    void disconnect() {
        if (!is_connected) {
            return;
        }

        sig_disconnect(index);
        is_connected = false;
    }

    explicit operator bool() const {
        return is_connected;
    }

private:
    Signal& signal;
    size_t index;
    bool is_connected;

    std::function<void(size_t)> sig_disconnect;
};

} // namespace easi

#endif // EVENTS_HPP