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

namespace signal {

// Forward declarations
template<typename Signal>
class Connection;

template<typename Owner, typename... Args>
class Signal;

struct shared_own {};

template<typename T>
struct unique_own {};

namespace detail {

#if SIGNAL_DEBUG
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

    Terminal terminal;
#endif

/**
 * @brief Event object.
 *
 * @details Represents a single callback managed by a Signal.
 * Each Event stores the callback function, its connection handle,
 * and a dirty flag used to skip execution during an emit cycle.
 *
 * @tparam Owner The class type that owns and emits callbacks.
 * @tparam Args  Parameter pack of argument types forwarded to
 *               each callback when invoked.
 */

template<typename Owner, typename... Args>
struct Event {
    bool dirty;
    std::function<void(Args...)> callback;
    Connection<Signal<Owner, Args...>> conn;
};

template<typename Owner, typename... Args>
struct EventBase {

    bool dirty;
    std::function<void(Args...)> callback;
    Connection<Signal<Owner, Args...>> conn;

};

template <typename... Args>
struct Event<shared_own, Args...>
    : EventBase<shared_own, Args...>
{
    using Base = EventBase<shared_own, Args...>;
    using Base::dirty;
    using Base::callback;
    using Base::conn;
};

template<typename Owner, typename... Args>
struct Slot {
    Event<Owner, Args...> event;
};

} // namespace detail



/**
 * @brief Callback manager object.
 *
 * @details Stores and manages callbacks that can be
 * connected and invoked by the owning class.
 *
 * @tparam Owner The class type that can emit callbacks.
 * @tparam Args  Parameter pack of argument types forwarded to
 *               each callback when invoked.
 */
template<typename Owner, typename... Args>
class SignalBase {
    friend Connection;
public:
    explicit SignalBase() = default;

    /**
     * @brief Connects a callback to the signal.
     *
     * @note If called during emission, the callback is deferred until the next cycle.
     * @warning The returned Connection becomes invalid if the Signal is destroyed.
     *
     * @param callback Function to invoke when the signal is emitted.
     * @return A Connection object that can later disconnect the callback.
     */
    Connection<Signal<Owner, Args...>> connect(std::function<void(Args...)> callback) {
        bool is_dirty;
        Connection<Signal<Owner, Args...>> connection;

        if (is_emitting) {
            is_dirty = true;
        } else {
            is_dirty = false;
        }


        if (!available_slots.empty()) {
            size_t index = available_slots.back();
            available_slots.pop_back();

            std::function<void(size_t)> sig_disconnect = [this](size_t index) {
                disconnect(index);
            };
            using SignalType = Signal<Owner, Args...>;
            Connection<SignalType> conn(index, sig_disconnect);

            detail::Event<Owner, Args...> ev;
            ev.dirty = is_dirty;
            ev.callback = std::move(callback);
            ev.conn = conn;
            connection = conn;

            slot_vector[index].event = ev;
            
        } else {

            size_t index = slot_vector.size();
            
            std::function<void(size_t)> sig_disconnect = [this](size_t index) {
                disconnect(index);
            };
            using SignalType = Signal<Owner, Args...>;
            Connection<SignalType> conn(index, sig_disconnect);

            detail::Event<Owner, Args...> ev;
            ev.dirty = is_dirty;
            ev.callback = std::move(callback);
            ev.conn = conn;
            connection = conn;

            detail::Slot<Owner, Args...> slot;
            slot.event = std::move(ev);

            slot_vector.push_back(std::move(slot));
        }
       
        #if SIGNAL_DEBUG
            detail::terminal.send_debug("easi::Signal::connect", "Created connection");
        #endif

        return connection;
    }

protected:
    /**
     * @brief Emits all connected callbacks with the given arguments.
     *
     * @note Skips dirty events until the next cycle.
     *
     * @param args Arguments forwarded to each callback.
     */
    void emit(Args... args) {
        is_emitting = true ;
        for (auto& s : slot_vector) {
            if (!s.event.callback) {
                continue;
            }

            if (s.event.dirty) {
                dirty_event_vector.push_back(s);
                continue;
            }

            s.event.callback(args...);
        }

        #if SIGNAL_DEBUG
            detail::terminal.send_debug("easi::Signal::emit", "Emitted callbacks");
            std::cout << "[debug] [easi::Signal::emit]: Callback vector size: " << event_vector.size() << std::endl;
        #endif

        is_emitting = false;

        for (auto& s : dirty_event_vector) {
            s.event.dirty = false;
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
        slot_vector[index].event.callback = nullptr;
        available_slots.push_back(index);
    }


    bool is_emitting = false;
    std::vector<detail::Slot<Owner, Args...>> slot_vector;
    std::vector<size_t> available_slots;
    std::vector<detail::Slot<Owner, Args...>> dirty_event_vector;
};

template<typename... Args>
class Signal<shared_own, Args...>
    : public SignalBase<shared_own, Args...> 
{
public:
    using Base = SignalBase<shared_own, Args...>;

    using Base::connect;
    using Base::emit;

private:
    using Base::disconnect;

};

template<typename Owner, typename... Args>
class Signal<unique_own<Owner>, Args...>
    : public SignalBase<unique_own<Owner>, Args...> 
{
    friend Owner;

public:
    using Base = SignalBase<unique_own<Owner>, Args...>;

    using Base::connect;

private:
    using Base::emit;
    using Base::disconnect;

};

// Each signal type is defined as a template, so we must know
// the specific type at compile time to handle it correctly.
/**
 * @brief Connection object to reference and manage 
 *        a callback inside a signal.
 *
 * @tparam Signal The signal type that owns the callback
 */
template<typename Signal>
class Connection {
public:
    Connection() : index(static_cast<size_t>(-1)), is_connected(std::make_shared<bool>(false)), sig_disconnect(nullptr) {}
    
    explicit Connection(size_t index, std::function<void(size_t)> disconnect)
        : index(index), sig_disconnect(disconnect), is_connected(std::make_shared<bool>(true)) {}
    
    /**
     * @brief Disconnects a callback from a signal.
     * 
     * @note Safe to call multiple times. Calls after the 
     *       callback has been disconnected will be ignored.
     */
    void disconnect() {
        if (!*is_connected) {
            return;
        }

        sig_disconnect(index);
        *is_connected = false;

        #if SIGNAL_DEBUG
            detail::terminal.send_debug("easi::Connection::disconnect", "Connection disconnected");
            
        #endif
    }

    explicit operator bool() const {
        return *is_connected;
    }

private:
    size_t index;
    // Share the same connection state across all copies
    std::shared_ptr<bool> is_connected;

    std::function<void(size_t)> sig_disconnect;
};

} // namespace signal

} // namespace easi

#endif // SIGNAL_HPP