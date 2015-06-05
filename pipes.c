#include <argp.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define xstr(s) str(s)
#define str(s) #s

#define DEFAULT_ITERS 100000
#define DEFAULT_WARMUP_ITERS 1000

static struct argp_option options[] = {
    {"iters", 'i', "COUNT", 0, "number of iterations to measure; default: " xstr(DEFAULT_ITERS), 0},
    {0, 'n', "COUNT", OPTION_ALIAS, 0, 0},
    {"warmup-iters", 'w', "COUNT", 0, "number of iterations before measurement; default: " xstr(DEFAULT_WARMUP_ITERS), 0},
    {0}
};

typedef struct {
    int iters;
    int warmup_iters;
} args;

static error_t parse_opt(int key, char* arg, struct argp_state* state) {
    args* args = state->input;

    switch(key) {
        case 'i':
        case 'n':
            args->iters = atoi(arg);
            break;
        case 'w':
            args->warmup_iters = atoi(arg);
            break;
        default:
            return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

static struct argp argp = { options, parse_opt, 0, 0, 0, 0, 0};

void parent_loop(int warmup_iters, int iters, int tx_fd, int rx_fd) {
    int i;
    struct timespec start, end;

    for (i = 0; i < warmup_iters; ++i) {
        char resp;
        if (write(tx_fd, "0", 1) == -1) {
            perror("could not write");
            break;
        };

        if (read(rx_fd, &resp, 1) == -1) {
            perror("could not read");
            break;
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &start);

    for (i = 0; i < iters; ++i) {
        char resp;
        if (write(tx_fd, "0", 1) == -1) {
            perror("could not write");
            break;
        };

        if (read(rx_fd, &resp, 1) == -1) {
            perror("could not read");
            break;
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &end);

    long long elapsed_nsec = (end.tv_sec - start.tv_sec) * 1000000000 + (end.tv_nsec - start.tv_nsec);

    fprintf(stderr, "%d iters in %lld ns\n %f ns/iter\n", iters, elapsed_nsec, (double) elapsed_nsec / iters);

    exit(EXIT_SUCCESS);
}

void child_loop(int tx_fd, int rx_fd) {
    for (;;) {
        char msg;
        read(rx_fd, &msg, 1);
        write(tx_fd, "1", 1);
    }
}

int main(int argc, char **argv) {
    args args;

    args.iters = DEFAULT_ITERS;
    args.warmup_iters = DEFAULT_WARMUP_ITERS;

    argp_parse(&argp, argc, argv, 0, 0, &args);

    int tx[2], rx[2];  // relative to parent
    if (pipe(tx)) {
        // uh-oh
    }
    if (pipe(rx)) {
        // uh-oh
    }

    pid_t child = fork();
    if (child == -1) {
        // uh-oh
    }
    else if (child) {
        close(tx[0]);
        close(rx[1]);

        parent_loop(args.warmup_iters, args.iters, tx[1], rx[0]);
    } else {
        close(tx[1]);
        close(rx[0]);

        child_loop(rx[1], tx[0]);
    }

    return 0;
}
