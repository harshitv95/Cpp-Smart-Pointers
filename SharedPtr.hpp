
//
// Created by harshitv on 11/20/2020.
//

#ifndef _SMARTPOINTER_
#define _SMARTPOINTER_

#include <cstddef>
#include <typeinfo>

namespace cs540 {

    struct dynamic_cast_helper {
        template<typename TGT, typename SRC>
        static TGT *do_cast(SRC *p) { return dynamic_cast<TGT *>(p); }
    };

    template<typename T, typename U>
    T *optional_dynamic_cast(U *val) {
        if (!val) return nullptr;
        T *ret = static_cast<T *>(val);
        typeid(ret = dynamic_cast_helper::do_cast<U>(ret), *val);
        return ret;
    }

    class Counter {
    private:
        int count;

    public:
        Counter() : count(0) {}

        int inc() {
            return __sync_add_and_fetch(&count, 1, __ATOMIC_SEQ_CST);
        }

        int dec() {
            return __sync_sub_and_fetch(&count, 1, __ATOMIC_SEQ_CST);
        }

        int get() {
            return count;
        }

        ~Counter() {
        }

    };

    template<typename T>
    class SharedPtr;

    class PtrBase {
    public:
        virtual ~PtrBase() {}

        virtual void dealloc() {}

        virtual PtrBase *clone() {
            return new PtrBase();
        }

        virtual explicit operator bool() const {
            return true;
        }

        virtual bool operator==(const PtrBase &ptr) const {
            return this == &ptr;
        }
    };

    template<typename U>
    class PtrValue : public PtrBase {
    private:
        U *value;
    public:
        PtrValue(U *value) : PtrBase(), value(value) {
        }

        ~PtrValue() {
        }

        void dealloc() {
            delete value;
            value = nullptr;
        }

        PtrBase *clone() {
            return new PtrValue<U>(value);
        }

        U *get() {
            return value;
        }

        virtual explicit operator bool() const {
            return value;
        }

        virtual bool operator==(const PtrValue<U> &ptr) {
            return (ptr.value) && (value == ptr.value);
        }

        template<typename V>
        bool operator==(const PtrValue<V> &ptr) const {
            return !!ptr && (value == ptr.value);
        }
    };

    template<typename T>
    class SharedPtr {
        template<typename U>
        friend
        class SharedPtr;

    private:
        PtrBase *ptr_wrap;
        Counter *counter;
        T *val;

    protected:

        SharedPtr(PtrBase *ptr, Counter *counter, T *val, bool inc = true) :
                ptr_wrap(ptr ? ptr : nullptr), counter(counter), val(val) {
            if (inc)
                inc_refcount();
        }

        void dealloc() {
            if (counter) {
                dec_refcount();
                delete ptr_wrap;
                counter = nullptr;
                ptr_wrap = nullptr;
            }
        }

        void dec_refcount() {
            if (counter) {
                if (counter->dec() == 0) {
                    if (ptr_wrap)
                        ptr_wrap->dealloc();
                    delete counter;
                }
            }
        }

        void inc_refcount() {
            if (ptr_wrap)
                counter->inc();
        }

        void set(PtrBase *ptr, Counter *counter, bool inc = true) {
            dealloc();
            this->ptr_wrap = ptr;
            this->counter = counter;
            if (inc)
                inc_refcount();
        }

        template<typename U>
        void set(const PtrBase *ptr, Counter *counter, bool inc = true) {
            dealloc();
            this->ptr_wrap = (PtrBase *) ptr;
            this->counter = counter;
            if (inc)
                inc_refcount();
        }

    public:
        // |- Construction:
        SharedPtr() : ptr_wrap(nullptr), counter(nullptr) {
        }

        SharedPtr(T *t) : SharedPtr(new PtrValue<T>(t), new Counter, t) {}

        template<typename U>
        explicit SharedPtr(U *u): SharedPtr(new PtrValue<U>(u), new Counter, static_cast<T *>(u)) {}

        // |-- Copy Construction
        SharedPtr(const SharedPtr &p) : SharedPtr(!p ? nullptr : p.ptr_wrap->clone(), p.counter, p.get()) {}

        template<typename U>
        SharedPtr(const SharedPtr<U> &p) :
                SharedPtr(!p ? nullptr : p.ptr_wrap->clone(), p.counter, static_cast<T *>(p.get())) {}

        // |- Move Construction
        SharedPtr(SharedPtr &&p) : SharedPtr(!p ? nullptr : p.ptr_wrap->clone(), p.counter, p.get(), false) {
            p.ptr_wrap = nullptr;
            p.counter = nullptr;
        }

        template<typename U>
        SharedPtr(SharedPtr<U> &&p):
                SharedPtr(!p ? nullptr : p.ptr_wrap->clone(), p.counter, static_cast<T *>(p.get()), false) {
            p.ptr_wrap = nullptr;
            p.counter = nullptr;
        }

        // |- Destruction
        ~SharedPtr() {
            dealloc();
        }

        // |- Assignment Operators
        // |-- Copy Assignment
        SharedPtr &operator=(const SharedPtr &p) {
            if (this != &p)
                this->set(!p ? nullptr : p.ptr_wrap->clone(), p.counter);
            return *this;
        }

        template<typename U>
        SharedPtr<T> &operator=(const SharedPtr<U> &p) {
//            this->val = optional_dynamic_cast<T *, U *>(static_cast<T *>(p.get()), std::is_polymorphic<T>::value);
            this->val = optional_dynamic_cast<T>(p.get());
            this->set<U>(!p ? nullptr : p.ptr_wrap->clone(), p.counter);
            return *this;
        }

        // |-- Move Assignment
        SharedPtr &operator=(SharedPtr &&p) {
            this->set(!p ? nullptr : p.ptr_wrap, p.counter, false);
            p.ptr_wrap = nullptr;
            p.counter = nullptr;
            return *this;
        }

        template<typename U>
        SharedPtr &operator=(SharedPtr<U> &&p) {
            this->set<U>(!p ? nullptr : p.ptr_wrap, p.counter, false);
            p.ptr_wrap = nullptr;
            p.counter = nullptr;
            return *this;
        }

        // |- Modifiers
        void reset() {
            set(nullptr, nullptr, false);
        }

        template<typename U>
        void reset(U *p) {
            if (!p)
                this->reset();
            else {
                this->set<U>(new PtrValue<U>(p), new Counter);
            }
        }

        // |- Observers
        T *get() const {
            return ((PtrValue<T> *) ptr_wrap)->get();
        }

        T &operator*() const {
            return *this->get();
        }

        T *operator->() const {
            return this->get();
        }

        explicit operator bool() const {
            return ptr_wrap && !!(get());
        }

        template<typename T1>
        friend bool operator!(const SharedPtr<T1> &);

        // |- Non-member (free standing) functions
        template<typename T1, typename T2>
        friend bool operator==(const SharedPtr<T1> &, const SharedPtr<T2> &);

        template<typename T1, typename T2>
        friend SharedPtr<T1> static_pointer_cast(const SharedPtr<T2> &);

        template<typename T1, typename T2>
        friend SharedPtr<T1> dynamic_pointer_cast(const SharedPtr<T2> &);

    };

    template<typename T1>
    bool operator!(const SharedPtr<T1> &sp) {
        return !sp.ptr_wrap || !(sp.get());
    }

    template<typename T1, typename T2>
    bool operator==(const SharedPtr<T1> &sp1, const SharedPtr<T2> &sp2) {
        if (!sp1 && !sp2) return true;
        return sp1.counter == sp2.counter;
    }

    template<typename T>
    bool operator==(const SharedPtr<T> &sp, std::nullptr_t) {
        return !sp;
    }

    template<typename T>
    bool operator==(std::nullptr_t, const SharedPtr<T> &sp) {
        return !sp;
    }

    template<typename T1, typename T2>
    bool operator!=(const SharedPtr<T1> &sp1, const SharedPtr<T2> &sp2) {
        return !(sp1 == sp2);
    }

    template<typename T>
    bool operator!=(const SharedPtr<T> &sp, std::nullptr_t) {
        return sp;
    }

    template<typename T>
    bool operator!=(std::nullptr_t, const SharedPtr<T> &sp) {
        return sp;
    }

    template<typename T, typename U>
    SharedPtr<T> static_pointer_cast(const SharedPtr<U> &sp) {
//        return SharedPtr<T>(sp);
//        static_cast<T *>(sp.get());
        return SharedPtr<T>(sp);
    }

    template<typename T, typename U>
    SharedPtr<T> dynamic_pointer_cast(const SharedPtr<U> &sp) {
        T *val = dynamic_cast<T *>(sp.get());
        return SharedPtr<T>(new PtrValue<T>(val), sp.counter, val);
    }

}

#endif //_SMARTPOINTER_