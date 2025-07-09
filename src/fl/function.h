#pragma once
#include "fl/ptr.h"
#include "fl/template_magic.h"
#include "fl/compiler_control.h"
#include "fl/variant.h"
#include "fl/type_traits.h"

FL_DISABLE_WARNING_PUSH
FL_DISABLE_WARNING(float-equal)

namespace fl {

//----------------------------------------------------------------------------
// More or less a drop in replacement for std::function
// function<R(Args...)>: type‐erasing "std::function" replacement
// Supports free functions, lambdas/functors, member functions (const &
// non‑const)
// 
// NEW: Uses fl::Variant for inline storage to avoid heap allocation
// for most common cases. Falls back to heap allocation for large callables.
//----------------------------------------------------------------------------
template <typename> class function;

template <typename R, typename... Args> class function<R(Args...)> {
  private:
    // Storage size for inline callables (64 bytes should cover most cases)
    static constexpr fl::size INLINE_STORAGE_SIZE = 64;
    
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

    // Wraps a non‑const member function
    template <typename C> struct MemCallable : CallableBase {
        C *obj;
        R (C::*mfp)(Args...);
        MemCallable(C *o, R (C::*m)(Args...)) : obj(o), mfp(m) {}
        R invoke(Args... args) override { return (obj->*mfp)(args...); }
    };

    // Wraps a const member function
    template <typename C> struct ConstMemCallable : CallableBase {
        const C *obj;
        R (C::*mfp)(Args...) const;
        ConstMemCallable(const C *o, R (C::*m)(Args...) const)
            : obj(o), mfp(m) {}
        R invoke(Args... args) override { return (obj->*mfp)(args...); }
    };

    // Function pointer type for free functions
    using FunctionPtr = R(*)(Args...);

    // Inline storage variant - can hold small callables without heap allocation
    using InlineCallable = fl::Variant<
        FunctionPtr                    // Free function pointer
    >;

    // Union to store either inline callable or heap pointer
    union Storage {
        InlineCallable inline_callable;
        Ptr<CallableBase> heap_callable;
        
        Storage() : inline_callable() {}
        ~Storage() {} // Manual cleanup required
    };

    Storage storage_;
    bool using_heap_;

    // Helper to check if a callable can fit inline
    template <typename F>
    static constexpr bool can_fit_inline() {
        return fl::is_same<F, FunctionPtr>::value;
    }

    // Helper to create inline callable
    template <typename F>
    void create_inline_callable(F&& f) {
        using_heap_ = false;
        if constexpr (fl::is_same<F, FunctionPtr>::value) {
            // Free function pointer
            storage_.inline_callable = f;
        } else {
            // For other types, we need to use heap allocation
            create_heap_callable(fl::forward<F>(f));
        }
    }

    // Helper to create heap callable
    template <typename F>
    void create_heap_callable(F&& f) {
        using_heap_ = true;
        new (&storage_.heap_callable) Ptr<CallableBase>(NewPtr<Callable<F>>(fl::forward<F>(f)));
    }

    // Helper to invoke inline callable
    R invoke_inline(Args... args) const {
        struct InvokeVisitor {
            Args... args;
            R result;
            
            InvokeVisitor(Args... a) : args(a...), result() {}
            
            void accept(FunctionPtr f) { result = f(args...); }
        };
        
        InvokeVisitor visitor(args...);
        storage_.inline_callable.visit(visitor);
        return visitor.result;
    }

    // Helper to cleanup storage
    void cleanup() {
        if (using_heap_) {
            storage_.heap_callable.~Ptr<CallableBase>();
        } else {
            storage_.inline_callable.~InlineCallable();
        }
    }

  public:
    function() : using_heap_(false) {}
    ~function() { cleanup(); }

    function(const function &o) : using_heap_(o.using_heap_) {
        if (using_heap_) {
            new (&storage_.heap_callable) Ptr<CallableBase>(o.storage_.heap_callable);
        } else {
            new (&storage_.inline_callable) InlineCallable(o.storage_.inline_callable);
        }
    }

    function(function &&o) noexcept : using_heap_(o.using_heap_) {
        if (using_heap_) {
            new (&storage_.heap_callable) Ptr<CallableBase>(fl::move(o.storage_.heap_callable));
        } else {
            new (&storage_.inline_callable) InlineCallable(fl::move(o.storage_.inline_callable));
        }
        o.using_heap_ = false;
        new (&o.storage_.inline_callable) InlineCallable();
    }

    function &operator=(const function &o) {
        if (this != &o) {
            cleanup();
            using_heap_ = o.using_heap_;
            if (using_heap_) {
                new (&storage_.heap_callable) Ptr<CallableBase>(o.storage_.heap_callable);
            } else {
                new (&storage_.inline_callable) InlineCallable(o.storage_.inline_callable);
            }
        }
        return *this;
    }

    function& operator=(function&& o) noexcept {
        if (this != &o) {
            cleanup();
            using_heap_ = o.using_heap_;
            if (using_heap_) {
                new (&storage_.heap_callable) Ptr<CallableBase>(fl::move(o.storage_.heap_callable));
            } else {
                new (&storage_.inline_callable) InlineCallable(fl::move(o.storage_.inline_callable));
            }
            o.using_heap_ = false;
            new (&o.storage_.inline_callable) InlineCallable();
        }
        return *this;
    }

    // 1) generic constructor for lambdas, free functions, functors
    template <typename F,
              typename = enable_if_t<!is_member_function_pointer<F>::value>>
    function(F f) : using_heap_(false) {
        if constexpr (fl::is_same<F, FunctionPtr>::value) {
            // Free function pointer can be stored inline
            create_inline_callable(fl::forward<F>(f));
        } else {
            // For lambdas and functors, use heap allocation for now
            // TODO: Implement proper inline storage for small lambdas
            create_heap_callable(fl::forward<F>(f));
        }
    }

    // 2) non‑const member function
    template <typename C>
    function(R (C::*mf)(Args...), C *obj) : using_heap_(false) {
        // Member functions always use heap allocation for now
        create_heap_callable(MemCallable<C>(obj, mf));
    }

    // 3) const member function
    template <typename C>
    function(R (C::*mf)(Args...) const, const C *obj) : using_heap_(false) {
        // Const member functions always use heap allocation for now
        create_heap_callable(ConstMemCallable<C>(obj, mf));
    }

    // Invocation
    R operator()(Args... args) const { 
        if (using_heap_) {
            return storage_.heap_callable->invoke(args...);
        } else {
            return invoke_inline(args...);
        }
    }

    explicit operator bool() const { 
        if (using_heap_) {
            return storage_.heap_callable != nullptr;
        } else {
            return !storage_.inline_callable.empty();
        }
    }

    bool operator==(const function &o) const {
        if (using_heap_ != o.using_heap_) return false;
        if (using_heap_) {
            return storage_.heap_callable == o.storage_.heap_callable;
        } else {
            return storage_.inline_callable == o.storage_.inline_callable;
        }
    }

    bool operator!=(const function &o) const {
        return !(*this == o);
    }
};

} // namespace fl

FL_DISABLE_WARNING_POP
