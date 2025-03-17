#include "unity.h"
#include "fixture.h"
#include "types/frame.h"
#include <stdbool.h>
#include <stdio.h>

static void run_all_tests(void) {
  RUN_TEST_GROUP(types_frame);
  RUN_TEST_GROUP(layout);
}

int main(int argc, const char *argv[]) {
  return UnityMain(argc, argv, run_all_tests);
}