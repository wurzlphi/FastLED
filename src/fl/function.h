#pragma once
#include "fl/ptr.h"
#include "fl/template_magic.h"
#include "fl/compiler_control.h"
#include "fl/warn.h"

FL_DISABLE_WARNING_PUSH
FL_DISABLE_WARNING(float-equal)

namespace fl {

//----------------------------------------------------------------------------
// More or less a drop in replacement for std::function
// function<R(Args...)>: type‐erasing "std::function" replacement
// Supports free functions, lambdas/functors, member functions (const &
// non‑const)
// 
// NEW: Uses inline storage for member function callables
// to avoid heap allocation for member function calls.
//----------------------------------------------------------------------------
template <typename> class function;

template <typename R, typename... Args> class function<R(Args...)> {
  private:
    // Base class for all callable types
    struct CallableBase : public Referent {
        virtual R invoke(Args... args) = 0;
        virtual ~CallableBase() = default;
    };

    // Wraps a lambda/functor or free function pointer
    template <typename F> struct Callable : CallableBase {
        F f;
        Callable(F fn) : f(fn) {}
        R invoke(Args... args) override { return f(args...); }
    };

    // Type-erased member function callable using inline storage
    // This stores member function pointers inline without heap allocation
    struct MemberCallable {
        // Object pointer (void* to avoid template bloat)
        void* obj;
        // Member function pointer stored in a union to allow void* casting
        union {
            void* void_ptr;
            // Non-const member function pointer
            R (*nonconst_mfp)(void*, Args...);
            // Const member function pointer  
            R (*const_mfp)(const void*, Args...);
        } func_ptr;
        // Whether this is a const member function
        bool is_const;
        
        MemberCallable() : obj(nullptr), func_ptr{nullptr}, is_const(false) {}
        
        // Constructor for non-const member function
        template <typename C>
        MemberCallable(C* object, R (C::*member_func)(Args...)) 
            : obj(object), is_const(false) {
            // Create a function pointer that can call the member function
            func_ptr.nonconst_mfp = [=](void* obj, Args... args) -> R {
                return (static_cast<C*>(obj)->*member_func)(args...);
            };
        }
        
        // Constructor for const member function
        template <typename C>
        MemberCallable(const C* object, R (C::*member_func)(Args...) const)
            : obj(const_cast<void*>(static_cast<const void*>(object))), is_const(true) {
            // Create a function pointer that can call the const member function
            func_ptr.const_mfp = [=](const void* obj, Args... args) -> R {
                return (static_cast<const C*>(obj)->*member_func)(args...);
            };
        }
        
        // Invoke the member function
        R invoke(Args... args) const {
            if (is_const) {
                return func_ptr.const_mfp(obj, args...);
            } else {
                return func_ptr.nonconst_mfp(obj, args...);
            }
        }
    };

    // Union to store either a heap-allocated callable or an inline member callable
    union Storage {
        Ptr<CallableBase> heap_callable;
        MemberCallable member_callable;
        
        Storage() : heap_callable() {}
        ~Storage() {} // Manual cleanup required
    };
    
    Storage storage_;
    bool is_member_callable_;

  public:
    function() : is_member_callable_(false) {}
    
    ~function() {
        if (is_member_callable_) {
            storage_.member_callable.~MemberCallable();
        }
        // heap_callable destructor is called automatically
    }

    function(const function &o) : is_member_callable_(o.is_member_callable_) {
        if (is_member_callable_) {
            new (&storage_.member_callable) MemberCallable(o.storage_.member_callable);
        } else {
            new (&storage_.heap_callable) Ptr<CallableBase>(o.storage_.heap_callable);
        }
    }

    function(function &&o) noexcept : is_member_callable_(o.is_member_callable_) {
        if (is_member_callable_) {
            new (&storage_.member_callable) MemberCallable(fl::move(o.storage_.member_callable));
            o.storage_.member_callable.~MemberCallable();
        } else {
            new (&storage_.heap_callable) Ptr<CallableBase>(fl::move(o.storage_.heap_callable));
        }
        o.is_member_callable_ = false;
    }

    function &operator=(const function &o) {
        if (this != &o) {
            // Clean up current storage
            if (is_member_callable_) {
                storage_.member_callable.~MemberCallable();
            }
            
            is_member_callable_ = o.is_member_callable_;
            
            if (is_member_callable_) {
                new (&storage_.member_callable) MemberCallable(o.storage_.member_callable);
            } else {
                new (&storage_.heap_callable) Ptr<CallableBase>(o.storage_.heap_callable);
            }
        }
        return *this;
    }

    function& operator=(function&& o) noexcept {
        if (this != &o) {
            // Clean up current storage
            if (is_member_callable_) {
                storage_.member_callable.~MemberCallable();
            }
            
            is_member_callable_ = o.is_member_callable_;
            
            if (is_member_callable_) {
                new (&storage_.member_callable) MemberCallable(fl::move(o.storage_.member_callable));
                o.storage_.member_callable.~MemberCallable();
            } else {
                new (&storage_.heap_callable) Ptr<CallableBase>(fl::move(o.storage_.heap_callable));
            }
            o.is_member_callable_ = false;
        }
        return *this;
    }

    // 1) generic constructor for lambdas, free functions, functors
    template <typename F,
              typename = enable_if_t<!is_member_function_pointer<F>::value>>
    function(F f) : is_member_callable_(false) {
        new (&storage_.heap_callable) Ptr<CallableBase>(NewPtr<Callable<F>>(f));
    }

    // 2) non‑const member function - now uses inline storage
    template <typename C>
    function(R (C::*mf)(Args...), C *obj) : is_member_callable_(true) {
        new (&storage_.member_callable) MemberCallable(obj, mf);
    }

    // 3) const member function - now uses inline storage
    template <typename C>
    function(R (C::*mf)(Args...) const, const C *obj) : is_member_callable_(true) {
        new (&storage_.member_callable) MemberCallable(obj, mf);
    }

    // Invocation
    R operator()(Args... args) const { 
        if (is_member_callable_) {
            return storage_.member_callable.invoke(args...);
        } else {
            return storage_.heap_callable->invoke(args...);
        }
    }

    explicit operator bool() const { 
        if (is_member_callable_) {
            return storage_.member_callable.obj != nullptr;
        } else {
            return storage_.heap_callable != nullptr;
        }
    }

    bool operator==(const function &o) const {
        if (is_member_callable_ != o.is_member_callable_) {
            return false;
        }
        if (is_member_callable_) {
            return storage_.member_callable.obj == o.storage_.member_callable.obj &&
                   storage_.member_callable.func_ptr.void_ptr == o.storage_.member_callable.func_ptr.void_ptr &&
                   storage_.member_callable.is_const == o.storage_.member_callable.is_const;
        } else {
            return storage_.heap_callable == o.storage_.heap_callable;
        }
    }

    bool operator!=(const function &o) const {
        return !(*this == o);
    }
};

} // namespace fl

FL_DISABLE_WARNING_POP
