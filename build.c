#define NOB_IMPLEMENTATION
#include "deps/nob.h"

#define OUT_FOLDER 			"out/"
#define INCLUDE_FOLDER	"include/"
#define SRC_FOLDER   		"src/"
#define DEPS_FOLDER			"deps/"
#define TESTS_FOLDER		"tests/"

int main(int argc, char **argv)
{
		if (!nob_mkdir_if_not_exists(OUT_FOLDER)) {
			return 1;
		}
		
    Nob_Cmd cmd = { 0 };
		
    nob_cmd_append(
			&cmd,
			"gcc",
			"-Wfatal-errors",
			// "-Wall",
			"-I"INCLUDE_FOLDER,
			"-I"TESTS_FOLDER,
			"-I"DEPS_FOLDER,
			"-I"DEPS_FOLDER"unity/src/",
			"-I"DEPS_FOLDER"unity/extras/fixture/src/",
			
			"-DDEBUG",
			"-DUNITY_INCLUDE_PRINT_FORMATTED",
			
			"-o", OUT_FOLDER"tests",
			
			DEPS_FOLDER"unity/src/unity.c",
			DEPS_FOLDER"unity/extras/fixture/src/fixture.c",
			SRC_FOLDER"imgui.c",
			TESTS_FOLDER"main.c",
			TESTS_FOLDER"core/layout.c",
			TESTS_FOLDER"core/input.c",
			TESTS_FOLDER"types/frame.c",
		);
		
		if (!nob_cmd_run_sync_and_reset(&cmd)) return 1;
		
		/*printf("second step\n");
		
		nob_cmd_append(&cmd, "gcc", "-Wfatal-errors", "-Wall",  "-o", OUT_FOLDER"tests", TESTS_FOLDER"main.c");
		
		if (!nob_cmd_run_sync_and_reset(&cmd)) return 1;*/
		
    return 0;
}