
//#ifndef OFXVP_BUILD_WITH_MINIMAL_OBJECTS

#pragma once

#if __cplusplus>=202002L
#include <bit>
#endif

template<typename TYPE>
inline TYPE* ofxVPD_GET_PIN_PTR_LVALUE(void*& _pinData){
    return static_cast<TYPE*>(_pinData);
}
// Specialisations/Exceptions : only float is using bit-cast style type-punning
// Basically it reads/writes the address of a pointer as a value using a bitswap, instead of writing the value to the pointed address.
// Note: Accessing the pointer normally will naturally crash the program; it's accessing an invalid memory address.
// Info: https://tttapa.github.io/Pages/Programming/Cpp/Practices/type-punning.html
template<>
inline float* ofxVPD_GET_PIN_PTR_LVALUE(void*& _pinData){
    // Compile-time check, will warn on platforms where this technique doesn't work !
    static_assert(sizeof(float) == sizeof(uint32_t));
#if __cplusplus>=202002L // C++20 brings native bitcast support !
    return std::bit_cast<float*>(&_pinData); // untested !
#else
    return reinterpret_cast<float *>(&_pinData);
#endif
};
