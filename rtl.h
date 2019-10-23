#pragma once

//////////////////////////////////////////////////////////////////////////
// Common types
typedef unsigned int COUNT;


//////////////////////////////////////////////////////////////////////////
// CWASSERT() macro

inline void CwShowAssert(const char* file, int line, const char* expression) {
    enum { MESSAGE_MAX = 512 };
    char message[MESSAGE_MAX];
    wsprintfA(message,
        "Assertion failed at %s:%d\r\n"
        "\r\n"
        "%s\r\n"
        "\r\n"
        "Abort: quit process\r\n"
        "Retry: debug\r\n"
        "Ignore: continue execution\r\n",
        file, line, expression);
    
    int result = MessageBoxA(NULL, message,
        "Assertion Failed", MB_ICONERROR | MB_ABORTRETRYIGNORE);
    
    switch (result) {
    case IDABORT:
        ExitProcess(0);
        break;
    case IDRETRY:
        DebugBreak();
        break;
    case IDIGNORE:
        break;
    }
    
}

#define CWASSERT(expr)      if (!(expr)) { CwShowAssert(__FILE__, __LINE__, #expr); }

//////////////////////////////////////////////////////////////////////////
// Memory allocation

void* operator new(COUNT size) {
    return HeapAlloc(GetProcessHeap(), 0, size);
}

void operator delete(void* ptr) {
    HeapFree(GetProcessHeap(), 0, ptr);
}

//////////////////////////////////////////////////////////////////////////
// RangeT and Range - buffer / string views

template<typename T>
struct RangeT {
    T* Start;
    COUNT Length;
};

typedef RangeT<char> Range;

inline Range RS(char* string) {
    Range result = { 0, 0 };
    if (string != NULL) {
        result.Start = string;
        result.Length = lstrlenA(string);
    }
    return result;
}

inline BOOL RangeIsEmpty(const Range r) {
    return r.Start == NULL || r.Length == 0;
}

//////////////////////////////////////////////////////////////////////////
// Vector - growable vector

template<typename T>
struct Vector {
    COUNT Capacity;
    COUNT Length;
    T* Items;
    
    Vector() {
        Capacity = 0;
        Length = 0;
        Items = NULL;
    }
    
    ~Vector() {
        if (Items != NULL) {
            delete[] Items;
            Items = NULL;
        }
        
        Capacity = 0;
        Length = 0;
    }
    
    BOOL CreateWithCapacity(COUNT newCapacity)  {
        BOOL success = FALSE;
        if (newCapacity != 0) {
            T* newItems = new T[newCapacity];
            if (newItems != NULL) {
                Items = newItems;
                Length = 0;
                Capacity = newCapacity;
                
                success = TRUE;
            }
        }
        return success;
    }
    
    BOOL _Grow() {
        BOOL success = FALSE;
        
        COUNT newCapacity = Capacity * 2;
        T* newItems = new T[newCapacity];
        if (newItems != NULL) {
            memcpy(newItems, Items, Length * sizeof(T));
            
            // TODO: this causes a crash due to RefPtr objects
            // being deleted when they shouldn't be
            delete[] Items;
            
            Items = newItems;
            Capacity = newCapacity;
            
            success = TRUE;
        }
        
        return success;
    }
    
    void Push(T& value) {
        if (Length == Capacity) {
            _Grow();
        }
        
        Items[Length] = value;
        Length++;
    }
};

//////////////////////////////////////////////////////////////////////////
// String

struct String {
    char* Buffer;
    COUNT Length;

    String() {
        Buffer = NULL;
        Length = 0;
    }

    ~String() {
        if (Buffer != NULL) {
            delete[] Buffer;
        }
        Length = 0;
    }

    // do not use (yet?)
    String(String&);
    operator=(String&);
    // 

    BOOL CreateWithRange(const Range src) {
        BOOL success = FALSE;
        if (!RangeIsEmpty(src)) {
            char* newBuffer = new char[src.Length + 1];
            if (newBuffer != NULL) {
                COUNT newLength = src.Length;
                memcpy(newBuffer, src.Start, newLength);
                newBuffer[newLength] = '\0';

                Buffer = newBuffer;
                Length = newLength;

                success = TRUE;
            }
        }
        return success;
    }

    BOOL IsEqualRange(const Range compare) {
        BOOL isEqual = FALSE;
        if (!RangeIsEmpty(compare) && Length == compare.Length) {
            isEqual = memcmp(Buffer, compare.Start, compare.Length) == 0;
        }
        return isEqual;
    }

    Range ToRange() {
        Range result = { Buffer, Length };
        return result;
    }
};

//////////////////////////////////////////////////////////////////////////
// StringTable - simple key/value table

struct StringTable {

    struct StringPair {
        String Key;
        String Value;
    };

    Vector<StringPair*> Table;

    ~StringTable() {
        for (COUNT i = 0; i < Table.Length; i++) {
            if (Table.Items[i] != NULL) {
                delete Table.Items[i];
                Table.Items[i] = NULL;
            }
        }
    }
    BOOL CreateWithCapacity(COUNT newCapacity) {
        return Table.CreateWithCapacity(newCapacity);
    }

    BOOL Set(const Range newKey, const Range newValue) {
        BOOL success = FALSE;
        if (!RangeIsEmpty(newKey) && !RangeIsEmpty(newValue)) {
            StringPair* pair = new StringPair;
            if (pair->Key.CreateWithRange(newKey)) {
                if (pair->Value.CreateWithRange(newValue)) {
                    Table.Push(pair);

                    success = TRUE;
                }
            }
        }
        return success;
    }

    BOOL Get(const Range searchKey, String* valueOut) {
        BOOL success = FALSE;
        if (!RangeIsEmpty(searchKey) && valueOut != NULL) {
            for (COUNT i = 0; i < Table.Length; i++) {
                StringPair* pair = Table.Items[i];
                if (pair != NULL) {
                    if (pair->Key.IsEqualRange(searchKey)) {
                        valueOut->CreateWithRange(pair->Value.ToRange());

                        success = TRUE;
                        break;
                    }
                }
            }
        }
        return success;
    }

};

//////////////////////////////////////////////////////////////////////////
// RefPtr - reference counting smart pointer

template<typename T>
struct RefPtr {
    struct RefHolder {
        T* Value;
        COUNT ReferenceCount;
    };

    RefHolder* _Reference;

    RefPtr() {
        _Reference = NULL;
    }

    RefPtr(T* ptr) {
        _Reference = new RefHolder;
        _Reference->Value = ptr;
        _Reference->ReferenceCount = 1;
    }

    RefPtr(RefPtr& src) {
        _UseReference(src);
    }

    ~RefPtr() {
        _Dereference();
    }

    void _UseReference(RefPtr& src) {
        _Reference = src._Reference;
        if (_Reference != NULL) {
            _Reference->ReferenceCount++;
        }
    }

    void _Dereference() {
        if (_Reference != NULL) {
            _Reference->ReferenceCount--;
            if (_Reference->ReferenceCount == 0) {
                delete _Reference->Value;
                delete _Reference;
                
                _Reference = NULL;
            }
        }
    }

    inline T* operator->() {
        HtmlElement* result = NULL;
        if (_Reference != NULL) {
            result = _Reference->Value;
        }
        return result;
    }

    RefPtr& operator=(RefPtr& src) {
        _UseReference(src);
        return *this;
    }

};