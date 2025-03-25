#define NOB_IMPLEMENTATION
#include "deps/nob.h"
#include <string.h>

#define BIN_FOLDER 			"bin/"
#define SRC_FOLDER   		"src/"
#define DEPS_FOLDER			"deps/"
#define TESTS_FOLDER		"tests/"

int main(int argc, char **argv)
{
  /* Having trouble running this consistently, so I'm disabling it for now.
     Just re-run the init/bootstrap process if you make any changes here */
	/* NOB_GO_REBUILD_URSELF(argc, argv); */
  
  enum targets {
    TARGET_TEST = 1,
    TARGET_DEMO = 2
  };
  
  int targets_included = 0;
  
  if (argc == 1) {
    printf("Usage: build <target>\n\n");
    printf("Builds the specified target.\n\n");
    printf("Arguments:\n");
    printf("\ttest\tBuilds the test target\n");
    printf("\tdemo\tBuilds the demo target (NOT IMPLEMENTED YET)\n");
    printf("\tall\tBuilds both test and demo targets\n");
    return 0;
  } else {
    for (int i = 1; i < argc; ++i) {
      if (!strcmp(argv[i], "all")) {
        targets_included = TARGET_TEST | TARGET_DEMO;
      } else if (!strcmp(argv[i], "test")) {
        targets_included |= TARGET_TEST;
      } else if (!strcmp(argv[i], "demo")) {
        targets_included |= TARGET_DEMO;
      } else {
        printf("Unknown target '%s'!\n", argv[i]);
        return 1;
      }
    }
  }

	if (!nob_mkdir_if_not_exists(BIN_FOLDER)) {
		return 1;
	}

	Nob_Cmd cmd = { 0 };

  if (targets_included & TARGET_TEST) {
    nob_cmd_append(
      &cmd,
      "gcc",
      "-std=gnu99",
      "-Wfatal-errors",
      "-I"SRC_FOLDER,
      "-I"TESTS_FOLDER,
      "-I"DEPS_FOLDER"unity/src/",
      "-I"DEPS_FOLDER"unity/extras/fixture/src/",

      "-DDEBUG",
      "-DUNITY_INCLUDE_PRINT_FORMATTED",
      "-DUNITY_INCLUDE_DOUBLE",

      "-o", BIN_FOLDER"tests",

      DEPS_FOLDER"unity/src/unity.c",
      DEPS_FOLDER"unity/extras/fixture/src/fixture.c",

      SRC_FOLDER"cigcore.c",

      TESTS_FOLDER"main.c",
      TESTS_FOLDER"core/layout.c",
      TESTS_FOLDER"core/state.c",
      TESTS_FOLDER"core/input.c",
      TESTS_FOLDER"core/macros.c",
      TESTS_FOLDER"types.c",
    );

    if (!nob_cmd_run_sync_and_reset(&cmd)) return 1;
  }

	return 0;
}