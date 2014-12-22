//
//  Buffer.h
//  SwiftSnails
//
//  Created by Chunwei on 12/22/14.
//  Copyright (c) 2014 Chunwei. All rights reserved.
//
#include "common.h"
#include "string.h"
namespace swift_snails {

/*
 * 将对象序列化，便于网络传输
 * 管理动态分配内存
 */
class BasicBuffer {
public:
    explicit BasicBuffer() {
    }
    BasicBuffer(const BasicBuffer&) = delete;
    explicit BasicBuffer(BasicBuffer&& other) :
        _buffer(other._buffer),
        _cursor(other._cursor),
        _end(other._end),
        _capacity(other._capacity)
    {
        // clear other's pointer
        other.clear();
    }

    ~BasicBuffer() {
        free();
    }

    char* buffer() {
        return _buffer;
    }
    char* cursor() {
        return _cursor;
    }
    char* end() {
        return _end;
    }
    size_t capacity() {
        return _capacity;
    }   
    size_t size() {
        return end() - buffer();
    }

    void set_cursor(char* x) {
        CHECK(_cursor < end());
        _cursor = x;
    }
    void set_end(char* x) {
        _end = x;
    }
    void reset_cursor() { 
        _cursor = buffer();
    }

    bool read_finished() {
        CHECK(cursor() <= end());
        return cursor() == end();
    }
protected:
    void reserve(size_t newcap) {
        if(newcap > capacity()) {
            char* newbuf;
            newbuf = new char[newcap];
            if (size() > 0) {
                memcpy(newbuf, buffer(), size());
            }
            _cursor = newbuf + (cursor() - buffer());
            _end = _cursor + 1;
            _capacity = newcap;
            free(); // free previous memory
            _buffer = newbuf;
        }
    }
    // on put mod
    // will change end and 
    // should not use in read mod
    void put_cursor_preceed(size_t size) {
      CHECK(cursor() + size < end());
      _cursor += size;
      _end = _cursor + 1;
    }

    void free() {
        if(_buffer) {
            delete _buffer; 
            clear();
        }
    }
    // clear status
    void clear() {
        _buffer = nullptr;  // should move or delete content of _buffer before
        _cursor = nullptr;
        _end = nullptr;
        _capacity = 0;
    }

private:
    char* _buffer = nullptr;
    char* _cursor = nullptr;
    char* _end = nullptr;   // the next byte of valid buffer's tail
    size_t _capacity = 0;
};  // end class BasicBuffer

// 二进制buffer管理
class BinaryBuffer  : public BasicBuffer {
public:
    // define << operator for basic types
    #define SS_REPEAT_PATTERN(T) \
    BasicBuffer& operator>>(T& x) { \
        get_raw(x); \
        return *this; \
    } \
    BasicBuffer& operator<<(const T& x) { \
        put_raw(x); \
        return *this; \
    }
    SS_REPEAT6(int16_t, uint16_t, int32_t, uint32_t, int64_t, uint64_t)
    SS_REPEAT2(double, float)
    SS_REPEAT1(bool)
    SS_REPEAT1(byte_t)
    #undef SS_REPEAT_PATTERN

    template<typename T>
    T get() {
        T x;
        *this >> x;
        return std::move(x);
    }

protected:
    // T should be basic types
    template<typename T>
    void get_raw(T& x) {
        CHECK(! read_finished());
        memcpy(&x, cursor(), sizeof(T));
        put_cursor_preceed(sizeof(T));
    }

    template<typename T>
    T get_raw() {
        T x;
        CHECK(! read_finished());
        memcpy(&x, cursor(), sizeof(T));
        put_cursor_preceed(sizeof(T));
        return std::move(x);
    }

    template<typename T>
    void put_raw(T& x) {
        if( size() + sizeof(x) > capacity()) {
            size_t newcap = 2 * capacity();
            reserve(newcap);
        }
        memcpy(end(), &x, sizeof(T));
        put_cursor_preceed(sizeof(T));
    }

};  // end class BinaryBuffer

/*
 * 以行为单位管理文本
 * 可以用于数据分发
 */
class TextBuffer    : public BasicBuffer {
public:
    template<typename T>
    void put_math(T& x) {   // just copy data's string to buffer
        std::string x_ = std::to_string(x);
        *this << x_;
    }
    // ints
    #define SS_REPEAT_PATTERN(T) \
    void get_math(T& x) { \
        char *cend = nullptr; \
        x = (T)strtol(cursor(), &cend, 10); \
        set_cursor(cend); \
    }
    SS_REPEAT4(int16_t, int32_t, int64_t, bool)
    #undef SS_REPEAT_PATTERN

    // uints
    #define SS_REPEAT_PATTERN(T) \
    void get_math(T& x) { \
        char *cend = nullptr; \
        x = (T)strtoul(cursor(), &cend, 10); \
        set_cursor(cend); \
    }
    SS_REPEAT3(uint16_t, uint32_t, uint64_t)
    #undef SS_REPEAT_PATTERN

    // float double
    #define SS_REPEAT_PATTERN(T) \
    void get_math(T& x) { \
        char *cend = nullptr; \
        x = (T)strtod(cursor(), &cend); \
        set_cursor(cend); \
    }
    SS_REPEAT2(float, double)
    #undef SS_REPEAT_PATTERN

    TextBuffer &operator<< (const std::string &x) {
        //swift_snails::trim(x);
        if(x.size() + size() > capacity()) {
            size_t newcap = 2 * capacity();
            reserve(newcap);
        }
        memcpy(buffer(), x.c_str(), x.size());
        put_cursor_preceed(x.size());
        set_end(cursor() + 1);
        return *this;
    }
    #define SS_REPEAT_PATTERN(T) \
    TextBuffer &operator<< (T x) { \
        std::string x_ = std::to_string(x); \
        x_ += " "; \
        *this << x_; \
        return *this; \
    } 
    SS_REPEAT6(int16_t, uint16_t, int32_t, uint32_t, int64_t, uint64_t)
    SS_REPEAT3(bool, double, float)
    #undef SS_REPEAT_PATTERN

    #define SS_REPEAT_PATTERN(T) \
    TextBuffer& operator>> (T &x) { \
        get_math(x); \
        return *this; \
    }
    SS_REPEAT6(int16_t, uint16_t, int32_t, uint32_t, int64_t, uint64_t)
    SS_REPEAT3(bool, double, float)
    #undef SS_REPEAT_PATTERN

};  // end class TextBuffer

};