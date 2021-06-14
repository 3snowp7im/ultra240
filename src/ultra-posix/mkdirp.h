#pragma once

#include <sys/stat.h>

namespace ultra::posix {

  void mkdirp(const char* path, mode_t mode = S_IRWXU | S_IRGRP |  S_IXGRP | S_IROTH | S_IXOTH);

}
