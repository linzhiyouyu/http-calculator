#ifndef __BUFFER_HPP__
#define __BUFFER_HPP__
#include<iostream>
#include<vector>
#include<algorithm>
#include<cassert>
#define BUFFER_DEFAULT_SIZE 1024 
#define CRLF "\r\n"
class Buffer
{
public:
    Buffer(uint64_t size = BUFFER_DEFAULT_SIZE)
        : buffer_(size), read_index_(0), write_index_(0)
    {}
public:
    char* _begin() { return &*buffer_.begin(); }
    char* read_start_position() { return _begin() + read_index_; }
    char* write_start_position() { return _begin() + write_index_; }
    uint64_t tail_idle_size() { return buffer_.size() - write_index_; }
    uint64_t head_idle_size() { return read_index_; }
    uint64_t readable_size() { return write_index_ - read_index_; }
    void move_read_offset(size_t len) { 
        if(len == 0) return;
        assert(read_index_ + len <= write_index_);
        read_index_ += len; 
    }
    void move_write_offset(size_t len) { 
        if(len == 0) return;
        assert(len <= tail_idle_size());
        write_index_ += len;
    }
    void ensure_write_space(size_t len) {
        if(len <= tail_idle_size()) return;
        else if(len <= tail_idle_size() + head_idle_size()) {
            uint64_t rsz = readable_size();
            std::copy(_begin() + rsz, _begin(), _begin());
            write_index_ = rsz; 
            read_index_ = 0;
        } else {
            buffer_.resize(write_index_ + len);
        }
    }
    void write(const void* data, size_t len) {
        ensure_write_space(len);
        std::copy((char*)data, (char*)data + len, write_start_position());
    }
    void write_string(const std::string& str) {
        write((void*)&str[0], str.size());
    }
    void write_string_and_push(const std::string& str) {
        write_string(str);
        move_write_offset(str.size());
    }
    void write_buffer(Buffer& buffer) {
        write(buffer.read_start_position(), buffer.readable_size());
    }
    void write_buffer_and_push(Buffer& buffer) {
        write_buffer(buffer);
        move_write_offset(buffer.readable_size());
    }
    void read(void* data, size_t len) {
        assert(len <= readable_size());
        std::copy(read_start_position(), read_start_position() + len, (char*)data);
    }
    std::string read_as_string(size_t len) {
        assert(len <= readable_size());
        std::string str;
        str.resize(len);
        read((void*)&str[0], len);
        return str;
    }
    std::string read_as_string_and_pop(size_t len) {
        std::string str = read_as_string(len);
        move_read_offset(len);
        return str;
    }
    char* findCRLF() {
        char* crlf = std::search(read_start_position(), write_start_position(), CRLF, CRLF + 2);
        return crlf == write_start_position() ? nullptr : crlf;    
    }
    std::string get_line() {
        char* crlf = findCRLF();
        if(crlf == nullptr) return "";
        std::string line = read_as_string(crlf - read_start_position() + 2);
        return line;
    }
    std::string get_line_and_pop() {
        std::string line = get_line();
        move_read_offset(line.size());
        return line;
    }
    void clear() {
        read_index_ = 0;
        write_index_ = 0;
    }
private:
    std::vector<char> buffer_;
    uint64_t read_index_;
    uint64_t write_index_;
};

#endif // __BUFFER_HPP__