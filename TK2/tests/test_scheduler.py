import os
from pathlib import Path
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
        for allocation in range(1, 6):
            with self.subTest(allocation=allocation):
                self.assert_rejected("2 2 3 0 1 1 1 2 2", allocation)

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
