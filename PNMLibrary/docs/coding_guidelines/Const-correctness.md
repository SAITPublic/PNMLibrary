### When to use const/constexpr
* Following [C++ Core Guidelines](http://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Res-const) suggestions for const and constexpr qualifiers:
    declare an object const or constexpr unless you want to modify its value later on.
    That way you can’t change the value by mistake. That way might offer the compiler optimization opportunities.

### When not to use const/constexpr
* Beware that constness prohibits objects movement.
    There are several demonstrative examples:
    * const argument
        ```C++
        const string str;
        std::move(str);
        ```
    * passing result as a const reference argument
        ```C++
         void func(const string& var);
         string str;
         func(std::move(str));
         ```
    *  class/struct with const fields
        ```C++
        struct Foo{
            const std::vector<int> v;
        };
        Foo f;
        Foo c = std::move(f);
        ```
    * const result type
        ```C++
        const string foo() {
            return string("message");
        }
        string str = std::move(foo());
        ```
