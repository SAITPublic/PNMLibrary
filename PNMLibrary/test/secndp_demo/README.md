## SecNDP demo application

The application demonstrates the usage of secure library to perform SLS. The app. can be used
as a unit test for new core parts of library.

At this moment app. perform SLS in 15 different ways combined into 3 main run approaches
(sequential, multithread and multiprocess). Each group run secure SLS with 5 runners that utilize different
devices, launch policies and apis.

To run app use next command:

```shell
./secndp_demo <num_instances> <root_path> <indices_name> <tag>
```

`num_instances` -- number of thread of processes for multithread/process tests
`root_path` -- path to a folder with embedded tables
`indices_name` -- name of indices set that located in the root_path
`tag` -- is this args is set the app. will use MAC to verifies SLS result.