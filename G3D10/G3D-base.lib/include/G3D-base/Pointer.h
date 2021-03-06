/**
  \file G3D-base.lib/include/G3D-base/Pointer.h

  G3D Innovation Engine http://casual-effects.com/g3d
  Copyright 2000-2019, Morgan McGuire
  All rights reserved
  Available under the BSD License
*/
#ifndef G3D_Pointer_h
#define G3D_Pointer_h

#include "G3D-base/debugAssert.h"
#include "G3D-base/ReferenceCount.h"
#include <functional>

namespace G3D {

/**
   Acts like a pointer to a value of type ValueType (i.e.,
   ValueType*), but can operate through accessor methods as well as on
   a value in memory.  This is useful for implementing scripting
   languages and other applications that need to connect existing APIs
   by reference.

   Because the accessors require values to be passed by value (instead of by reference)
   this is primarily useful for objects whose memory size is small.

   \code
   class Foo {
   public:
      void setEnabled(bool b);
      bool getEnabled() const;
   };

   Foo  f;
   bool b;
   
   Pointer<bool> p1(&b);
   Pointer<bool> p2(&f, &Foo::getEnabled, &Foo::setEnabled);

   *p1 = true;
   *p2 = false;
   *p2 = *p1; \/\/ Value assignment
   p2 = p1; \/\/ Pointer aliasing

   \/\/ Or, equivalently:
   p1.setValue(true);
   p2.setValue(false);

   p2.setValue(p1.getValue());
   p2 = p1;
   \endcode

   <i>Note:</i> Because of the way that dereference is implemented, you cannot pass <code>*p</code> through a function
   that takes varargs (...), e.g., <code>printf("%d", *p)</code> will produce a compile-time error.  Instead use
   <code>printf("%d",(bool)*p)</code> or <code>printf("%d", p.getValue())</code>.

   \cite McGuire, GUIs for Real-time Programs, using Universal Pointers, SIGGRAPH 2008 Poster.
 */
template<typename ValueType>
class Pointer {
private:

    class Interface {
    public:
        virtual ~Interface() {};
        virtual void set(ValueType b) = 0;
        virtual ValueType get() const = 0;
        virtual Interface* clone() const = 0;
        virtual bool isNull() const = 0;
    };

    class Memory : public Interface {
    private:

        ValueType* value;

    public:
        
        Memory(ValueType* value) : value(value) {}

        virtual void set(ValueType v) override {
            *value = v;
        }

        virtual ValueType get() const override {
            return *value;
        }

        virtual Interface* clone() const override {
            return new Memory(value);
        }

        virtual bool isNull() const override {
            return value == nullptr;
        }
    };

    template<typename GetMethod, typename SetMethod>
    class FcnAccessor : public Interface {
    private:

        GetMethod   getMethod;
        SetMethod   setMethod;

    public:
        
        FcnAccessor(GetMethod getMethod, SetMethod setMethod) : getMethod(getMethod), setMethod(setMethod) {
        }

        virtual void set(ValueType v) override {
            if (setMethod) {
                (*setMethod)(v);
            }
        }

        virtual ValueType get() const override {
            return (*getMethod)();
        }

        virtual Interface* clone() const override {
            return new FcnAccessor(getMethod, setMethod);
        }

        virtual bool isNull() const override { return false; }
    };
    
    class StdFcnGetter : public Interface {
    private:

        std::function<ValueType(void)>   getMethod;

    public:

        StdFcnGetter(std::function<ValueType(void)> getMethod) : getMethod(getMethod) {
        }

        virtual void set(ValueType v) override {
            (void)v;
        }

        virtual ValueType get() const override {
            return getMethod();
        }

        virtual Interface* clone() const override {
            return new StdFcnGetter(getMethod);
        }

        virtual bool isNull() const override { return false; }
    };

    class StdFcnAccessor : public Interface {
    private:

        std::function<ValueType(void)>   getMethod;
        std::function<void(ValueType)>   setMethod;

    public:

        StdFcnAccessor(std::function<ValueType(void)> getMethod, std::function<void(ValueType)> setMethod) : getMethod(getMethod), setMethod(setMethod) {
        }

        virtual void set(ValueType v) override {
            setMethod(v);
        }

        virtual ValueType get() const override {
            return getMethod();
        }

        virtual Interface* clone() const override {
            return new StdFcnAccessor(getMethod, setMethod);
        }

        virtual bool isNull() const override { return false; }
    };

    template<class T, typename GetMethod, typename SetMethod>
    class Accessor : public Interface {
    private:

        T*          object;
        GetMethod   getMethod;
        SetMethod   setMethod;

    public:
        
        Accessor(T* object, 
                 GetMethod getMethod, 
                 SetMethod setMethod) : object(object), getMethod(getMethod), setMethod(setMethod) {
      debugAssert(notNull(object));
        }

        virtual void set(ValueType v) override {
            if (setMethod) {
                (object->*setMethod)(v);
            }
        }

        virtual ValueType get() const override {
            return (object->*getMethod)();
        }

        virtual Interface* clone() const override {
            return new Accessor(object, getMethod, setMethod);
        }

        virtual bool isNull() const override {
            return object == nullptr;
        }
    };


    template<class T, typename GetMethod, typename SetMethod>
    class SharedAccessor : public Interface {
    private:

        shared_ptr<T>   object;
        GetMethod       getMethod;
        SetMethod       setMethod;

    public:
        
        SharedAccessor
           (const shared_ptr<T>& object, 
            GetMethod getMethod, 
            SetMethod setMethod) : object(object), getMethod(getMethod), setMethod(setMethod) {
            debugAssert(notNull(object));
        }

        virtual void set(ValueType v) override {
            if (setMethod) {
                (object.get()->*setMethod)(v);
            }
        }

        virtual ValueType get() const override {
            return (object.get()->*getMethod)();
        }

        virtual Interface* clone() const override {
            return new SharedAccessor(object, getMethod, setMethod);
        }

        virtual bool isNull() const override {
            return (bool)object;
        }
    };

    Interface* m_interface;

public:

    Pointer() : m_interface(nullptr) {};

    /** Allows implicit cast from real pointer */
    Pointer(ValueType* v) : m_interface(new Memory(v)) {}

    inline bool isNull() const {
        return (m_interface == nullptr) || m_interface->isNull();
    }

    // Assignment
    inline Pointer& operator=(const Pointer& r) {
        delete m_interface;
        if (r.m_interface != nullptr) {
            m_interface = r.m_interface->clone();
        } else {
            m_interface = nullptr;
        }
        return this[0];
    }

    Pointer(const Pointer& p) : m_interface(nullptr) {
        this[0] = p;
    }


    /** \param setMethod May be nullptr */
    template<class Class>
    Pointer(const shared_ptr<Class>& object,
            ValueType (Class::*getMethod)() const,
            void (Class::*setMethod)(ValueType)) : 
        m_interface(new SharedAccessor<Class, ValueType (Class::*)() const, void (Class::*)(ValueType)>(object, getMethod, setMethod)) {}

    /** \param setMethod May be nullptr */
    template<class Class>
    Pointer(const shared_ptr<Class>& object,
            const ValueType& (Class::*getMethod)() const,
            void (Class::*setMethod)(ValueType)) : 
        m_interface(new SharedAccessor<Class, const ValueType& (Class::*)() const, void (Class::*)(ValueType)>(object, getMethod, setMethod)) {}


    /** \param setMethod May be nullptr */
    Pointer(ValueType (*getMethod)(),
            void (*setMethod)(ValueType)) : 
        m_interface(new FcnAccessor<ValueType (*)(), void (*)(ValueType)>(getMethod, setMethod)) {}

    Pointer(std::function<ValueType(void)> getMethod) :
        m_interface(new StdFcnGetter(getMethod)) {}

    Pointer(std::function<ValueType(void)> getMethod,
        std::function<void(ValueType)> setMethod) :
        m_interface(new StdFcnAccessor(getMethod, setMethod)) {}

    /** \param setMethod May be nullptr */
    Pointer(const ValueType& (*getMethod)(),
            void (*setMethod)(ValueType)) : 
        m_interface(new FcnAccessor<const ValueType& (*)(), void (*)(ValueType)>(getMethod, setMethod)) {}

    /** \param setMethod May be nullptr */
    template<class Class>
    Pointer(const shared_ptr<Class>& object,
            ValueType (Class::*getMethod)() const,
            void (Class::*setMethod)(const ValueType&)) : 
        m_interface(new SharedAccessor<Class, ValueType (Class::*)() const, void (Class::*)(const ValueType&)>(object, getMethod, setMethod)) {}

    /** \param setMethod May be nullptr */
    template<class Class>
    Pointer(const shared_ptr<Class>& object,
            const ValueType& (Class::*getMethod)() const,
            void (Class::*setMethod)(const ValueType&)) : 
        m_interface(new SharedAccessor<Class, const ValueType& (Class::*)() const, void (Class::*)(const ValueType&)>(object, getMethod, setMethod)) {}

    /** \param setMethod May be nullptr */
    template<class Class>
    Pointer(Class* object,
            const ValueType& (Class::*getMethod)() const,
            void (Class::*setMethod)(const ValueType&)) : 
        m_interface(new Accessor<Class, const ValueType& (Class::*)() const, void (Class::*)(const ValueType&)>(object, getMethod, setMethod)) {}

    /** \param setMethod May be nullptr */
    template<class Class>
    Pointer(Class* object,
            ValueType (Class::*getMethod)() const,
            void (Class::*setMethod)(const ValueType&)) : 
        m_interface(new Accessor<Class, ValueType (Class::*)() const, void (Class::*)(const ValueType&)>(object, getMethod, setMethod)) {}

    /** \param setMethod May be nullptr */
    template<class Class>
    Pointer(Class* object,
            const ValueType& (Class::*getMethod)() const,
            void (Class::*setMethod)(ValueType)) : 
        m_interface(new Accessor<Class, const ValueType& (Class::*)() const, void (Class::*)(ValueType)>(object, getMethod, setMethod)) {}

    /** \param setMethod May be nullptr */
    template<class Class>
    Pointer(Class* object,
            ValueType (Class::*getMethod)() const,
            void (Class::*setMethod)(ValueType)) : 
        m_interface(new Accessor<Class, ValueType (Class::*)() const, void (Class::*)(ValueType)>(object, getMethod, setMethod)) {}

    ~Pointer() {
        delete m_interface;
    }

    inline const ValueType getValue() const {
        debugAssert(m_interface != nullptr);
        return m_interface->get();
    }

    /** \brief Assign a value to the referenced element.
        If this Pointer was initialized with a nullptr setMethod, the call is ignored */
    inline void setValue(const ValueType& v) const {
        debugAssert(m_interface != nullptr);
        m_interface->set(v);
    }

    class IndirectValue {
    private:

        friend class Pointer;
        const Pointer* pointer;
        IndirectValue(const Pointer* p) : pointer(p) {}

    public:

        void operator=(const ValueType& v) {
            pointer->setValue(v);
        }

        operator ValueType() const {
            return pointer->getValue();
        }

    };

    inline IndirectValue operator*() {
        return IndirectValue(this);
    }

    inline IndirectValue operator*() const {
        return IndirectValue(this);
    }

/*    inline const ValueType operator*() const {
        return getValue();
    }
    */
};

template<class T>
bool isNull(const Pointer<T>& p) {
    return p.isNull();
}

template<class T>
bool notNull(const Pointer<T>& p) {
    return ! p.isNull();
}


/** Wraps a boolean Pointer with one that inverts its value. */
class NotAdapter : public ReferenceCountedObject {
private:
    friend class Pointer<bool>;

    Pointer<bool>      m_source;

    typedef shared_ptr<NotAdapter> Ptr;

    NotAdapter(const Pointer<bool>& ptr) : m_source(ptr) {
    }

    /** For use by Pointer<T> */
    bool get() const {
        return ! m_source.getValue();
    }

    /** For use by Pointer<T> */
    void set(const bool& v) {
        m_source.setValue(! v);
    }

public:

    static Pointer<bool> wrap(const Pointer<bool>& ptr) {
        Ptr p(new NotAdapter(ptr));
        return Pointer<bool>(p, &NotAdapter::get, &NotAdapter::set);
    }
};


/** Maps an integer pointer to a power of two G3D::Pointer value \sa NegativeAdapter */
template<class T>
class PowerOfTwoAdapter : public ReferenceCountedObject {
private:

    friend class Pointer<int>;

    Pointer<int>      m_source;

    typedef shared_ptr<PowerOfTwoAdapter> Ptr;

    /** For use by Pointer<T> */
    T get() const { return m_source.getValue(); }

    /** For use by Pointer<T> */
    void set(const T& v) { m_source.setValue(ceilPow2(v)); }

    PowerOfTwoAdapter(Pointer<T> ptr) : m_source(ptr) {}

public:

    static Pointer<T> create(Pointer<T> ptr) {
        Ptr p(new PowerOfTwoAdapter(ptr));
        return Pointer<T>(p, &PowerOfTwoAdapter::get, &PowerOfTwoAdapter::set);
    }
};

/** Negates a numeric type G3D::Pointer value \sa PercentageAdapter */
template<class T>
class NegativeAdapter : public ReferenceCountedObject {
private:

    friend class Pointer<T>;

    Pointer<T>      m_source;

    typedef shared_ptr<NegativeAdapter> Ptr;

    NegativeAdapter(Pointer<T> ptr) : m_source(ptr) {}

    /** For use by Pointer<T> */
    T get() const { return -m_source.getValue(); }

    /** For use by Pointer<T> */
    void set(const T& v) { m_source.setValue(-v); }

public:

    static Pointer<T> create(Pointer<T> ptr) {
        Ptr p(new NegativeAdapter(ptr));
        return Pointer<T>(p, &NegativeAdapter<T>::get, &NegativeAdapter<T>::set);
    }
};


/** Maps a floating point fraction to a percentage G3D::Pointer value \sa NegativeAdapter */
template<class T>
class PercentageAdapter : public ReferenceCountedObject {
private:

    friend class Pointer<float>;

    Pointer<float>      m_source;

    typedef shared_ptr<PercentageAdapter> Ptr;

    /** For use by Pointer<T> */
    T get() const { return 100.0f * m_source.getValue(); }

    /** For use by Pointer<T> */
    void set(const T& v) { m_source.setValue(v * 0.01f); }

    PercentageAdapter(Pointer<T> ptr) : m_source(ptr) {}

public:

    static Pointer<T> create(Pointer<T> ptr) {
        Ptr p(new PercentageAdapter(ptr));
        return Pointer<T>(p, &PercentageAdapter::get, &PercentageAdapter::set);
    }
};


/** Maps an integer pointer to a perfect square */
template<class T>
class SquareAdapter : public ReferenceCountedObject {
private:

    friend class Pointer<int>;

    Pointer<int>      m_source;

    typedef shared_ptr<SquareAdapter> Ptr;

    /** For use by Pointer<T> */
    T get() const { return m_source.getValue(); }

    /** For use by Pointer<T> */
    void set(const T& v) {
        int low = int(square(floor(sqrt(v))));
        int high = int(square(ceil(sqrt(v))));
        if (high - v < v - low) {
            m_source.setValue(high);
        }
        else {
            m_source.setValue(low);
        }
    }

    SquareAdapter(Pointer<T> ptr) : m_source(ptr) {}

public:

    static Pointer<T> create(Pointer<T> ptr) {
        Ptr p(new SquareAdapter(ptr));
        return Pointer<T>(p, &SquareAdapter::get, &SquareAdapter::set);
    }
};

} // G3D

#endif
