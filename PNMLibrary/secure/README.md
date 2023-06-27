## SecNDP library structure

```
secure/ ----------------------------- Top level directory
├── CMakeLists.txt
├── common -------------------------- Common high-level api classes. 
│   └── action_queue ---------------- Action queue's sources for producer/consumer model
├── encryption ---------------------- Classes for encryption/decryption processes
│   └── AES ------------------------- Implementation of AES algorithm via NI instruction set
└── plain --------------------------- Plain SecNDP library
    ├── devices --------------------- NDP devices
    └── run_strategies -------------- Source files that define SLS run strategies
```
