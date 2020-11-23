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
            const T *value;
        public:
            PtrType(T *value) : value(value), ref_count(0) {}

            ~PtrType() {
                delete value;
            }

            bool operator==(const typename PtrType &ptr) {
                return !!ptr && (value == ptr.value);
            }

            template<typename U>
            bool operator==(const typename SharedPtr<U>::PtrType &ptr) {
                return !!ptr && (value == ptr.value);
            }
        };

        PtrType *ptr;
    public:

        // |- Construction:
        SharedPtr() : ptr(nullptr) {}

        SharedPtr(T *t) : SharedPtr(new PtrType(t)) {}

        template<typename U>
        explicit SharedPtr(U *u): SharedPtr(new PtrType(u)) {}

        // |-- Copy Construction
        SharedPtr(const SharedPtr &p) : SharedPtr(p.ptr) {}

        template<typename U>
        SharedPtr(const SharedPtr<U> &p) : SharedPtr(p.ptr) {}

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
        }

        template<typename U>
        SharedPtr<T> &operator=(const SharedPtr<U> &p) {
            if (this != &p)
                this->set<U>(p.ptr);
        }

        // |-- Move Assignment
        SharedPtr &operator=(SharedPtr &&p) {
            this->set(p.ptr);
        }

        template<typename U>
        SharedPtr &operator=(SharedPtr<U> &&p) {
            this->set<U>(p.ptr);
        }

        // |- Modifiers
        void reset() {
            set(nullptr);
        }

        template<typename U>
        void reset(U *p) {
            this->set<U>(!p ? nullptr : new PtrType(p));
        }

        // |- Observers
        T *get() const {
            return ptr->value;
        }

        T &operator*() const {
            return *ptr->value;
        }

        T *operator->() const {
            return ptr->value;
        }

        explicit operator bool() const {
            return ptr && ptr->value;
        }

        // |- Non-member (free standing) functions
        template<typename T1, typename T2>
        friend bool operator==(const SharedPtr<T1> &, const SharedPtr<T2> &);

        template<typename T1, typename T2>
        friend SharedPtr<T1> static_pointer_cast(const SharedPtr<T2> &);

        template<typename T1, typename T2>
        friend SharedPtr<T1> dynamic_pointer_cast(const SharedPtr<T2> &);

    protected:
        SharedPtr(PtrType *ptr, bool inc = true) : ptr(ptr ? ptr : nullptr) {
            if (inc)
                inc_refcount();
        }

        template<typename U>
        SharedPtr(typename SharedPtr<U>::PtrType *ptr, bool inc = true) : ptr(ptr ? ptr : nullptr) {
            if (inc)
                inc_refcount();
        }

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

        void set(const PtrType *ptr) {
            dec_refcount();
            this->ptr = ptr;
            inc_refcount();
        }

        template<typename U>
        void set(const typename SharedPtr<U>::PtrType *ptr) {
            dec_refcount();
            this->ptr = ptr;
            inc_refcount();
        }

    };

    template<typename T1, typename T2>
    bool operator==(const SharedPtr<T1> &sp1, const SharedPtr<T2> &sp2) {
        if (!sp1 && !sp2) return true;
        return sp1.ptr == sp2.ptr || *sp1.ptr == *sp2.ptr;
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
        return SharedPtr<T>(static_cast<T *>(sp.ptr));
    }

    template<typename T, typename U>
    SharedPtr<T> dynamic_pointer_cast(const SharedPtr<U> &sp) {
        return SharedPtr<T>(dynamic_cast<T *>(sp.ptr));
    }

}

#endif //_SMARTPOINTER_
