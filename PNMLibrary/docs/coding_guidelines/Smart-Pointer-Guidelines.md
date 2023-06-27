## What types of smart pointers exist?
The two common smart pointers in AXDIMM are `std::unique_ptr<>` and `std::shared_ptr<>`. The former is used for singly-owned objects, while the latter is used for reference-counted objects (though normally you should [avoid these](http://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#r21-prefer-unique_ptr-over-shared_ptr-unless-you-need-to-share-ownership) -- see below).

## Why do we use them?
* Smart pointers ensure we properly destroy an object even if its creation and destruction are widely separated. They make functions simpler and safer by ensuring that no matter how many different exit paths exist, local objects are always cleaned up correctly.
* They help enforce that exactly one object owns another object at any given time, preventing both leaks and double-frees.
* Finally, their use clarifies ownership transference expectations at function calls.

## How do we create objects?
* [Prefer scoped objects, don’t heap-allocate unnecessarily](http://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#r5-prefer-scoped-objects-dont-heap-allocate-unnecessarily)

A scoped object is a local object, a global object, or a member. This implies that there is no separate allocation and deallocation cost in excess of that already used for the containing scope or object. The members of a scoped object are themselves scoped and the scoped object’s constructor and destructor manage the members’ lifetimes.
* [Avoid malloc() and free()](http://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#r10-avoid-malloc-and-free)
* [Avoid calling new and delete explicitly](http://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#r11-avoid-calling-new-and-delete-explicitly)

The pointer returned by new should belong to a resource handle (that can call delete). If the pointer returned by new is assigned to a plain/naked pointer, the object can be leaked.
* [Use unique_ptr or shared_ptr to represent ownership](http://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#r20-use-unique_ptr-or-shared_ptr-to-represent-ownership)


## When do we use each smart pointer?
* **Singly-owned** objects - use `std::unique_ptr<>`. Specifically, these are for non-reference-counted, heap-allocated objects that you own.
* **Ref-counted (shared)** objects - use `std::shared_ptr<>`, but better yet, **rethink your design**.
Reference-counted objects make it difficult to understand ownership and destruction order, especially when multiple threads are involved. There is almost always another way to design your object hierarchy to avoid refcounting. Note that **too much of our existing code uses refcounting**, so just because you see existing code doing it does not mean it's the right solution. (Bonus points if you're able to clean up such cases.)
This rule also declared in C++ Core Guidelines [Prefer unique_ptr over shared_ptr unless you need to share ownership](http://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#r21-prefer-unique_ptr-over-shared_ptr-unless-you-need-to-share-ownership).
* **Non-owned** objects - use `raw pointers/references`.

## How do we use smart pointers?
In general, we try to follow following practices described in [ C++ Core Guidelines section R:](http://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#rsmart-smart-pointers)
* [Prefer unique_ptr over shared_ptr unless you need to share ownership](http://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#r21-prefer-unique_ptr-over-shared_ptr-unless-you-need-to-share-ownership)
* [For general use, take T* or T& function arguments rather than smart pointers](http://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#f7-for-general-use-take-t-or-t-arguments-rather-than-smart-pointers)
* [Take smart pointers as parameters only to explicitly express lifetime semantics](http://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#r30-take-smart-pointers-as-parameters-only-to-explicitly-express-lifetime-semantics)
* [Take a unique_ptr<object> parameter to express that a function assumes ownership of a object](http://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#r32-take-a-unique_ptrwidget-parameter-to-express-that-a-function-assumes-ownership-of-a-widget)
* [Take a shared_ptr<object> parameter to express that a function is part owner](http://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#r34-take-a-shared_ptrwidget-parameter-to-express-that-a-function-is-part-owner)