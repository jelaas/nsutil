#include <sys/types.h>

struct cg {
  char path[512];
};

ssize_t cgroup_get(const struct cg *cg, char *buf, size_t buflen, const char *name);
int cgroup_put(const struct cg *cg, const char *name, const char *value);
struct cg *cgroup_access(const char *name, int *err);
