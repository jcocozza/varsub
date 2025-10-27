#include <string.h>
// flags
char *var_sep = "";
char *assignment_op = "";
int batch = 0;

int main(int argc, char *argv[]) {

  // cli stuff
  for (int i = 1; i < argc; i++) {
    // handle flags
    if (argv[i][0] == '-' && strlen(argv[i]) > 0) {
      if (!strcmp(argv[i], "--help")) {
        return 0;
      }
    }
  }
}
