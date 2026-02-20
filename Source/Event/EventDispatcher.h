#pragma once

#include "../Precompiled.h"
#include <functional>

template<typename... Args>
class CCallbackBase {
public:
    virtual ~CCallbackBase() {};
    virtual void Execute(Args... args) = 0;
};

template<class T, typename... Args>
class CCallback : public CCallbackBase<Args...> {
public:
    typedef void (T::*fn)(Args...);

public:
    CCallback(T& target, fn opr) : m_target(target), m_operation(opr) {}

public:
    void Execute(Args... args) override
    {
        (m_target.*m_operation)(args...);
    }

private:
    T& m_target;
    fn m_operation;
};

template<typename EventType, typename... Args>
class EventDispatcher {
public:
    EventDispatcher() {}
    ~EventDispatcher()
    {
        Kill();
    }

public:
    template<class T>
    void Register(const EventType& eventType, T& target, void(T::*fn)(Args...)) 
    {
        m_handlers[eventType] = new CCallback<T, Args...>(target, fn);
    }

    void Dispatch(const EventType& eventType, Args... args) 
    {
        auto it = m_handlers.find(eventType);
        if(it != m_handlers.end() && it->second) {
            it->second->Execute(args...);
        }
    }

    bool Has(const EventType& eventType) 
    {
        return m_handlers.find(eventType) != m_handlers.end();
    }

    void Kill() 
    {
        for(auto& [_, pCb] : m_handlers) {
            SAFE_DELETE(pCb);
        }

        m_handlers.clear();
    }

private:
    std::unordered_map<EventType, CCallbackBase<Args...>*> m_handlers;
};