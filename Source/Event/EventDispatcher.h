#pragma once

#include "../Precompiled.h"
#include <functional>

/*template<typename... Args>
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
};*/

template<typename... Args>
class Delegate {
public:
    typedef void(*Opr)(void*, Args...);

public:
    Delegate() : m_target(nullptr), m_operation(nullptr) {}

public:
    template<class T, void(T::*fn)(Args...)>
    static Delegate Create(T* target)
    {
        Delegate delegate;
        delegate.m_target = target;
        delegate.m_operation = &FuncOpr<T, fn>;

        return delegate;
    }

    template<void(*fn)(Args...)>
    static Delegate Create()
    {
        Delegate delegate;
        delegate.m_target = nullptr;
        delegate.m_operation = &FuncOpr<fn>;
        return delegate;
    }

    void Execute(Args... args)
    {
        if(m_operation) {
            (*m_operation)(m_target, args...);
        }
    }

    bool IsInitialized() const
    {
        return m_operation != nullptr;
    }

private:
    template<class T, void(T::*fn)(Args...)>
    static void FuncOpr(void* target, Args... args)
    {
        T* tg = (T*)target;
        (tg->*fn)(args...);
    }

    template<void(*fn)(Args...)>
    static void FuncOpr(void*, Args... args)
    {
        fn(args...);
    }

private:
    void* m_target;
    Opr m_operation;
};

template<typename EventType, typename... Args>
class EventDispatcher {
public:
    typedef Delegate<Args...> Handler;

public:
    EventDispatcher() {}

public:
    void Register(EventType type, Handler handler)
    {
        m_handlers[type] = handler;
    }

    void Dispatch(EventType type, Args... args)
    {
        auto it = m_handlers.find(type);
        if(it != m_handlers.end() && it->second.IsInitialized()) {
            it->second.Execute(args...);
        }
    }

private:
    std::unordered_map<EventType, Handler> m_handlers;
};