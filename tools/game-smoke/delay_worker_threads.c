// Test-only scheduler perturbation; never linked into the runtime.
#define _GNU_SOURCE
#include <dlfcn.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct worker_start {
    void *(*function)(void *);
    void *argument;
};

static void *delayed_start(void *argument) {
    struct worker_start start = *(struct worker_start *)argument;
    free(argument);
    usleep(150000);
    return start.function(start.argument);
}

int pthread_create(pthread_t *thread, const pthread_attr_t *attributes,
                   void *(*function)(void *), void *argument) {
    int (*create)(pthread_t *, const pthread_attr_t *, void *(*)(void *),
                  void *) = dlsym(RTLD_NEXT, "pthread_create");
    if(!create)
        return ENOSYS;
    char executable[4096];
    const ssize_t size =
        readlink("/proc/self/exe", executable, sizeof(executable) - 1);
    if(size <= 0)
        return create(thread, attributes, function, argument);
    executable[size] = 0;
    const char *name = strrchr(executable, '/');
    if(!name || strcmp(name + 1, "krkr2"))
        return create(thread, attributes, function, argument);
    struct worker_start *start = malloc(sizeof(*start));
    if(!start)
        return ENOMEM;
    *start = (struct worker_start){ function, argument };
    const int result = create(thread, attributes, delayed_start, start);
    if(result)
        free(start);
    return result;
}
