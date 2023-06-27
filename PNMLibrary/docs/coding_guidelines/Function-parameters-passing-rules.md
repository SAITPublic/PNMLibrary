In general, we are trying to conform [C++ Core Guidelines](http://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#fcall-parameter-passing) suggestions for passing function parameters:

1) Cheap to copy -> pass by value ([rule F16](http://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#f16-for-in-parameters-pass-cheaply-copied-types-by-value-and-others-by-reference-to-const)).
2) Don't pass by const value, it's meaningless and prohibits move semantics.
    * This is true for [primitive types as well](http://www.cplusplus.com/forum/general/135731/#msg723591).
3) Non-modifiable, non-null -> const reference ([rule F16](http://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#f16-for-in-parameters-pass-cheaply-copied-types-by-value-and-others-by-reference-to-const)).
4) Non-modifiable, can be null -> const pointer.
5) Modifiable, non-null -> reference ([rule F17](http://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#f17-for-in-out-parameters-pass-by-reference-to-non-const)).
6) Modifiable, can be null -> pointer ([rule F60](http://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#f60-prefer-t-over-t-when-no-argument-is-a-valid-option)).
7) Modifiable, can transfer ownership -> unique_ptr<object> by value ([rule F26](http://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#f26-use-a-unique_ptrt-to-transfer-ownership-where-a-pointer-is-needed)).
8) Modifiable, can reseat the pointer -> unique_ptr<object> by reference ([rule R33](http://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#r33-take-a-unique_ptrwidget-parameter-to-express-that-a-function-reseats-thewidget))
9) Modifiable, the function is part owner -> shared_ptr<object> by value ([rule F27](http://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#f27-use-a-shared_ptrt-to-share-ownership)).
10) Modifiable, the function might reset the shared pointer -> shared_ptr<object> by reference ([rule R35](http://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#r35-take-a-shared_ptrwidget-parameter-to-express-that-a-function-might-reseat-the-shared-pointer)).