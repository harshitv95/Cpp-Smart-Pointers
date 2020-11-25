//
// Created by harshitv on 11/20/2020.
//

#ifndef _SMARTPOINTER_
#define _SMARTPOINTER_

#include <iostream>
#include <typeinfo>
#include <mutex>

namespace cs540 {

    extern bool debug;

    class SharedPtrMutex {
        std::mutex sp_mutex;
    public:
        SharedPtrMutex() {
            sp_mutex.lock();
        }

        ~SharedPtrMutex() {
            sp_mutex.unlock();
        }
    };

    class Counter {
    private:
        int count;

    public:
        Counter() : count(0) {}

        int inc() {
            return ++count;
        }

        int dec() {
            return --count;
        }

        int get() {
            return count;
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
            return true;
        }
    };

    template<typename U>
    class PtrValue : public PtrBase {
    private:
        U *value;
    protected:


    public:
        PtrValue(U *value) : PtrBase(), value(value) {
            if (debug)
                std::cout << "Creating instance of PtrValue having addr: " << this << std::endl;
        }

        ~PtrValue() {
//            if (debug)
//                std::cout << "Attempting to delete ptr having addr: " << value << std::endl;
//            delete value;
        }

        void dealloc() {
            delete value;
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
//        T *value = nullptr;

    protected:
//        SharedPtr(PtrBase *ptr, bool inc = true) : SharedPtr(new PtrBase(ptr), inc) {}

        SharedPtr(PtrBase *ptr, Counter *counter, bool inc = true) : ptr_wrap(ptr ? ptr : nullptr), counter(counter) {
            if (inc)
                inc_refcount();
//            if (debug)
//                std::cout << "SharedPtr(PB*, bool) Initializing value at addr: " << ptr_wrap << ", ref_count: "
//                          << ptr_wrap->ref_count << std::endl;
        }

        void dealloc() {
            if (ptr_wrap) {
                dec_refcount();
                delete ptr_wrap;
            }
            ptr_wrap = nullptr;
        }

        void dec_refcount() {
            if (ptr_wrap && (counter->dec() <= 0)) {
                ptr_wrap->dealloc();
                delete counter;
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
//            this->value = ptr->get<T>();
            if (inc)
                inc_refcount();
//            if (debug)
//                std::cout << "set(PB*) Setting new value at addr: " << ptr_wrap << ", ref_count: "
//                          << ptr_wrap->ref_count
//                          << std::endl;
        }

        template<typename U>
        void set(const PtrBase *ptr, Counter *counter, bool inc = true) {
            dealloc();
            this->ptr_wrap = (PtrBase *) ptr;
            this->counter = counter;
//            this->value = ptr->get<T>();
            if (inc)
                inc_refcount();
//            if (debug)
//                std::cout << "set<U>(PB*) Setting new value at addr: " << ptr_wrap << ", ref_count: "
//                          << ptr_wrap->ref_count
//                          << std::endl;
        }

    public:
        // |- Construction:
        SharedPtr() : ptr_wrap(nullptr), counter(nullptr) {
            if (debug)
                std::cout << "SharedPtr() Initializing null at addr: " << ptr_wrap << std::endl;
        }

        SharedPtr(T *t) : SharedPtr(new PtrValue<T>(t), new Counter, true) {}

        template<typename U>
        explicit SharedPtr(U *u): SharedPtr(new PtrValue<U>(u), new Counter, true) {}

        // |-- Copy Construction
        SharedPtr(const SharedPtr &p) : SharedPtr(p.ptr_wrap->clone(), p.counter, true) {}

        template<typename U>
        SharedPtr(const SharedPtr<U> &p) : SharedPtr(p.ptr_wrap->clone(), p.counter, true) {}

        SharedPtr(SharedPtr &&p) : SharedPtr(p.ptr_wrap->clone(), p.counter, false) {}

        template<typename U>
        SharedPtr(SharedPtr<U> &&p): SharedPtr(p.ptr_wrap->clone(), p.counter, false) {}

        // |- Destruction
        ~SharedPtr() {
            dealloc();
        }

        // |- Assignment Operators
        // |-- Copy Assignment
        SharedPtr &operator=(const SharedPtr &p) {
            if (this != &p)
                this->set(p.ptr_wrap->clone(), p.counter);
            return *this;
        }

        template<typename U>
        SharedPtr<T> &operator=(const SharedPtr<U> &p) {
//            if (this != &p)
            this->set<U>(p.ptr_wrap->clone(), p.counter);
            return *this;
        }

        // |-- Move Assignment
        SharedPtr &operator=(SharedPtr &&p) {
            this->set(p.ptr_wrap, p.counter);
            return *this;
        }

        template<typename U>
        SharedPtr &operator=(SharedPtr<U> &&p) {
            this->set<U>(p.ptr_wrap, p.counter);
            return *this;
        }

        // |- Modifiers
        void reset() {
            set(nullptr, nullptr);
        }

        template<typename U>
        void reset(U *p) {
            if (!p)
                this->reset();
            else
                this->set<U>(new PtrValue<U>(p), new Counter);
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
            return ptr_wrap && *ptr_wrap;
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
//        return sp1.ptr_wrap == sp2.ptr_wrap;
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
        return SharedPtr<T>(sp);
//        return SharedPtr<T>(new PtrValue<T>(static_cast<T *>(sp.get())), sp.counter);
//        return SharedPtr<T>(static_cast<T *>(sp.ptr->get()));
    }

    template<typename T, typename U>
    SharedPtr<T> dynamic_pointer_cast(const SharedPtr<U> &sp) {
        return SharedPtr<T>(new PtrValue<T>(dynamic_cast<T *>(sp.get())), sp.counter);
//        SharedPtr<T> ret;
//        PtrWrapper *ptr_wrap = new PtrWrapper()
////        ret.value = dynamic_cast<T *>(sp.value);
//        ret.ptr_wrap = sp.ptr_wrap;
//        return ret;
    }

}

#endif //_SMARTPOINTER_