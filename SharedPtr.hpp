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

    template<typename T>
    class SharedPtr;

    class PtrBase {
        template<typename U>
        friend
        class SharedPtr;

    private:
        int ref_count;
    protected:
        PtrBase() : ref_count(0) {}

//            template<typename U>
//            PtrBase(typename SharedPtr<U>::PtrBase *value) : value(static_cast<T *>(value)), ref_count(0) {}

//            template<typename U>
//            PtrBase(U *value) : value(static_cast<T *>(value)), ref_count(0) {}

    public:
        virtual ~PtrBase() {}

        virtual explicit operator bool() {
            return true;
        }

        virtual bool operator==(const PtrBase &ptr) {
            return true;
        }
    };

    template<typename U>
    class PtrValue : public PtrBase {
        template<typename T>
        friend
        class SharedPtr;

    private:
        U *value;
    protected:
        PtrValue(U *value) : PtrBase(), value(value) {
            if (debug)
                std::cout << "Creating instance of PtrValue having addr: " << this << std::endl;
        }

    public:
        ~PtrValue() {
            if (debug)
                std::cout << "Attempting to delete ptr having addr: " << value << std::endl;
            delete value;
        }

        virtual explicit operator bool() {
            return value;
        }

        virtual bool operator==(const PtrValue<U> &ptr) {
            return (ptr.value) && (value == ptr.value);
        }

        template<typename V>
        bool operator==(const PtrValue<V> &ptr) {
            return !!ptr && (value == ptr.value);
        }
    };

    template<typename T>
    class SharedPtr {
        template<typename U>
        friend
        class SharedPtr;

    private:
        PtrBase *ptr = nullptr;

    protected:
        SharedPtr(PtrBase *ptr, bool inc = true) : ptr(ptr ? ptr : nullptr) {
            if (inc)
                inc_refcount();
            if (debug)
                std::cout << "SharedPtr(PB*, bool) Initializing value at addr: " << ptr << ", ref_count: "
                          << ptr->ref_count << std::endl;
        }

//        template<typename U>
//        SharedPtr(typename SharedPtr<U>::PtrBase *ptr, bool inc = true) : ptr(ptr ? ptr : nullptr) {
//            if (inc)
//                inc_refcount();
//            if (debug)
//                std::cout << "SharedPtr<U>(PB*, bool) Initializing value at addr: " ptr << ", ref_count: " << ptr->ref_count << std::endl;
//        }

        void dec_refcount() {
            if (ptr && (--ptr->ref_count <= 0)) {
                delete ptr;
                ptr = nullptr;
            }
        }

        void inc_refcount() {
            if (ptr)
                ++ptr->ref_count;
        }

        void set(PtrBase *ptr) {
            dec_refcount();
            this->ptr = ptr;
            inc_refcount();
            if (debug)
                std::cout << "set(PB*) Setting new value at addr: " << ptr << ", ref_count: " << ptr->ref_count
                          << std::endl;
        }

        template<typename U>
        void set(const PtrBase *ptr) {
            dec_refcount();
            this->ptr = (PtrBase *) ptr;
            inc_refcount();
            if (debug)
                std::cout << "set<U>(PB*) Setting new value at addr: " << ptr << ", ref_count: " << ptr->ref_count
                          << std::endl;
        }

    public:

        // |- Construction:
        SharedPtr() : ptr(nullptr) {
            if (debug)
                std::cout << "SharedPtr() Initializing null at addr: " << ptr << std::endl;
        }

        SharedPtr(T *t) : SharedPtr(new PtrValue<T>(t), true) {}

        template<typename U>
        explicit SharedPtr(U *u): SharedPtr(new PtrValue<U>(u), true) {}

        // |-- Copy Construction
        SharedPtr(const SharedPtr &p) : SharedPtr(p.ptr, true) {}

        template<typename U>
        SharedPtr(const SharedPtr<U> &p) :
                SharedPtr(p.ptr, true) {}

        SharedPtr(SharedPtr &&p) : SharedPtr(p.ptr, false) {}

        template<typename U>
        SharedPtr(SharedPtr<U> &&p): SharedPtr(p.ptr, false) {}

        // |- Destruction
        ~SharedPtr() {
            dec_refcount();
        }

        // |- Assignment Operators
        // |-- Copy Assignment
        SharedPtr &operator=(const SharedPtr &p) {
            if (this != &p)
                this->set(p.ptr);
            return *this;
        }

        template<typename U>
        SharedPtr<T> &operator=(const SharedPtr<U> &p) {
//            if (this != &p)
            this->set<U>(p.ptr);
            return *this;
        }

        // |-- Move Assignment
        SharedPtr &operator=(SharedPtr &&p) {
            this->set(p.ptr);
            return *this;
        }

        template<typename U>
        SharedPtr &operator=(SharedPtr<U> &&p) {
            this->set<U>(p.ptr);
            return *this;
        }

        // |- Modifiers
        void reset() {
            set(nullptr);
        }

        template<typename U>
        void reset(U *p) {
            this->set<U>(!p ? nullptr : new PtrValue<U>(p));
        }

        // |- Observers
        T *get() const {
            return ((PtrValue<T> *) ptr)->value;
        }

        T &operator*() const {
            return *this->get();
        }

        T *operator->() const {
            return this->get();
        }

        explicit operator bool() const {
            return ptr && *ptr;
        }

        template<typename T1>
        friend bool operator!(const SharedPtr<T1> &);

        // |- Non-member (free standing) functions
        template<typename T1, typename T2>
        friend bool operator==(const SharedPtr<T1> &, const SharedPtr<T2> &);

//        template<typename T1, typename T2>
//        friend SharedPtr<T1> static_pointer_cast(const SharedPtr<T2> &);

//        template<typename T1, typename T2>
//        friend SharedPtr<T1> dynamic_pointer_cast(const SharedPtr<T2> &);

    };

    template<typename T1>
    bool operator!(const SharedPtr<T1> &sp) {
        return !sp.ptr || !(sp.get());
    }

    template<typename T1, typename T2>
    bool operator==(const SharedPtr<T1> &sp1, const SharedPtr<T2> &sp2) {
        if (!sp1 && !sp2) return true;
        return sp1.ptr == sp2.ptr;
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
//        return SharedPtr<T>(static_cast<T *>(sp.ptr->get()));
    }

    template<typename T, typename U>
    SharedPtr<T> dynamic_pointer_cast(const SharedPtr<U> &sp) {
        return SharedPtr<T>(dynamic_cast<T *>(sp.get()));
    }

}

#endif //_SMARTPOINTER_
