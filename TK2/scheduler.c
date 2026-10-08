#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>

#define LINE_WIDTH 73

struct process {
    int pid;
    int arrival_time;
    int burst_time;
    int queue;
    int remaining_time;
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

static int queue_push(struct queue *q, struct process *p) {
    struct node *n = (struct node *)malloc(sizeof(struct node));
    if (!n) return -1;

    n->process = p;
    n->next = NULL;

    if (q->tail == NULL) {
        q->head = n;
        q->tail = n;
    } else {
        q->tail->next = n;
        q->tail = n;
    }
    return 0;
}

static struct process *queue_pop(struct queue *q) {
   if (q->head == NULL) return NULL;

   struct node *n = q->head;
   struct process *p = n->process;

   q->head = q->head->next;
   if (q->head == NULL) q->tail = NULL;

   free(n);
   return p;
}

static void simulate_fcfs(struct queue *q) {
    int current_time = 0;

    while (q->head != NULL) {
        struct process *p = queue_pop(q);
        
        if (current_time < p->arrival_time) {
            current_time = p->arrival_time;
        }

        int start_time = current_time;
        current_time += p->remaining_time;
        p->remaining_time = 0;

        printf("P%d: %d -> %d\n", p->pid, start_time, current_time);
    }
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

    // Allocate memory for queue processes
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

        // Add process to the appropriate queue
        if(queue_push(&queue_list[queue_choice - 1], p[i]) != 0) {
            printf("Gagal menambahkan proses ke queue %d.\n", queue_choice);
            cleanup(p, queue_list, allocated);
            return 1;
        }
    }

    process_input(p, ctx, n);
    print_process_queue(p, ctx, n);
    simulate_fcfs(&queue_list[2]);
    cleanup(p, queue_list, allocated);
    return 0;
}
