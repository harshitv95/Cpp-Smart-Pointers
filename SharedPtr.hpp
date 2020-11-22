//
// Created by harshitv on 11/20/2020.
//

#ifndef _SMARTPOINTER_
#define _SMARTPOINTER_

#include <mutex>

namespace cs540 {

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
    class SharedPtr {
    private:
        class PtrType {
            friend SharedPtr<T>;
            int ref_count;
            T *val;
        public:
            PtrType(T *val) : val(val), ref_count(0) {}

            ~PtrType() {
                delete val;
            }

            T *getValue() {
                return val;
            }
        };

        PtrType *ptr;
    public:

        // |- Construction:
        SharedPtr() : ptr(new PtrType(nullptr)) {}

        template<typename U>
        explicit SharedPtr(U *u): ptr(u) {}

        // |-- Copy Construction
        SharedPtr(const SharedPtr &p) : SharedPtr(!p ? nullptr : p.ptr) {}

        template<typename U>
        SharedPtr(const SharedPtr<U> &p) : SharedPtr(!p ? nullptr : p.ptr) {}

        SharedPtr(SharedPtr &&p) : SharedPtr(!p ? nullptr : p.ptr, false) {}

        template<typename U>
        SharedPtr(SharedPtr<U> &&p): SharedPtr(!p ? nullptr : p.ptr, false) {}

        // |- Destruction
        ~SharedPtr() {
            dec_refcount();
        }

        // |-Operators

        // |-- Assignment
        // |--- Copy Assignment
        SharedPtr &operator=(const SharedPtr &p) {
            if (this != &p)
                this->set(p.ptr);
        }

        template<typename U>
        SharedPtr<T> &operator=(const SharedPtr<U> &p) {
            if (this != &p)
                this->set<U>(p.ptr);
        }

        // |--- Move Assignment
        SharedPtr &operator=(SharedPtr &&p) {
            this->set(p.ptr);
        }

        template<typename U>
        SharedPtr &operator=(SharedPtr<U> &&p) {
            this->set<U>(p.ptr);
        }

        // |- Modifiers
        void reset() {
            set(new PtrType(nullptr));
        }

        template<typename U>
        void reset(U *p) {
            this->set<U>(new PtrType(p));
        }

    protected:
        SharedPtr(PtrType *ptr, bool inc = true) : ptr(ptr ? ptr : new PtrType(nullptr)) {
            if (inc && ptr)
                inc_refcount();
        }

        void dec_refcount() {
            if ((--ptr->ref_count) <= 0) {
                delete ptr;
                ptr = nullptr;
            }
        }

        void inc_refcount() {
            if (ptr)
                ++ptr->ref_count;
        }

        void set(const PtrType *ptr) {
            dec_refcount();
            this->ptr = ptr;
            inc_refcount();
        }

        template<typename U>
        void set(const typename SharedPtr<U>::PtrType *p) {
            dec_refcount();
            this->ptr = p.ptr;
            inc_refcount();
        }

    };

}

#endif //_SMARTPOINTER_
