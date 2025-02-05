#pragma once
#include <memory>
/**
 * @brief The Singleton class template.
 * 使用 std::unique_ptr 自动管理单例实例的生命周期。
 */
template <typename T>
class Singleton {
public:
    static std::shared_ptr<T>& getInstance()
    {
        static std::shared_ptr<T> instance(new T());
        return instance;
    }
protected:
    // 保护构造函数，防止外部构造
    Singleton() = default;
    ~Singleton() = default;
    Singleton(const Singleton&) = delete; // 禁止拷贝构造
    Singleton& operator=(const Singleton&) = delete; // 禁止赋值操作
};
#define SINGLETON(Class)                         \
private:                                         \
    friend class Singleton<Class>;               \
                                                 \
public:                                          \
    static std::shared_ptr<Class>& getInstance() \
    {                                            \
        return Singleton<Class>::getInstance();  \
    }                                            \
                                                 \
protected:                                       \
    Class(const Class&) = delete;                \
    Class& operator=(const Class&) = delete;     \
                                                 \
public:
