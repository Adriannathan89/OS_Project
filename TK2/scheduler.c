#include <stdio.h>
#include <stdlib.h>

#define LINE_WIDTH 73

struct process {
    int pid;
    int arrival_time;
    int burst_time;
    int queue;
};

// use linked list
struct queue {
    struct process **processes;
    struct queue *next;
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
    struct queue *new_queue = (struct queue *)malloc(sizeof(struct queue));
    if (!new_queue) return -1; // Memory allocation failed

    new_queue->processes = &p;
    new_queue->next = q->next;
    q->next = new_queue;
    return 0; // Success
}

int main() {
    int n, quantum_1, quantum_2;

    // Input jumlah proses dan quantum untuk masing-masing queue
    printf("Jumlah proses: ");
    scanf("%d", &n);
    printf("Quantum Q1 (RR > 0): ");
    scanf("%d", &quantum_1);
    printf("Quantum Q2 (RR > 0): ");
    scanf("%d", &quantum_2);

    // input validation
    if(quantum_1 <= 0 || quantum_2 <= 0) {
        printf("Quantum harus lebih besar dari 0.\n");
        return 1;
    }

    // Allocate memory for process pointers
    struct process *p[n];

    // Allocate memory for queue processes
    struct queue queue_list[3] = {0}; // Initialize queues for Q1, Q2, Q3

    struct context ctx = {quantum_1, quantum_2};

    for(int i = 0; i < n; i++) {
        int at, bt, queue_choice;
        printf("P%d - masukkan AT BT queue: ", i+1);
        scanf("%d %d %d", &at, &bt, &queue_choice);

        if(at < 0 || bt <= 0) {
            printf("AT harus >= 0 dan BT harus > 0.\n");
            return 1;
        }

        // Allocate memory for each process
        p[i] = (struct process *)malloc(sizeof(struct process));
        p[i]->pid = i + 1;
        p[i]->arrival_time = at;
        p[i]->burst_time = bt;
        p[i]->queue = queue_choice; // initial queue

        // Add process to the appropriate queue
        if(queue_push(&queue_list[queue_choice - 1], p[i]) != 0) {
            printf("Gagal menambahkan proses ke queue 0.\n");
            return 1;
        }
    }

    process_input(p, ctx, n);
    print_process_queue(p, ctx, n);
}