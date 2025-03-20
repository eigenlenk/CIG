#include "unity.h"
#include "fixture.h"
#include <stdbool.h>
#include <stdio.h>

static void run_all_tests(void) {
  RUN_TEST_GROUP(types);
  RUN_TEST_GROUP(core_layout);
  RUN_TEST_GROUP(core_input);
}

int main(int argc, const char *argv[]) {
  return UnityMain(argc, argv, run_all_tests);
}