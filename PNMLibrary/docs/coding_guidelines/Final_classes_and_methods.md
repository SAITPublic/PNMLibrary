## General info about final keyword:
At the moment, the design of our library has not been completed, so the use of the `final` keyword when inheriting classes and overriding virtual methods is prohibited (strictly not recommended).
If you want to mark a method or class as a `final` one, check it with your colleagues and be sure to add a comment about it.

## final method
We should make method `final` for next reason:

1. significant performance improvement. We should make a comment about it and provides benchmarks that proves this statement.
2. We definitely don't want to allow override the method in derived classes. We also should make a comment why we do that.

In other case the method should be marked `override`.

## final class
The same statement is true for class:

We should make it `final` when we get performance improvement (make comments + benchmarks) or we definitely want to prevent inheritance from this class (leave comment why).

## Delete final keyword
We must not discard `final` modifier when we face it in code.
If you still want to do this, then you need to discuss it with your colleagues.
