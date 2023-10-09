#ifndef __ANY_HPP__
#define __ANY_HPP__

#include<algorithm>
#include<typeinfo>
class Any
{
public:
    Any() : content_(nullptr) {}
    template<class T>
    Any(const T& value) : content_(new holder<T>(value)) {}
    Any(const Any& other) : content_(other.content_ ? other.content_->clone() : nullptr) {}
    ~Any() {
        if(content_) delete content_;
    }
    Any& swap(Any& rhs) {
        std::swap(content_, rhs.content_);
        return *this;
    }
    template<class T>
    T* get() {
        if(typeid(T) != content_->type()) return nullptr;
        return &static_cast<holder<T>*>(content_)->value_;
    }
    template<class T>
    Any& operator=(const T& value) {
        Any tmp(value);
        swap(tmp);
        return *this;
    }
    Any& operator=(const Any& other) {
        Any tmp(other);
        swap(tmp);
        return *this;
    }
private:
    class holder_base
    {
    public:
        virtual ~holder_base() {}
        virtual const std::type_info& type() const = 0;
        virtual holder_base* clone() const = 0;
    };
    template<class T>
    class holder : public holder_base
    {
    public:
        holder(const T& value)
            :value_(value)
        {}
        virtual const std::type_info& type() const { return typeid(T); }
        virtual holder_base* clone() const { return new holder(value_); }
    public:
        T value_;
    };
private:
    holder_base* content_;
};



#endif // __ANY_HPP__