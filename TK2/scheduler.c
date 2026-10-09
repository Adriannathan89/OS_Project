#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>

#define LINE_WIDTH 73
#define INF 1000000000   // [TAMBAHAN] limit / quantum "tak terbatas" (Q1 tanpa queue atas, Q3 FCFS)

struct process {
    int pid;
    int arrival_time;
    int burst_time;
    int queue;
    int remaining_time;
    int first_start;       // -1 kalau belum pernah jalan (buat RT)
    int completion_time;   // CT
    int quantum_left;
};

// use linked list
struct node {
    struct process *process;
    struct node *next;
};

struct queue {
    struct node *head;
    struct node *tail;
};

struct context {
    int quantum_1;
    int quantum_2;
};

struct execution_segment {
    int pid; // 0 berarti CPU idle.
    int queue;
    long long start;
    long long end;
    int preempted_remaining; // Sisa BT jika di-preempt pada akhir interval.
    struct execution_segment *next;
};

struct mlq_result {
    struct execution_segment *head;
    struct execution_segment *tail;
    long long higher_preemptions;
};

// line printer function
static void print_line(char c) {
    for (int i = 0; i < LINE_WIDTH; i++) putchar(c);
    putchar('\n');
}

// Function to process input and print the configuration
void process_input(struct process *p[], struct context ctx, int n) {
    printf("\n");
    print_line('=');
    printf("PROCESS INPUT AND QUEUE CONFIGURATION\n");
    print_line('=');

    printf("Q1: RR (quantum=%d) | Q2: RR (quantum=%d) | Q3: FCFS\n",
           ctx.quantum_1, ctx.quantum_2);
    printf("%-3s%15s%11s%9s%18s\n", "PID", "AT", "BT", "Queue", "Algorithm");

    print_line('-');

    for(int i = 0; i < n; i++) {
        char pid_str[16], queue_str[16];
        snprintf(pid_str, sizeof(pid_str), "P%d", p[i]->pid);
        snprintf(queue_str, sizeof(queue_str), "Q%d", p[i]->queue);

        const char *algorithm = (p[i]->queue == 3) ? "FCFS" : "RR";

        printf("%-3s%15d%11d%9s%18s\n",
               pid_str, p[i]->arrival_time, p[i]->burst_time, queue_str, algorithm);
    }

    print_line('=');
    printf("\n");
}

void print_process_queue(struct process *p[], struct context ctx, int n) {
    print_line('=');
    printf("PROCESS QUEUE ASSIGNMENT AND ALGORITHM\n");
    print_line('=');

    for (int i = 0; i < n; i++) {
        printf("P%d -> Q%d -> ", p[i]->pid, p[i]->queue);

        switch (p[i]->queue) {
            case 1:  printf("RR (quantum=%d)\n", ctx.quantum_1); break;
            case 2:  printf("RR (quantum=%d)\n", ctx.quantum_2); break;
            default: printf("FCFS\n"); break;
        }
    }
    printf("\n");
}

// Enqueue ke belakang antrean (FIFO) untuk Requeue Round Robin.
static int queue_push(struct queue *q, struct process *p) {
    struct node *new_node = (struct node *)malloc(sizeof(struct node));
    if (!new_node) return -1;

    new_node->process = p;
    new_node->next = NULL;
    if (q->tail) q->tail->next = new_node;
    else q->head = new_node;
    q->tail = new_node;
    return 0;
}

// Proses yang dipreempt kembali ke depan queue asal.
static int queue_push_front(struct queue *q, struct process *p) {
    struct node *new_node = (struct node *)malloc(sizeof(struct node));
    if (!new_node) return -1;

    new_node->process = p;
    new_node->next = q->head;
    q->head = new_node;
    if (!q->tail) q->tail = new_node;
    return 0;
}

static struct process* queue_pop(struct queue *q) {
    if (q->head == NULL) return NULL; // empty queue

    struct node *n = q->head; // accessing first node in queue
    struct process *p = n->process;

    q->head = n->next;
    if (q->head == NULL) q->tail = NULL;
    free(n); // freeing memory

    return p;
}

// Engine RR dari Dimas: waktu bertambah di engine, queue diurus dispatcher.
#define RR_RUNNING  0
#define RR_DONE     1
#define RR_QUANTUM  2

void rr_start(struct process *p, int quantum, int now) {
    p->quantum_left = quantum;
    if (p->first_start == -1) p->first_start = now;
}

int execute_rr_tick(struct process *p, int *current_time) {
    p->remaining_time--;
    p->quantum_left--;
    (*current_time)++;

    if (p->remaining_time == 0) {
        p->completion_time = *current_time;
        return RR_DONE;
    }
    if (p->quantum_left == 0) return RR_QUANTUM;
    return RR_RUNNING;
}

// FCFS Q3 memakai waktu bersama, tanpa batas quantum.
static int execute_fcfs_tick(struct process *p, int *current_time) {
    p->remaining_time--;
    (*current_time)++;
    if (p->remaining_time == 0) {
        p->completion_time = *current_time;
        return RR_DONE;
    }
    return RR_RUNNING;
}

static int admit_arrivals(
    struct process *p[],
    int n,
    unsigned char admitted[],
    struct queue ready_queue[],
    long long time
){
    for (int i=0; i<n; i++) {
        if (!admitted[i] && p[i]->arrival_time <= time) {
            if (queue_push(
                &ready_queue[p[i]->queue - 1], p[i]
            ) != 0)
                return -1;

            admitted[i] = 1;
        }
    }

    return 0;
}


static int select_queue(struct queue ready_queue[]) {
    for (int i = 0; i < 3; i++) {
        if (ready_queue[i].head != NULL)
            return i;
    }

    return -1;
}


static int record_execution(
    struct mlq_result *result,
    struct process *p,
    long long start,
    long long end
) {
    int pid = p ? p->pid : 0;
    int queue = p ? p->queue : 0;

    // gabungkan interval berurutan dari proses yang sama.
    if (result->tail &&
        result->tail->pid == pid &&
        result->tail->queue == queue &&
        result->tail->end == start) {

        result->tail->end = end;
        return 0;
    }

    struct execution_segment *segment = (struct execution_segment *)malloc(sizeof(*segment));

    if (!segment) return -1;

    segment->pid = pid;
    segment->queue = queue;
    segment->start = start;
    segment->end = end;
    segment->preempted_remaining = 0;
    segment->next = NULL;

    if (result->tail) result->tail->next = segment;
    else result->head = segment;

    result->tail = segment;
    return 0;
}


static void record_preemption(
    struct mlq_result *result,
    struct process *previous,
    struct process *running
) {
    if (previous != NULL &&
        running->queue < previous->queue) {

        result->tail->preempted_remaining =
            previous->remaining_time;

        result->higher_preemptions++;
    }
}

static long long next_arrival_time(
    struct process *p[],
    int n,
    unsigned char admitted[]
) {
    long long next = LLONG_MAX;

    for (int i=0; i<n; i++) {
        if (!admitted[i] && p[i]->arrival_time < next) {
            next = p[i]->arrival_time;
        }
    }

    return next;
}

static void free_mlq_result(struct mlq_result *result) {
    struct execution_segment *segment = result->head;

    while (segment) {
        struct execution_segment *next = segment->next;
        free(segment);
        segment = next;
    }

    result->head = NULL;
    result->tail = NULL;
    result->higher_preemptions = 0;
}


// Dispatcher mengurus ready queue; engine hanya menjalankan satu tick.
// Sesuai remote: requeue mendahului arrival berikutnya, quantum reset saat resume.
static int simulate_mlq(
   struct process *p[],
   int n,
   struct context ctx,
   struct mlq_result *result
) {
    struct queue ready_queue[3] = {
        {NULL, NULL},
        {NULL, NULL},
        {NULL, NULL}
    };

    result->head = NULL;
    result->tail = NULL;
    result->higher_preemptions = 0;

    if (n <= 0 || ctx.quantum_1 <= 0 || ctx.quantum_2 <= 0) return -1;

    unsigned char *admitted = (unsigned char *)calloc((size_t)n, sizeof(*admitted));

    if (!admitted) return -1;

    int time = 0;
    int completed = 0;
    int error = 0;
    struct process *running = NULL;

    while (completed < n) {
        if (admit_arrivals(p, n, admitted, ready_queue, time) != 0) {
            error = 1;
            break;
        }

        int selected = select_queue(ready_queue);

        if (running && selected != -1 && selected < running->queue - 1) {
            record_preemption(result, running, ready_queue[selected].head->process);
            if (queue_push_front(&ready_queue[running->queue - 1], running) != 0) {
                error = 1;
                break;
            }
            running = NULL;
        }

        if (!running && selected == -1) {
            long long next = next_arrival_time(p, n, admitted);

            if (next == LLONG_MAX ||
                record_execution(result, NULL, time, next) != 0) {
                error = 1;
                break;
            }

            time = (int)next;
            continue;
        }

        if (!running) {
            running = queue_pop(&ready_queue[selected]);
            if (selected < 2) {
                int quantum = selected == 0 ? ctx.quantum_1 : ctx.quantum_2;
                rr_start(running, quantum, time);
            } else if (running->first_start == -1) {
                running->first_start = time;
            }
        }

        // Interface waktu/CT remote memakai int; cegah overflow sebelum engine.
        if (time == INT_MAX ||
            record_execution(result, running, time, (long long)time + 1) != 0) {
            error = 1;
            break;
        }

        int status;
        if (running->queue == 3) {
            status = execute_fcfs_tick(running, &time);
        } else {
            status = execute_rr_tick(running, &time);
        }

        if (status == RR_DONE) {
            completed++;
            running = NULL;
        } else if (status == RR_QUANTUM) {
            if (queue_push(&ready_queue[running->queue - 1], running) != 0) {
                error = 1;
                break;
            }
            running = NULL;
        }
    }

    // Pada sukses semua antrean kosong; pada gagal bersihkan sisa node.
    for (int i = 0; i < 3; i++) {
        while (queue_pop(&ready_queue[i]) != NULL) {
        }
    }
    free(admitted);
    if (error) {
        free_mlq_result(result);
        return -1;
    }
    return 0;
}
// Read a whole token so malformed or overflowing integers are rejected.
static int read_int(int *value) {
    char token[64];
    size_t length = 0;
    int c, too_long = 0;

    do {
        c = getchar();
    } while (c != EOF && isspace(c));
    if (c == EOF) return -1;

    do {
        if (length < sizeof(token) - 1) token[length++] = (char)c;
        else too_long = 1;
        c = getchar();
    } while (c != EOF && !isspace(c));
    token[length] = '\0';
    if (too_long) return -1;

    char *end;
    errno = 0;
    long parsed = strtol(token, &end, 10);
    if (errno == ERANGE || end == token || end != token + length ||
        parsed < INT_MIN || parsed > INT_MAX) return -1;
    *value = (int)parsed;
    return 0;
}

static void cleanup(struct process *p[], struct queue queue_list[], int allocated) {
    for (int i = 0; i < 3; i++) {
        struct node *node = queue_list[i].head;
        while (node) {
            struct node *next = node->next;
            free(node);
            node = next;
        }
        queue_list[i].head = NULL;
        queue_list[i].tail = NULL;
    }
    for (int i = 0; i < allocated; i++) free(p[i]);
    free(p);
}

int main(void) {
    int n, quantum_1, quantum_2;

    // Input jumlah proses dan quantum untuk masing-masing queue
    printf("Jumlah proses: ");
    if (read_int(&n) != 0 || n <= 0) {
        printf("Jumlah proses harus berupa bilangan bulat lebih besar dari 0.\n");
        return 1;
    }
    printf("Quantum Q1 (RR > 0): ");
    if (read_int(&quantum_1) != 0) {
        printf("Quantum Q1 harus berupa bilangan bulat.\n");
        return 1;
    }
    printf("Quantum Q2 (RR > 0): ");
    if (read_int(&quantum_2) != 0) {
        printf("Quantum Q2 harus berupa bilangan bulat.\n");
        return 1;
    }

    // input validation
    if(quantum_1 <= 0 || quantum_2 <= 0) {
        printf("Quantum harus lebih besar dari 0.\n");
        return 1;
    }

    // Allocate memory for process pointers
    struct process **p = (struct process **)calloc((size_t)n, sizeof(*p));
    if (!p) {
        printf("Gagal mengalokasikan memori untuk daftar proses.\n");
        return 1;
    }
    int allocated = 0;

    // Allocate memory for queue process
    struct queue queue_list[3] = {{NULL, NULL}, {NULL, NULL}, {NULL, NULL}}; // Initialize queues for Q1, Q2, Q3

    struct context ctx = {quantum_1, quantum_2};

    for(int i = 0; i < n; i++) {
        int at, bt, queue_choice;
        printf("P%d - masukkan AT BT queue: ", i+1);
        if (read_int(&at) != 0 || read_int(&bt) != 0 ||
            read_int(&queue_choice) != 0) {
            printf("AT, BT, dan queue harus berupa bilangan bulat lengkap.\n");
            cleanup(p, queue_list, allocated);
            return 1;
        }

        if(at < 0 || bt <= 0) {
            printf("AT harus >= 0 dan BT harus > 0.\n");
            cleanup(p, queue_list, allocated);
            return 1;
        }
        if (queue_choice < 1 || queue_choice > 3) {
            printf("Queue harus bernilai 1, 2, atau 3.\n");
            cleanup(p, queue_list, allocated);
            return 1;
        }

        // Allocate memory for each process
        p[i] = (struct process *)malloc(sizeof(*p[i]));
        if (!p[i]) {
            printf("Gagal mengalokasikan memori untuk proses P%d.\n", i + 1);
            cleanup(p, queue_list, allocated);
            return 1;
        }
        allocated++;
        p[i]->pid = i + 1;
        p[i]->arrival_time = at;
        p[i]->burst_time = bt;
        p[i]->queue = queue_choice; // initial queue
        p[i]->remaining_time = bt; // initial value
        p[i]->first_start = -1;
        p[i]->completion_time = 0;
        p[i]->quantum_left = 0;
    }

    process_input(p, ctx, n);
    print_process_queue(p, ctx, n);
    struct mlq_result result;
    int status = simulate_mlq(p, n, ctx, &result);
    if (status != 0) printf("Simulasi gagal: alokasi memori atau waktu melebihi INT_MAX.\n");

    // Reporting Hisyam menggunakan result di sini, sebelum dibebaskan.
    free_mlq_result(&result);
    cleanup(p, queue_list, allocated);
    return status == 0 ? 0 : 1;
}
