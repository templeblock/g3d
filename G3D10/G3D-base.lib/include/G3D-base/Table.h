/**
  \file G3D-base.lib/include/G3D-base/Table.h

  G3D Innovation Engine http://casual-effects.com/g3d
  Copyright 2000-2019, Morgan McGuire
  All rights reserved
  Available under the BSD License
*/

#ifndef G3D_Table_h
#define G3D_Table_h

#include <cstddef>
#include "G3D-base/G3DString.h"

#include "G3D-base/platform.h"
#include "G3D-base/Array.h"
#include "G3D-base/debug.h"
#include "G3D-base/System.h"
#include "G3D-base/g3dmath.h"
#include "G3D-base/EqualsTrait.h"
#include "G3D-base/HashTrait.h"
#include "G3D-base/MemoryManager.h"

#ifdef _MSC_VER
#   pragma warning (push)
    // Debug name too long warning
#   pragma warning (disable : 4786)
#endif

namespace G3D {

/**
 An unordered data structure mapping keys to values.

 There are two ways of definining custom hash functions (G3D provides built-in ones for most classes):

 <pre>
 class Foo {
 public:
     String     name;
     int             index;
     static size_t hashCode(const Foo& key) {
          return HashTrait<String>::hashCode(key.name) + key.index;
     }
  };

  template<> struct HashTrait<class Foo> {
       static size_t hashCode(const Foo& key) { return HashTrait<String>::hashCode(key.name) + key.index; }
  }; 


  // Use Foo::hashCode
  Table<Foo, String, Foo> fooTable1;

  // Use HashTrait<Foo>
  Table<Foo, String>      fooTable2;
  </pre>


 Key must be a pointer, an int, a String or provide overloads for: 

  <PRE>
    template<> struct HashTrait<class Key> {
        static size_t hashCode(const Key& key) { return reinterpret_cast<size_t>( ... ); }
    }; 
  </PRE>

  and one of 

  <PRE>
    template<> struct EqualsTrait<class Key>{
         static bool equals(const Key& a, const Key& b) { return ... ; }
    };


    bool operator==(const Key&, const Key&);
  </PRE>

 G3D pre-defines HashTrait specializations for common types (like <CODE>int</CODE> and <CODE>String</CODE>).
 If you use a Table with a different type you must write those functions yourself.  For example,
 an enum would use:

  <PRE>
    template<> struct HashTrait<MyEnum> {
        static size_t hashCode(const MyEnum& key) const { return reinterpret_cast<size_t>( key ); }
    };
  </PRE>

  And rely on the default enum operator==.


  Periodically check that debugGetLoad() is low (> 0.1).  When it gets near
  1.0 your hash function is badly designed and maps too many inputs to
  the same output.
 */
template<class Key, class Value, class HashFunc = HashTrait<Key>, class EqualsFunc = EqualsTrait<Key> > 
class Table {
public:

    /**
     The pairs returned by iterator.
     */
    class Entry {
    public:
        Key    key;
        Value  value;
        Entry() {}
        Entry(const Key& k) : key(k) {}
        Entry(const Key& k, const Value& v) : key(k), value(v) {}
        bool operator==(const Entry &peer) const { return (key == peer.key && value == peer.value); }
        bool operator!=(const Entry &peer) const { return !operator==(peer); }
    };

private:

    typedef Table<Key, Value, HashFunc, EqualsFunc> ThisType;

    /**
     Linked list nodes used internally by HashTable.
     */
    class Node {
    public:
        Entry       entry;
        size_t      hashCode;
        Node*       next;

    private:

        // Private to require use of the allocator
        Node(const Key& k, const Value& v, size_t h, Node* n) 
            : entry(k, v), hashCode(h), next(n) {
                debugAssert((next == nullptr) || isValidHeapPointer(next));
        }

        Node(const Key& k, size_t h, Node* n) 
            : entry(k), hashCode(h), next(n) {
                debugAssert((next == nullptr) || isValidHeapPointer(next));
        }

    public:

        static Node* create(const Key& k, const Value& v, size_t h, Node* n, shared_ptr<MemoryManager>& mm) {
            Node* node = (Node*)mm->alloc(sizeof(Node));
            return new (node) Node(k, v, h, n);
        }

        static Node* create(const Key& k, size_t hashCode, Node* n, shared_ptr<MemoryManager>& mm) {
            Node* node = (Node*)mm->alloc(sizeof(Node));
            return new (node) Node(k, hashCode, n);
        }

        static void destroy(Node* n, shared_ptr<MemoryManager>& mm) {
            n->~Node();
            mm->free(n);
        }

        /**
        Clones a whole chain;
        */
        Node* clone(shared_ptr<MemoryManager>& mm) {
           return create(this->entry.key, this->entry.value, hashCode, (next == nullptr) ? nullptr : next->clone(mm), mm);
        }
    };

    void checkIntegrity() const {
#       ifdef G3D_DEBUG
           debugAssert(m_bucket == nullptr || isValidHeapPointer(m_bucket));
           for (size_t b = 0; b < m_numBuckets; ++b) {
               Node* node = m_bucket[b];
               debugAssert(node == nullptr || isValidHeapPointer(node));
               while (node != nullptr) {
                   debugAssert(node == nullptr || isValidHeapPointer(node));
                   node = node->next;
               }
           }
#       endif
    }

    /** Number of elements in the table.*/
    size_t                      m_size;

    /**
     Array of Node*. 

     We don't use Array<Node*> because Table is lower-level than Array.
     Some elements may be nullptr.
     */
    Node**                      m_bucket;
    
    /**
     Length of the m_bucket array.
     */
    size_t                      m_numBuckets;

    shared_ptr<MemoryManager>   m_memoryManager;

    void* alloc(size_t s) const {
        return m_memoryManager->alloc(s);
    }

    void free(void* p) const {
        return m_memoryManager->free(p);
    }

    /**
     Re-hashes for a larger m_bucket size.
     */
    void resize(size_t newSize) {
        alwaysAssertM((m_bucket != nullptr) || (m_numBuckets == 0), "Internal pointer is null even though buckets have been allocated.");
        // Hang onto the old m_bucket array
        Node** oldBucket = m_bucket;

        // Allocate a new m_bucket array with the new size
        m_bucket = (Node**)alloc(sizeof(Node*) * newSize);
        alwaysAssertM(m_bucket != nullptr, "MemoryManager::alloc returned nullptr. Out of memory.");
        // Set all pointers to nullptr
        System::memset(m_bucket, 0, newSize * sizeof(Node*));
        // Move each node to its new hash location
        for (size_t b = 0; b < m_numBuckets; ++b) {
            Node* node = oldBucket[b];
         
            // There is a linked list of nodes at this m_bucket
            while (node != nullptr) {
                // Hang onto the old next pointer
                Node* nextNode = node->next;
        
                // Insert at the head of the list for m_bucket[i]
                size_t i = node->hashCode % newSize;
                node->next = m_bucket[i];
                m_bucket[i] = node;

                // Move on to the next node
                node = nextNode;
            }

            // Drop the old pointer for cleanliness when debugging
            oldBucket[b] = nullptr;
        }

        // Delete the old storage
        free(oldBucket);
        m_numBuckets = newSize;
        alwaysAssertM(m_bucket != nullptr, "Buckets were null after allocation");

        checkIntegrity();
    }


    void copyFrom(const ThisType& h) {
        if (&h == this) {
            return;
        }

        debugAssert(m_bucket == nullptr);
        m_size = h.m_size;
        m_numBuckets = h.m_numBuckets;
        m_bucket = (Node**)alloc(sizeof(Node*) * m_numBuckets);
        // No need to nullptr elements since we're about to overwrite them

        for (size_t b = 0; b < m_numBuckets; ++b) {
            if (h.m_bucket[b] != nullptr) {
                m_bucket[b] = h.m_bucket[b]->clone(m_memoryManager);
            } else {
                m_bucket[b] = nullptr;
            }
        }

        checkIntegrity();
    }

    /**
     Frees the heap structures for the nodes.
     */
    void freeMemory() {
        checkIntegrity();

        for (size_t b = 0; b < m_numBuckets; ++b) {
            Node* node = m_bucket[b];
            while (node != nullptr) {
                Node* next = node->next;
                Node::destroy(node, m_memoryManager);
                node = next;
            }
            m_bucket[b] = nullptr;
        }
        free(m_bucket);
        m_bucket     = nullptr;
        m_numBuckets = 0;
        m_size       = 0;
    }

public:

    /**
     Creates an empty hash table using the default MemoryManager.
     */
    Table() : m_bucket(nullptr) {
        m_memoryManager = MemoryManager::create();
        m_numBuckets = 0;
        m_size       = 0;
        m_bucket     = nullptr;
        checkIntegrity();
    }

    /** Changes the internal memory manager to m */
    void clearAndSetMemoryManager(const shared_ptr<MemoryManager>& m) {
        clear();
        debugAssert(m_bucket == nullptr);
        m_memoryManager = m;
    }

    /** 
        Recommends that the table resize to anticipate at least this number of elements.
     */
    void setSizeHint(size_t n) {
        size_t s = n * 3;
        if (s > m_numBuckets) {
            resize(s);
        }
    }
    
    /**
       Destroys all of the memory allocated by the table, but does <B>not</B>
       call delete on keys or values if they are pointers.  If you want to
       deallocate things that the table points at, use getKeys() and Array::deleteAll()
       to delete them.
    */
    virtual ~Table() {
        freeMemory();
    }

    /** Uses the default memory manager */
    Table(const ThisType& h) {
        m_memoryManager = MemoryManager::create();
        m_numBuckets = 0;
        m_size = 0;
        m_bucket = nullptr;
        this->copyFrom(h);
        checkIntegrity();
    }


    Table& operator=(const ThisType& h) {
        // No need to copy if the argument is this
        if (this != &h) {
            // Free the existing nodes
            freeMemory();
            this->copyFrom(h);
            checkIntegrity();
        }
        return *this;
    }

    /**
     Returns the length of the deepest m_bucket.
     */
    size_t debugGetDeepestBucketSize() const {
        size_t deepest = 0;

        for (size_t b = 0; b < m_numBuckets; ++b) {
            size_t  count = 0;
            Node*   node = m_bucket[b];
            while (node != nullptr) {
                node = node->next;
                ++count;
            }

            if (count > deepest) {
                deepest = count;
            }
        }

        return deepest;
    }

    /**
       Returns the average size of non-empty buckets.
    */
    float debugGetAverageBucketSize() const {
        uint64 num = 0;

        for (size_t b = 0; b < m_numBuckets; ++b) {
            Node* node = m_bucket[b];
            if (node != nullptr) {
                ++num;
            }
        }

        return (float)((double)size() / num);
    }

    /**
     A small load (close to zero) means the hash table is acting very
     efficiently most of the time.  A large load (close to 1) means 
     the hash table is acting poorly-- all operations will be very slow.
     A large load will result from a bad hash function that maps too
     many keys to the same code.
     */
    double debugGetLoad() const {
        return (double)size() / m_numBuckets;
    }

    /**
     Returns the number of buckets.
     */
    size_t debugGetNumBuckets() const {
        return m_numBuckets;
    }

    /**
     C++ STL style iterator variable.  See begin().
     */
    class Iterator {
    private:
        friend class Table<Key, Value, HashFunc, EqualsFunc>;

        /**
         Bucket index.
         */
        size_t              index;

        /**
         Linked list node.
         */
        Node*               node;

        size_t              m_numBuckets;
        Node**              m_bucket;
        bool                isDone;

        /**
         Creates the end iterator.
         */
        Iterator() : index(0), node(nullptr), m_bucket(nullptr) {
            isDone = true;
        }

        Iterator(size_t numBuckets, Node** m_bucket) :
            index(0), 
            node(nullptr),
            m_numBuckets(numBuckets),
            m_bucket(m_bucket) {
            
            if (m_numBuckets == 0) {
                // Empty table
                isDone = true;
                return;
            }

#           ifdef G3D_DEBUG
                for (unsigned int i = 0; i < m_numBuckets; ++i) {
                    debugAssert((m_bucket[i] == nullptr) || isValidHeapPointer(m_bucket[i]));
                }
#           endif

            index = 0;
            node = m_bucket[index];
            debugAssert((node == nullptr) || isValidHeapPointer(node));
            isDone = false;
            findNext();
            debugAssert((node == nullptr) || isValidHeapPointer(node));
        }

        /**
         If node is nullptr, then finds the next element by searching through the bucket array.
         Sets isDone if no more nodes are available.
         */
        void findNext() {
            while (node == nullptr) {
                ++index;
                if (index >= m_numBuckets) {
                    m_bucket = nullptr;
                    index = 0;
                    isDone = true;
                    return;
                } else {
                    node = m_bucket[index];
                    debugAssert((node == nullptr) || isValidHeapPointer(node));
                }
            }
            debugAssert(isValidHeapPointer(node));
        }

    public:
        inline bool operator!=(const Iterator& other) const {
            return !(*this == other);
        }

        bool operator==(const Iterator& other) const {
            if (other.isDone || isDone) {
                // Common case; check against isDone.
                return (isDone == other.isDone);
            } else {
                return (node == other.node) && (index == other.index);
            }
        }

        /**
         Pre increment.
         */
        Iterator& operator++() {
            debugAssert(! isDone);
            debugAssert(node != nullptr);
            debugAssert(isValidHeapPointer(node));
            debugAssert((node->next == nullptr) || isValidHeapPointer(node->next));
            node = node->next;
            findNext();
            debugAssert(isDone || isValidHeapPointer(node));
            return *this;
        }

        /**
         Post increment (slower than preincrement).
         */
        Iterator operator++(int) {
            Iterator old = *this;
            ++(*this);
            return old;
        }

        const Entry& operator*() const {
            return node->entry;
        }

        const Value& value() const {
            return node->entry.value;
        }

        const Key& key() const {
            return node->entry.key;
        }

        Entry* operator->() const {
            debugAssert(isValidHeapPointer(node));
            return &(node->entry);
        }

        operator Entry*() const {
            debugAssert(isValidHeapPointer(node));
            return &(node->entry);
        }

        bool isValid() const {
            return ! isDone;
        }

        /** @deprecated  Use isValid */
        bool hasMore() const {
            return ! isDone;
        }
    };


    /**
     C++ STL style iterator method.  Returns the first Entry, which 
     contains a key and value.  Use preincrement (++entry) to get to
     the next element.  Do not modify the table while iterating.
     */
    Iterator begin() const {
        return Iterator(m_numBuckets, m_bucket);
    }

    /**
     C++ STL style iterator method.  Returns one after the last iterator
     element.
     */
    const Iterator end() const {
        return Iterator();
    }

    /**
     Removes all elements. Guaranteed to free all memory associated with
     the table.
     */
    void clear() {
        freeMemory();
        m_numBuckets = 0;
        m_size = 0;
        m_bucket = nullptr;
    }

   
    /**
     Returns the number of keys.
     */
    size_t size() const {
        return m_size;
    }


    /**
     If you insert a pointer into the key or value of a table, you are
     responsible for deallocating the object eventually.  Inserting 
     key into a table is O(1), but may cause a potentially slow rehashing.
     */
    void set(const Key& key, const Value& value) {
        getCreateEntry(key).value = value;
    }

private:

    /** Helper for remove() and getRemove() */
    bool remove(const Key& key, Key& removedKey, Value& removedValue, bool updateRemoved) {
        if (m_numBuckets == 0) {
            return false;
        }
       
        const size_t code = HashFunc::hashCode(key);
        const size_t b = code % m_numBuckets;

        // Go to the m_bucket
        Node* n = m_bucket[b];

        if (n == nullptr) {
            return false;
        }

        Node* previous = nullptr;
      
        // Try to find the node
        do {
          if ((code == n->hashCode) && EqualsFunc::equals(n->entry.key, key)) {
              // This is the node; remove it

              // Replace the previous's next pointer
              if (previous == nullptr) {
                  m_bucket[b] = n->next;
              } else {
                  previous->next = n->next;
              }

              if (updateRemoved) {
                  removedKey   = n->entry.key;
                  removedValue = n->entry.value;
              }
              // Delete the node
              Node::destroy(n, m_memoryManager);
              --m_size;

              //checkIntegrity();
              return true;
          }

          previous = n;
          n = n->next;
      } while (n != nullptr);

      //checkIntegrity();
      return false;
   }

public:

   /** If @a member is present, sets @a removed to the element
    being removed and returns true.  Otherwise returns false
    and does not write to @a removed. */
    bool getRemove(const Key& key, Key& removedKey, Value& removedValue) {
       return remove(key, removedKey, removedValue, true);
    }

    /**
    Removes an element from the table if it is present.  
    @return true if the element was found and removed, otherwise  false
    */
   bool remove(const Key& key) {
       Key x;
       Value v;
       return remove(key, x, v, false);
   }

private:

   Entry* getEntryPointer(const Key& key) const {
       if (m_numBuckets == 0) {
           return nullptr;
       }

       size_t  code = HashFunc::hashCode(key);
       size_t b = code % m_numBuckets;

       Node* node = m_bucket[b];

       while (node != nullptr) {
           if ((node->hashCode == code) && EqualsFunc::equals(node->entry.key, key)) {
               return &(node->entry);
           }
           node = node->next;
       }

       return nullptr;
   }

public:
   
    /** If a value that is EqualsFunc to @a member is present, returns a pointer to the 
        version stored in the data structure, otherwise returns nullptr.
     */
   const Key* getKeyPointer(const Key& key) const {
       const Entry* e = getEntryPointer(key);
       if (e == nullptr) {
           return nullptr;
       } else {
           return &(e->key);
       }
   }

   /**
    Returns the value associated with key.
    @deprecated Use get(key, val) or getPointer(key) 
    */
   Value& get(const Key& key) const {
       Entry* e = getEntryPointer(key);
       debugAssertM(e != nullptr, "Key not found");
       return e->value;
   }


   /** Returns a pointer to the element if it exists, or nullptr if it does not.
       Note that if your value type <i>is</i> a pointer, the return value is 
       a pointer to a pointer.  Do not remove the element while holding this 
       pointer.

       It is easy to accidentally mis-use this method.  Consider making 
       a Table<Value*> and using get(key, val) instead, which makes you manage
       the memory for the values yourself and is less likely to result in 
       pointer errors.
     */
   Value* getPointer(const Key& key) const {
       if (m_numBuckets == 0) {
           return nullptr;
       }

       size_t code = HashFunc::hashCode(key);
       size_t b = code % m_numBuckets;

       Node* node = m_bucket[b];

       while (node != nullptr) {
          if ((node->hashCode == code) && EqualsFunc::equals(node->entry.key, key)) {
             // found key
             return &(node->entry.value);
          }
          node = node->next;
      }

      // Failed to find key
      return nullptr;
   }

   /**
    If the key is present in the table, val is set to the associated value and returns true.
    If the key is not present, returns false.
    */
   bool get(const Key& key, Value& val) const {
       Value* v = getPointer(key);
       if (v != nullptr) {
           val = *v;
           return true;
       } else {
           return false;
       }
   }


    /** Called by getCreate() and set() 
        
        \param created Set to true if the entry was created by this method.
    */  
    Entry& getCreateEntry(const Key& key, bool& created) {
        created = false;

        if (m_numBuckets == 0) {
            resize(10);
        }

        size_t code = HashFunc::hashCode(key);
        size_t b = code % m_numBuckets;
        
        // Go to the m_bucket
        Node* n = m_bucket[b];

        // No m_bucket, so this must be the first
        if (n == nullptr) {
            m_bucket[b] = Node::create(key, code, nullptr, m_memoryManager);
            ++m_size;
            created = true;
            //checkIntegrity();
            return m_bucket[b]->entry;
        }

        size_t bucketLength = 1;

        // Sometimes a bad hash code will cause all elements
        // to collide.  Detect this case and don't rehash when 
        // it occurs; nothing good will come from the rehashing.
        bool allSameCode = true;

        // Try to find the node
        do {
            allSameCode = allSameCode && (code == n->hashCode);

            if ((code == n->hashCode) && EqualsFunc::equals(n->entry.key, key)) {
               // This is the a pre-existing node
               //checkIntegrity();
               return n->entry;
            }

            n = n->next;
            ++bucketLength;
        } while (n != nullptr);

        // Allow the load factor to rise as the table gets huge
        const int bucketsPerElement = 
            (m_size > 50000) ? 3 :
                ((m_size > 10000) ? 5 :
                 ((m_size > 5000) ? 10 : 15));

        const size_t maxBucketLength = 3;
        // (Don't bother changing the size of the table if all entries
        // have the same hashcode--they'll still collide)
        if ((bucketLength > maxBucketLength) && 
            ! allSameCode && 
            (m_numBuckets < m_size * bucketsPerElement)) {

            // This m_bucket was really large; rehash if all elements
            // don't have the same hashcode the number of buckets is
            // reasonable.

            // Back off the scale factor as the number of buckets gets 
            // large
            float f = 3.0f;
            if (m_numBuckets > 1000000) {
                f = 1.5f;
            } else if (m_numBuckets > 100000) {
                f = 2.0f;
            }
            int newSize = max((int)(m_numBuckets * f) + 1, (int)(m_size * f));
            resize(newSize);
        }

        // Not found; insert at the head.
        b = code % m_numBuckets;
        m_bucket[b] = Node::create(key, code, m_bucket[b], m_memoryManager);
        ++m_size;
        created = true;

        //checkIntegrity();
        return m_bucket[b]->entry;
    }

    Entry& getCreateEntry(const Key& key) {
        bool ignore;
        return getCreateEntry(key, ignore);
    }
    

    /** Returns the current value that key maps to, creating it if necessary.*/
    Value& getCreate(const Key& key) {
        return getCreateEntry(key).value;
    }

    /** \param created True if the element was created. */
    Value& getCreate(const Key& key, bool& created) {
        return getCreateEntry(key, created).value;
    }


   /**
    Returns true if any key maps to value using operator==.
    */
    bool containsValue(const Value& value) const {
        for (Iterator it = begin(); it.isValid(); ++it) {
            if (it.value() == value) {
                return true;
            }
        }
        return false;
    }

   /**
    Returns true if key is in the table.
    */
   bool containsKey(const Key& key) const {
       if (m_numBuckets == 0) {
           return false;
       }

       size_t code = HashFunc::hashCode(key);
       size_t b = code % m_numBuckets;

       Node* node = m_bucket[b];

       while (node != nullptr) {
           if ((node->hashCode == code) && EqualsFunc::equals(node->entry.key, key)) {
              return true;
           }
           node = node->next;
       }

       return false;
   }


   /**
    Short syntax for get.
    */
   inline Value& operator[](const Key &key) const {
      return get(key);
   }

   /**
    Returns an array of all of the keys in the table.
    You can iterate over the keys to get the values.
    @deprecated
    */
   Array<Key> getKeys() const {
       Array<Key> keyArray;
       getKeys(keyArray);
       return keyArray;
   }

   void getKeys(Array<Key>& keyArray) const {
       keyArray.resize(0, DONT_SHRINK_UNDERLYING_ARRAY);
       for (size_t i = 0; i < m_numBuckets; ++i) {
           Node* node = m_bucket[i];
           while (node != nullptr) {
               keyArray.append(node->entry.key);
               node = node->next;
           }
       }
   }

   /** Will contain duplicate values if they exist in the table.  This array is parallel to the one returned by getKeys() if the table has not been modified. */
   void getValues(Array<Value>& valueArray) const {
       valueArray.resize(0, DONT_SHRINK_UNDERLYING_ARRAY);
       for (size_t i = 0; i < m_numBuckets; ++i) {
           Node* node = m_bucket[i];
           while (node != nullptr) {
               valueArray.append(node->entry.value);
               node = node->next;
           }
       }
   }

   /**
    Calls delete on all of the keys and then clears the table.
    */
   void deleteKeys() {
       for (size_t i = 0; i < m_numBuckets; ++i) {
           Node* node = m_bucket[i];
           while (node != nullptr) {
               delete node->entry.key;
               node->entry.key = nullptr;
               node = node->next;
           }
       }
       clear();
   }

   /**
    Calls delete on all of the values.  This is unsafe--
    do not call unless you know that each value appears
    at most once.

    Does not clear the table, so you are left with a table
    of nullptr pointers.
    */
   void deleteValues() {
       for (size_t i = 0; i < m_numBuckets; ++i) {
           Node* node = m_bucket[i];
           while (node != nullptr) {
               delete node->entry.value;
               node->entry.value = nullptr;
               node = node->next;
           }
       }
   }

    template<class H, class E>
    bool operator==(const Table<Key, Value, H, E>& other) const {
        if (size() != other.size()) {
            return false;
        }

        for (Iterator it = begin(); it.hasMore(); ++it) {
            const Value* v = other.getPointer(it->key);
            if ((v == nullptr) || (*v != it->value)) {
                // Either the key did not exist or the value was not the same
                return false;
            }
        }

        // this and other have the same number of keys, so we don't
        // have to check for extra keys in other.

        return true;
    }

    template<class H, class E>
    bool operator!=(const Table<Key, Value, H, E>& other) const {
        return ! (*this == other);
    }

    void debugPrintStatus() {
        debugPrintf("Deepest bucket size    = %d\n", (int)debugGetDeepestBucketSize());
        debugPrintf("Average bucket size    = %g\n", debugGetAverageBucketSize());
        debugPrintf("Load factor            = %g\n", debugGetLoad());
    }
};

} // namespace

#ifdef _MSC_VER
#   pragma warning (pop)
#endif

#endif
