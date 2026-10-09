import os
from pathlib import Path
import re
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]

# Track the program's allocations and simulate malloc/calloc failures.
ALLOCATOR = r"""
#include <stdio.h>
#include <stdlib.h>
void *__real_malloc(size_t);
void *__real_calloc(size_t, size_t);
void __real_free(void *);
static int calls, live;
static void check_leaks(void) {
    if (live) {
        fprintf(stderr, "Leaked allocations: %d\n", live);
        _Exit(99);
    }
}
static int fail(void) {
    if (calls++ == 0) atexit(check_leaks);
    const char *value = getenv("FAIL_ALLOC");
    return value && calls == atoi(value);
}
void *__wrap_malloc(size_t size) {
    if (fail()) return NULL;
    void *p = __real_malloc(size);
    if (p) live++;
    return p;
}
void *__wrap_calloc(size_t count, size_t size) {
    if (fail()) return NULL;
    void *p = __real_calloc(count, size);
    if (p) live++;
    return p;
}
void __wrap_free(void *p) {
    if (p) live--;
    __real_free(p);
}
"""

HARNESS = r"""
#define main scheduler_main
#include "scheduler.c"
#undef main
int main(void) {
    int n;
    struct context ctx;
    if (scanf("%d%d%d", &n, &ctx.quantum_1, &ctx.quantum_2) != 3) return 2;
    struct process **p = (struct process **)calloc((size_t)n, sizeof(*p));
    if (!p) return 1;
    struct queue unused[3] = {{NULL, NULL}, {NULL, NULL}, {NULL, NULL}};
    int allocated = 0;
    for (int i = 0; i < n; i++) {
        p[i] = (struct process *)malloc(sizeof(*p[i]));
        if (!p[i]) { cleanup(p, unused, allocated); return 1; }
        allocated++;
        if (scanf("%d%d%d", &p[i]->arrival_time, &p[i]->burst_time, &p[i]->queue) != 3) {
            cleanup(p, unused, allocated);
            return 2;
        }
        p[i]->pid = i + 1;
        p[i]->remaining_time = p[i]->burst_time;
        p[i]->first_start = -1;
        p[i]->completion_time = 0;
        p[i]->quantum_left = 0;
    }
    struct mlq_result result;
    int status = simulate_mlq(p, n, ctx, &result);
    if (status == 0) {
        for (struct execution_segment *s = result.head; s; s = s->next)
            printf("S %d %d %lld %lld %d\n", s->pid, s->queue, s->start, s->end, s->preempted_remaining);
        for (int i = 0; i < n; i++)
            printf("M %d %d %d %d\n", p[i]->pid, p[i]->first_start, p[i]->completion_time, p[i]->remaining_time);
        printf("P %lld\n", result.higher_preemptions);
    }
    free_mlq_result(&result);
    cleanup(p, unused, allocated);
    return status == 0 ? 0 : 1;
}
"""


class SchedulerTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp = tempfile.TemporaryDirectory()
        cls.binary = Path(cls.temp.name) / "scheduler"
        allocator = Path(cls.temp.name) / "allocator.c"
        allocator.write_text(ALLOCATOR)
        subprocess.run([
            "gcc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
            "-fsanitize=undefined", "-fno-sanitize-recover=all",
            str(ROOT / "scheduler.c"), str(allocator),
            "-Wl,--wrap=malloc,--wrap=calloc,--wrap=free",
            "-o", str(cls.binary),
        ], check=True)
        harness = Path(cls.temp.name) / "harness.c"
        harness.write_text(HARNESS)
        cls.harness = Path(cls.temp.name) / "harness"
        subprocess.run([
            "gcc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
            "-fsanitize=undefined", "-fno-sanitize-recover=all", "-I", str(ROOT),
            str(harness), str(allocator),
            "-Wl,--wrap=malloc,--wrap=calloc,--wrap=free",
            "-o", str(cls.harness),
        ], check=True)

    @classmethod
    def tearDownClass(cls):
        cls.temp.cleanup()

    def run_scheduler(self, data, fail_alloc=None):
        env = dict(os.environ)
        env.pop("FAIL_ALLOC", None)
        if fail_alloc is not None:
            env["FAIL_ALLOC"] = str(fail_alloc)
        return subprocess.run([str(self.binary)], input=data, text=True,
                              capture_output=True, timeout=5, env=env)

    def assert_rejected(self, data, fail_alloc=None):
        result = self.run_scheduler(data, fail_alloc)
        self.assertEqual(result.returncode, 1, result.stdout + result.stderr)
        self.assertEqual(result.stderr, "")
        self.assertNotIn("PROCESS INPUT AND QUEUE CONFIGURATION", result.stdout)

    def test_sample_keeps_queue_assignments_and_frees_memory(self):
        result = self.run_scheduler((ROOT / "input.txt").read_text())
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stderr, "")
        for line in ["P1 -> Q3 -> FCFS", "P2 -> Q1 -> RR (quantum=2)",
                     "P3 -> Q2 -> RR (quantum=3)", "P4 -> Q3 -> FCFS"]:
            self.assertIn(line, result.stdout)

    def test_invalid_queue_is_rejected_before_indexing(self):
        for queue in [-1, 0, 4, 2147483647]:
            with self.subTest(queue=queue):
                self.assert_rejected(f"1 2 3 0 1 {queue}")

    def test_process_count_must_be_positive(self):
        for count in [0, -1]:
            with self.subTest(count=count):
                self.assert_rejected(f"{count} 2 3")

    def test_bad_or_incomplete_input_is_rejected(self):
        for data in ["", "x", "1", "1 2", "1 x 3", "1 2 3",
                     "1 2 3 0 1", "1 2 3 x 1 1", "1 2 3 0 1 1x",
                     "2 2 3 0 1 1", "2 2 3 0 1 1 0 x 2",
                     "1\x00x 2 3 0 1 1"]:
            with self.subTest(data=data):
                self.assert_rejected(data)

    def test_out_of_range_integers_are_rejected(self):
        for data in ["4294967297 2 3 0 1 1", "1 4294967298 3 0 1 1",
                     "1 2 3 4294967296 1 1", "1 2 3 0 4294967297 1",
                     "1 2 3 0 1 4294967297", "9" * 100 + " 2 3"]:
            with self.subTest(data=data):
                self.assert_rejected(data)

    def test_invalid_times_release_prior_processes(self):
        for row in ["-1 1 1", "0 0 1", "0 -1 1"]:
            with self.subTest(row=row):
                self.assert_rejected("2 2 3 0 1 1 " + row)
        for data in ["1 0 3", "1 2 -1"]:
            with self.subTest(data=data):
                self.assert_rejected(data)

    def test_allocation_failures_exit_cleanly(self):
        # Input allocations happen before the configuration is printed.
        for allocation in range(1, 4):
            with self.subTest(allocation=allocation):
                self.assert_rejected("2 2 3 0 1 1 1 2 2", allocation)

    def simulate(self, data):
        env = dict(os.environ)
        env.pop("FAIL_ALLOC", None)
        result = subprocess.run([str(self.harness)], input=data, text=True,
                                capture_output=True, timeout=5, env=env)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertEqual(result.stderr, "")
        segments, metrics, preemptions = [], {}, None
        for line in result.stdout.splitlines():
            kind, *values = line.split()
            values = list(map(int, values))
            if kind == "S":
                segments.append(tuple(values))
            elif kind == "M":
                pid, start, end, remaining = values
                metrics[pid] = (start, end, remaining)
            elif kind == "P":
                preemptions = values[0]
        return segments, metrics, preemptions

    def test_remote_eight_scenarios_match_timeline_and_process_times(self):
        document = (ROOT / "tests" / "unittest.txt").read_text()
        cases = re.findall(
            r"Input: tests/inputs/(\S+)\n(.*?)(?=\nTC\d+:|\nPERHITUNGAN MANUAL)",
            document, re.S,
        )
        self.assertEqual(len(cases), 8)
        for filename, block in cases:
            with self.subTest(filename=filename):
                segments, metrics, _ = self.simulate(
                    (ROOT / "tests" / "inputs" / filename).read_text()
                )
                chart = re.search(r"Gantt MLQ yang diharapkan:\n\s+([^\n]+)", block)[1]
                expected = [
                    (int(pid[1:]) if pid != "IDLE" else 0, int(start), int(end))
                    for start, end, pid in re.findall(r"(\d+)-(\d+) (P\d+|IDLE)", chart)
                ]
                self.assertEqual([(s[0], s[2], s[3]) for s in segments], expected)
                rows = re.findall(r"^P(\d+)\s+((?:\d+\s+){7}\d+)\s*$", block, re.M)
                self.assertEqual(len(rows), len(metrics))
                for pid, row in rows:
                    at, bt, queue, start, ct, tat, wt, rt = map(int, row.split())
                    self.assertEqual(metrics[int(pid)], (start, ct, 0))
                    self.assertEqual((ct - at, ct - at - bt, start - at), (tat, wt, rt))
                    self.assertTrue(all(s[1] == queue for s in segments if s[0] == int(pid)))

    def test_rr_quantum_resets_after_higher_queue_preemption(self):
        segments, metrics, preemptions = self.simulate("3 1 3 0 6 2 0 1 2 1 1 1")
        self.assertEqual([(s[0], s[2], s[3]) for s in segments],
                         [(1, 0, 1), (3, 1, 2), (1, 2, 5), (2, 5, 6), (1, 6, 8)])
        self.assertEqual(segments[0][4], 5)
        self.assertEqual(preemptions, 1)
        self.assertEqual(metrics[1], (0, 8, 0))

    def test_fcfs_resumes_before_other_q3_processes(self):
        segments, _, preemptions = self.simulate("3 2 3 0 4 3 1 1 3 2 1 1")
        self.assertEqual([(s[0], s[2], s[3]) for s in segments],
                         [(1, 0, 2), (3, 2, 3), (1, 3, 5), (2, 5, 6)])
        self.assertEqual(preemptions, 1)

    def test_rr_requeue_precedes_arrival_at_quantum_boundary(self):
        segments, _, preemptions = self.simulate("2 2 3 0 4 1 2 1 1")
        self.assertEqual([(s[0], s[2], s[3]) for s in segments], [(1, 0, 4), (2, 4, 5)])
        self.assertEqual(preemptions, 0)

    def test_completion_is_not_counted_as_higher_queue_preemption(self):
        _, _, preemptions = self.simulate("2 2 3 0 1 2 1 1 1")
        self.assertEqual(preemptions, 0)

    def test_int_time_boundary_does_not_overflow(self):
        _, metrics, _ = self.simulate("1 2 3 2147483646 1 1")
        self.assertEqual(metrics[1], (2147483646, 2147483647, 0))
        result = self.run_scheduler("1 2 3 2147483647 1 1")
        self.assertEqual(result.returncode, 1, result.stderr)
        self.assertEqual(result.stderr, "")
        self.assertIn("INT_MAX", result.stdout)

    def test_simulation_allocation_failures_release_queues_and_timeline(self):
        data = "4 2 3 0 8 3 1 4 1 2 2 2 3 5 3"
        for allocation in range(1, 100):
            with self.subTest(allocation=allocation):
                result = self.run_scheduler(data, allocation)
                self.assertEqual(result.stderr, "")
                self.assertIn(result.returncode, (0, 1), result.stdout)
                if result.returncode == 0:
                    break
        else:
            self.fail("No successful execution after checking all allocation sites")

    def test_cpp_build_preserves_sample_output(self):
        binary = Path(self.temp.name) / "scheduler-cpp"
        build = subprocess.run([
            "g++", "-std=c++17", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
            str(ROOT / "scheduler.c"), "-o", str(binary),
        ], text=True, capture_output=True, timeout=10)
        self.assertEqual(build.returncode, 0, build.stderr)
        data = (ROOT / "input.txt").read_text()
        result = subprocess.run([str(binary)], input=data, text=True,
                                capture_output=True, timeout=5)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stderr, "")
        self.assertEqual(result.stdout, self.run_scheduler(data).stdout)


if __name__ == "__main__":
    unittest.main()
