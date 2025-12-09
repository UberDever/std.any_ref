// clang-format off
/// NOB_BUILD_LINUX: cc -std=c99 -I.. -o nob nob.c internal/headeronly/nob.c internal/headeronly/flag.c
// clang-format on

#include "std.any_ref/third_party/nob.h/nob.h"
#include "std.any_ref/third_party/flag.h/flag.h"

#ifdef _WIN32
#define FS_SEP "\\"
#else
#define FS_SEP "/"
#endif

static Nob_String_View nob_current_build_key(void) {
#if defined(_WIN32)
  return nob_sv_from_cstr("NOB_BUILD_WINDOWS");
#elif defined(__APPLE__)
  return nob_sv_from_cstr("NOB_BUILD_MACOS");
#else
  return nob_sv_from_cstr("NOB_BUILD_LINUX");
#endif
}

static void nob_collect_c_sources_from_command(const char *command,
                                               Nob_File_Paths *inputs) {
  Nob_String_View sv = nob_sv_from_cstr(command);

  while (sv.count > 0) {
    while (sv.count > 0 && isspace((unsigned char)sv.data[0])) {
      nob_sv_chop_left(&sv, 1);
    }
    if (sv.count == 0) { break; }

    size_t i = 0;
    while (i < sv.count && !isspace((unsigned char)sv.data[i])) {
      i += 1;
    }

    Nob_String_View token = nob_sv_from_parts(sv.data, i);
    nob_sv_chop_left(&sv, i);

    if (token.count >= 2 && (token.data[0] == '"' || token.data[0] == '\'') &&
        token.data[0] == token.data[token.count - 1]) {
      token.data += 1;
      token.count -= 2;
    }

    if (token.count > 0 && nob_sv_end_with(token, ".c")) {
      nob_da_append(inputs, nob_temp_sv_to_cstr(token));
    }
  }
}

static void nob_rebuild_from_directives(int argc, char **argv,
                                        const char *source_path) {
  const char *binary_path = nob_shift(argv, argc);
#ifdef _WIN32
  if (!nob_sv_end_with(nob_sv_from_cstr(binary_path), ".exe")) {
    binary_path = nob_temp_sprintf("%s.exe", binary_path);
  }
#endif

  Nob_File_Paths inputs = {0};
  nob_da_append(&inputs, source_path);

  Nob_String_Builder source = {0};
  if (!nob_read_entire_file(source_path, &source)) {
    nob_log(NOB_ERROR, "failed to read %s", source_path);
    exit(1);
  }

  Nob_String_View cursor = nob_sv_from_parts(source.items, source.count);
  Nob_String_View prefix = nob_sv_from_cstr("/// NOB_");
  Nob_String_View platform_key = nob_current_build_key();

  const char *command = NULL;

  while (cursor.count > 0) {
    Nob_String_View raw_line = nob_sv_chop_by_delim(&cursor, '\n');
    Nob_String_View line = nob_sv_trim(raw_line);
    if (nob_sv_starts_with(line, nob_sv_from_cstr("#include"))) { break; }
    if (!nob_sv_starts_with(line, prefix)) { continue; }

    Nob_String_View rest = line;
    nob_sv_chop_left(&rest, 3); // drop leading slashes
    rest = nob_sv_trim_left(rest);

    Nob_String_View name = nob_sv_chop_by_delim(&rest, ':');
    name = nob_sv_trim_right(name);
    rest = nob_sv_trim_left(rest);
    if (nob_sv_eq(name, platform_key)) {
      command = nob_temp_sv_to_cstr(rest);
    } else {
      nob_log(NOB_ERROR, "unknown build directive key (%.*s)", (int)name.count,
              name.data);
      exit(1);
    }
  }

  nob_sb_free(source);

  if (command == NULL) {
    nob_log(NOB_ERROR, "no build directive for platform (%.*s) found in %s",
            (int)platform_key.count, platform_key.data, source_path);
    exit(1);
  }

  nob_collect_c_sources_from_command(command, &inputs);

  int rebuild_is_needed =
      nob_needs_rebuild(binary_path, inputs.items, inputs.count);
  NOB_FREE(inputs.items);
  if (rebuild_is_needed < 0) { exit(1); }
  if (!rebuild_is_needed) { return; }

  Nob_Cmd cmd = {0};
  const char *old_binary_path = nob_temp_sprintf("%s.old", binary_path);

  if (!nob_rename(binary_path, old_binary_path)) { exit(1); }

#if defined(_WIN32)
  nob_cmd_append(&cmd, "cmd", "/C", command);
#else
  nob_cmd_append(&cmd, "sh", "-c", command);
#endif

  if (!nob_cmd_run(&cmd)) {
    nob_rename(old_binary_path, binary_path);
    exit(1);
  }
  if (unlink(old_binary_path) < 0) {
    nob_log(NOB_WARNING, "could not delete temporary file %s: %s",
            old_binary_path, strerror(errno));
    exit(1);
  }

  nob_cmd_append(&cmd, binary_path);
  nob_da_append_many(&cmd, argv, argc);
  if (!nob_cmd_run(&cmd)) { exit(1); }

  exit(0);
}

#define CC "cc"
#define CFLAGS                                                                 \
  "-std=c99", "-fsanitize=address", "-O0", "-Wall", "-Wextra", "-Werror"

static int command_build(const char *target) {
  // TODO: here we use target
  Nob_Cmd cmd = {0};
  nob_cmd_append(&cmd, CC);
  nob_cmd_append(&cmd, CFLAGS);
  nob_cmd_append(&cmd, "-g");
  nob_cmd_append(&cmd, "-I..");
  nob_cmd_append(&cmd, "-I./public/std.any_ref");
  nob_cmd_append(&cmd, "-I./third_party/nob.h");
  nob_cmd_append(&cmd, "internal/headeronly/nob.c");
  nob_cmd_append(&cmd, "public/std.any_ref/any_ref.c");
  nob_cmd_append(&cmd, "public/std.any_ref/any_ref_test.c", "-o",
                 "build/any_ref_test");
  if (!nob_cmd_run(&cmd)) { return 1; }
  return 0;
}

static int command_test() {
  Nob_Cmd cmd = {0};
  nob_cmd_append(&cmd, "build/any_ref_test");
  if (!nob_cmd_run(&cmd)) { return 1; }
  return 0;
}

static void usage(const char *program, FILE *stream) {
  fprintf(stream, "Usage: ./%s [OPTIONS] [--] [ARGS]\n", program);
  fprintf(stream, "OPTIONS:\n");
  flag_print_options(stream);
}

// TODO: this must be properly integrated withing gob build framework
int main(int argc, char **argv) {
  nob_rebuild_from_directives(argc, argv, __FILE__);

  const char *program = nob_shift(argv, argc);
  if (argc == 0) {
    nob_log(NOB_ERROR, "no command specified");
    return 1;
  }

  const char *command = nob_shift(argv, argc);

  char *const *flag_target = NULL;

  if (argc > 0) {
    flag_target =
        flag_str("target", NULL, "The target we need to execute command for");

    if (!flag_parse(argc, argv)) {
      usage(program, stderr);
      flag_print_error(stderr);
      exit(1);
    }
  }

  // TODO: this should be done within gob build framework
  // should also add help registry and flag separation for different commands
  // parse flags separately for each command???
  if (0 == strcmp(command, "build")) {
    nob_mkdir_if_not_exists("build");
    const char *target = (flag_target != NULL) ? *flag_target : "all";
    if (command_build(target) != 0) { return 1; }

  } else if (0 == strcmp(command, "test")) {
    nob_mkdir_if_not_exists("build");
    const char *target = (flag_target != NULL) ? *flag_target : "tests";
    if (command_build(target) != 0) { return 1; }
    if (command_test() != 0) { return 1; }

  } else if (0 == strcmp(command, "clean")) {
    // TODO: implement proper clean command
    NOB_TODO("implement clean command");
  }

  else {
    nob_log(NOB_ERROR, "unknown command: %s", command);
    return 1;
  }

  return 0;
}
