#include "threadpool.h"

#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>

static TASK(long, fib, long);

long fib(long n) {
    if (n <= 1)
        return n;

    fib_fut *a = fibAsync((fib_fut[]){fibFuture(n - 1)});
    fib_fut *b = fibAsync((fib_fut[]){fibFuture(n - 2)});

	long out1 = fibAwait(a);
	//printf("out1 ");
	long out2 = fibAwait(b);
	//printf("out2: %li", out2);

    return out1 + out2;
}

int main() {
    if (tpInit(8) != 0)
        perror("Thread Pool initialization failed"), exit(-1);
    atexit(&tpRelease);

    for (long i = 0; i <= 30; ++i){
        printf("fib(%2li) = %li\n", i, fib(i));
	}
    return 0;
}
