For better header dependency tracking, we use following rule for header inclusion (from most local to most global):

```
#include "library local headers"

#include "library common headers"
#include "project common headers"

#include "library test/tools headers"

#include <external libraries headers>

#include <OS system headers>

#include <C/C++ system headers> (C++ headers with "c" prefix are preferable)
```

Different header levels should be split by the new line, for example:

```
#include "core/device/imdb/base.h"
#include "core/memory/sls/accessor.h"

#include "common/compiler_internal.h"

#include <cerrno>
#include <string>
#include <vector>
```

Current project header map of levels by priority:

```
1. #include "Class1.h" (Class header if exists Class1.h -> Class1.cpp)

2. #include "local_headers" (headers located nearby in the same directory)

3. #include "secure/... .h"

4. #include "sls/... .h"

5. #include "imdb/... .h"

6. #include "core/... .h"

7. #include "common/... .h" and #include "hw/... .h"

8. #include "test/... .h" and #include "tools/... .h"

9. #include "pnmlib/secure/... .h"

10. #include "pnmlib/sls/... .h"

11. #include "pnmlib/imdb/... .h"

12. #include "pnmlib/core/... .h"

13. #include "pnmlib/common/... .h"

14. #include <gtest/... .h"

15. #include <CLI/... .h"

16. #include <fmt/... .h"

17. #include <linux/pnm_sls_mem_topology.h>

18. #include <fcntl/fcntl.h> (OS system headers)

19. #include <cerrno> (C++ or C with "c" prefix headers without .h postfix of old style)
```

Please do not use relative paths from custom include directories and do not introduce new ones.

Currently we have public top-level include directory: [/include]
It's used in user API and so should be separate from internal headers.
Whenever you are including user API headers use a path like `"pnmlib/sls/sls.h"`.
**Not** ~~`"include/pnmlib/sls/sls.h"`~~ even though that is also possible.

You can also check the project format including headers section in [.clang-format](.clang-format) for details.
