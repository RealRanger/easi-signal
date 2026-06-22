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
#define SIGNAL_DEBUG 0
#endif

// IMPLEMENTATION
#if SIGNAL_DEBUG
#include <iostream>
#include <chrono>
#endif
#include <cstddef>
#include <vector>    
#include <functional>
#include <memory>

namespace easi {

// Forward declarations
template<typename Signal>
class Connection;

template<typename Owner, typename... Args>
class Signal;

// Private

#if SIGNAL_DEBUG
namespace debug_tools {

class Terminal {
public:
    Terminal() =  default;

    void send_debug(std::string origin, std::string message) {
        std::cout << "[debug] " << "[" << origin << "]: " << message << std::endl;
    }
};

class Timer {
public:
    Timer() : start_time(), end_time() {}

    void start() {
        start_time = std::chrono::high_resolution_clock::now();
    }

    void stop() {
        end_time = std::chrono::high_resolution_clock::now();
    }

    double elapsed_ms() {
        return std::chrono::duration<double, std::milli>(end_time - start_time).count();
    }

private:
    std::chrono::high_resolution_clock::time_point start_time;
    std::chrono::high_resolution_clock::time_point end_time;
};

debug_tools::Terminal terminal;

}
#endif

namespace core {

template<typename Owner, typename... Args>
struct Event {
    // Dirty events are skipped until the next emit cycle.
    bool dirty;
    std::function<void(Args...)> callback;
    Connection<Signal<Owner, Args...>> conn;

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
    Connection<Signal<Owner, Args...>> connect(std::function<void(Args...)> callback) {
        bool is_dirty;

        if (is_emitting) {
            is_dirty = true;
        } else {
            is_dirty = false;
        }
       
        size_t index = event_vector.size();
        std::function<void(size_t)> sig_disconnect = [this](size_t index) {
            disconnect(index);
        };
        Connection conn(*this, index, sig_disconnect);

        event_vector.push_back(core::Event<Owner, Args...>{is_dirty, callback, conn});

        #if SIGNAL_DEBUG
            debug_tools::terminal.send_debug("easi::Signal::connect", "Created connection");
        #endif

        return conn;
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

        #if SIGNAL_DEBUG
            debug_tools::terminal.send_debug("easi::Signal::emit", "Emitted callbacks");
        #endif

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
    std::vector<core::Event<Owner, Args...>> event_vector;
    std::vector<core::Event<Owner, Args...>> dirty_event_vector;
};

// Each signal type is defined as a template, so we must know
// the specific type at compile time to handle it correctly.
template<typename Signal>
class Connection {
public:
    explicit Connection(Signal& signal, size_t index, std::function<void(size_t)> disconnect)
        : signal(signal), index(index), sig_disconnect(disconnect), is_connected(std::make_shared<bool>(true)) {}
    
    void disconnect() {
        if (!*is_connected) {
            return;
        }

        sig_disconnect(index);
        *is_connected = false;

        #if SIGNAL_DEBUG
            debug_tools::terminal.send_debug("easi::Connection::disconnect", "Connection disconnected");
            
        #endif
    }

    explicit operator bool() const {
        return *is_connected;
    }

private:
    Signal& signal;
    size_t index;
    // Share the same connection state across all copies
    std::shared_ptr<bool> is_connected;

    std::function<void(size_t)> sig_disconnect;
};

} // namespace easi

#endif // EVENTS_HPP