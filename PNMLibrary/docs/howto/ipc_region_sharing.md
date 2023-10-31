# Share region between separate processes

## Steps to share region

1. Create a buffer
2. Call method `make_shared` of this buffer. This method returns object of `SharedRegion` class
3. Call method `get_serialized` of this object. This method returns vector of serialized regions
4. Send this vector via some IPC mechanism to another process
5. In the process-acceptor receive vector of serialized regions and create new object of `SharedRegion`
6. Construct new buffer with this object as an argument
7. Now you are able to use buffer with values, which was originally created by another process


## Example

1. Sender part:
```
constexpr size_t buffer_size = 100;
pnm::memory::Buffer<int> buffer(buffer_size, context);

// "serialized" is a vector of serialized regions
pnm::memory::SharedRegion::SerializedRegion serialized = buffer.make_shared().get_serialized();
```
Then send serialized regions to another process.

2. Receiver part:
```
const pnm::memory::SharedRegion shared(serialized);
pnm::memory::Buffer<int> shared_buffer(shared, context);
```