#pragma once

namespace Singleton{
    template <typename T>
    class Singleton {
    private:
        static T* _instance;

    protected:
        Singleton() = default;
        ~Singleton() = default;
        Singleton(const Singleton&) = delete;
        Singleton& operator=(const Singleton&) = delete;

    public:
        static T& getInstance() {
            if (_instance != nullptr) {
                return *_instance;
            }else{
                _instance = new T();
                return *_instance;
            }
        }
        static void releaseInstance() {
            if (_instance != nullptr) {
                delete _instance;
                _instance = nullptr;
            }
        }
    };

    template <typename T>
    T* Singleton<T>::_instance = nullptr;
}