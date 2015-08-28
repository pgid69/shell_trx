#include <ctype.h>
#include <fcntl.h>
#include <limits.h>
#include <linux/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define BLOCK_SIZE 64

static const char prefix_cmd_spawn[] = "spawn ";
static const char suffix_cmd_spawn[] = "\n";
static const char cmd_interact_before[] = "exp_send_user \"**** Shell launched. Entering interactive mode. Type CTRL+C to exit interactive mode and transfer the file ****\\n\"\ninteract \\003 return\n";
static const char cmd_interact_after[] = "exp_send_user \"**** Entering interactive mode. Type CTRL+C to exit interactive mode and exit the shell ****\\n\"\ninteract \\003 return\n";
static const char cmd_msg_trx_successful[] = "exp_send_user \"Transfer successfully ended\\n\"\n";
static const char prefix_cmd_set_timeout[] = "set timeout ";
static const char suffix_cmd_set_timeout[] = "\n";
static const char prefix_cmd_expect_before[] = "expect_before -re {";
static const char suffix_cmd_expect_before[] = "}\n";
static const char cmd_expect_after[] = "expect_after {\n  timeout {\n    exp_send_user \"Timeout. Exiting\\n\"\n    exit\n  }\n  eof {\n    exp_send_user \"EOF. Exiting\\n\"\n    exit\n  }\n}\n";
static const char prefix_cmd_send[] = "exp_send -- {";
static const char suffix_cmd_send[] = "}; exp_send \"\\r\"\n";
static const char prefix_cmd_send_user[] = "exp_send_user -- {";
static const char suffix_cmd_send_user[] = "}; exp_send_user \"\\n\"\n";
static const char cmd_expect[] = "expect\n";
static const char cmd_close[] = "exp_send \"exit\\r\"\nclose\n";
static const char prefix_script_cmd_echo[] = "echo -e -n";
static const char script_cmd_disable_echo[] = "stty -echo";
static const char script_cmd_enable_echo[] = "stty echo";

#define ARRAY_SIZE(tab) (sizeof(tab) / sizeof(tab[0]))

static const char prm_name_shell[] = "shell";
static const char prm_name_in[] = "in";
static const char prm_name_out[] = "out";
static const char prm_name_script[] = "script";
static const char prm_name_prompt[] = "prompt";
static const char prm_name_timeout[] = "timeout";
static const char prm_name_interact_before[] = "interact_before";
static const char prm_name_interact_after[] = "interact_after";
static const char prm_name_echo[] = "echo";

static const char *prm_shell = "telnet";
static const char *prm_input_file_name = NULL;
static const char *prm_output_file_name = "/tmp/fil";
static const char *prm_script_file_name = "/tmp/shell_trx";
static const char *prm_prompt = NULL;
static const char *prm_timeout = "10";
static const char *prm_interact_before = "1";
static const char *prm_interact_after = "0";
static const char *prm_echo = "0";

static void parse_option(const char *option, const char **name, size_t *name_len, const char **value)
{
   const char *tmp;

   while (isspace(*option)) {
      option += 1;
   }
   *name = option;

   tmp = strchr(*name, '=');
   if (NULL != tmp) {
      *value = tmp + 1;
   }
   else {
      *value = NULL;
      tmp = *name;
      tmp += strlen(tmp);
   }
   *name_len = tmp - *name;
}

static inline int value_is_bool(const char *value)
{
   if ((('0' != value[0]) && ('1' != value[0])) || ('\0' != value[1])) {
      return (0);
   }
   else {
      return (1);
   }
}

static inline int value_is_long(const char *value)
{
   char *ptr;
   if ('\0' != *value) {
      long v = strtol(value, &(ptr), 0);
      if ((LONG_MIN != v) && (LONG_MAX != v)) {
         if (NULL != ptr) {
            while (isspace(*ptr)) {
               ptr += 1;
            }
         }
         if ((NULL == ptr) || ('\0' == *ptr)) {
            return (1);
         }
      }
   }
   return (0);
}

static int read_parameters(int argc, char **argv)
{
   int ret = -1;

   if (argc > 1) {
      int i;
      const char *name;
      size_t name_len;
      const char *value;
      for (i = 1; (i < argc); i += 1) {
         parse_option(argv[i], &(name), &(name_len), &(value));
         if ((NULL == value) || ('\0' == value[0])) {
            fprintf(stderr, "Arg '%s' is ignored because it has no value\n", argv[i]);
            continue;
         }
         if (((ARRAY_SIZE(prm_name_shell) - 1) == name_len) && (0 == strncasecmp(name, prm_name_shell, name_len))) {
            prm_shell = value;
            continue;
         }
         if (((ARRAY_SIZE(prm_name_in) - 1) == name_len) && (0 == strncasecmp(name, prm_name_in, name_len))) {
            prm_input_file_name = value;
            continue;
         }
         if (((ARRAY_SIZE(prm_name_out) - 1) == name_len) && (0 == strncasecmp(name, prm_name_out, name_len))) {
            prm_output_file_name = value;
            continue;
         }
         if (((ARRAY_SIZE(prm_name_script) - 1) == name_len) && (0 == strncasecmp(name, prm_name_script, name_len))) {
            prm_script_file_name = value;
            continue;
         }
         if (((ARRAY_SIZE(prm_name_prompt) - 1) == name_len) && (0 == strncasecmp(name, prm_name_prompt, name_len))) {
            prm_prompt = value;
            continue;
         }
         if (((ARRAY_SIZE(prm_name_timeout) - 1) == name_len) && (0 == strncasecmp(name, prm_name_timeout, name_len))) {
            prm_timeout = value;
            continue;
         }
         if (((ARRAY_SIZE(prm_name_interact_before) - 1) == name_len) && (0 == strncasecmp(name, prm_name_interact_before, name_len))) {
            prm_interact_before = value;
            continue;
         }
         if (((ARRAY_SIZE(prm_name_interact_after) - 1) == name_len) && (0 == strncasecmp(name, prm_name_interact_after, name_len))) {
            prm_interact_after = value;
            continue;
         }
         if (((ARRAY_SIZE(prm_name_echo) - 1) == name_len) && (0 == strncasecmp(name, prm_name_echo, name_len))) {
            prm_echo = value;
            continue;
         }
         fprintf(stderr, "Unknown parameter '%s'\n", argv[i]);
      }
   }

   do { /* Empty loop */
      if ((NULL == prm_shell) || ('\0' == prm_shell)) {
         fprintf(stderr, "No shell specified\n");
         break;
      }
      if ((NULL == prm_input_file_name) || ('\0' == prm_input_file_name)) {
         fprintf(stderr, "No input file name specified\n");
         break;
      }
      if ((NULL == prm_output_file_name) || ('\0' == prm_output_file_name)) {
         fprintf(stderr, "No output file name specified\n");
         break;
      }
      if ((NULL == prm_script_file_name) || ('\0' == prm_script_file_name)) {
         fprintf(stderr, "No script file name specified\n");
         break;
      }
      if ((NULL == prm_prompt) || ('\0' == prm_prompt)) {
         fprintf(stderr, "No prompt specified\n");
         break;
      }
      if ((NULL == prm_timeout) || (!value_is_long(prm_timeout))) {
         fprintf(stderr, "Value of parameter timeout must be an integer\n");
         break;
      }
      if ((NULL == prm_interact_before) || (!value_is_bool(prm_interact_before))) {
         fprintf(stderr, "Value of parameter interact_before can only be '0' or '1'\n");
         break;
      }
      if ((NULL == prm_interact_after) || (!value_is_bool(prm_interact_after))) {
         fprintf(stderr, "Value of parameter interact_after can only be '0' or '1'\n");
         break;
      }
      if ((NULL == prm_echo) || (!value_is_bool(prm_echo))) {
         fprintf(stderr, "Value of parameter echo can only be '0' or '1'\n");
         break;
      }
      ret = 0;
   } while (0);


   return (ret);
}

static int send_command(int fout, const char *cmd, size_t cmd_len)
{
   int ret = -1;

   do { /* Empty loop */
      ssize_t count;

      count = sizeof(prefix_cmd_send) - sizeof(char);
      if (write(fout, prefix_cmd_send, count) != count) {
         perror("Error writing to output file");
         break;
      }
      count = cmd_len * sizeof(char);
      if (write(fout, cmd, count) != count) {
         perror("Error writing to output file");
         break;
      }
      count = sizeof(suffix_cmd_send) - sizeof(char);
      if (write(fout, suffix_cmd_send, count) != count) {
         perror("Error writing to output file");
         break;
      }
      count = sizeof(prefix_cmd_send_user) - sizeof(char);
      if (write(fout, prefix_cmd_send_user, count) != count) {
         perror("Error writing to output file");
         break;
      }
      if ('1' == *prm_echo) {
         count = cmd_len * sizeof(char);
         if (write(fout, cmd, count) != count) {
            perror("Error writing to output file");
            break;
         }
      }
      count = sizeof(suffix_cmd_send_user) - sizeof(char);
      if (write(fout, suffix_cmd_send_user, count) != count) {
         perror("Error writing to output file");
         break;
      }
      count = sizeof(cmd_expect) - sizeof(char);
      if (write(fout, cmd_expect, count) != count) {
         perror("Error writing to output file");
         break;
      }
      ret = 0;
   } while (0);

   return (ret);
}

int main(int argc, char **argv)
{
   int ret = -1;
   int fin = -1;
   int fout = -1;

   do {
      char buffer_out[BLOCK_SIZE * 5 + 512];
      ssize_t count;
      const char *to;

      if (read_parameters(argc, argv)) {
         break;
      }
      fin = open(prm_input_file_name, O_RDONLY);
      if (fin < 0) {
         fprintf(stderr, "Failed to open input file '%s'\n", argv[1]);
         break;
      }
      fout = open(prm_script_file_name, O_CREAT | O_TRUNC | O_WRONLY, S_IRWXU);
      if (fout < 0) {
         fprintf(stderr, "Failed to open output file '%s'\n", argv[2]);
         break;
      }

      count = snprintf(buffer_out, ARRAY_SIZE(buffer_out), "%s%s%s",
         prefix_cmd_spawn, prm_shell, suffix_cmd_spawn);
      if (write(fout, buffer_out, count) != count) {
         perror("Error writing to output file");
         break;
      }
      if ('1' == *prm_interact_before) {
         count = sizeof(cmd_interact_before) - sizeof(char);
         if (write(fout, cmd_interact_before, count) != count) {
            perror("Error writing to output file");
            break;
         }
      }
      count = snprintf(buffer_out, ARRAY_SIZE(buffer_out), "%s%s%s",
         prefix_cmd_set_timeout, prm_timeout, suffix_cmd_set_timeout);
      if (write(fout, buffer_out, count) != count) {
         perror("Error writing to output file");
         break;
      }
      count = snprintf(buffer_out, ARRAY_SIZE(buffer_out), "%s%s%s",
         prefix_cmd_expect_before, prm_prompt, suffix_cmd_expect_before);
      if (write(fout, buffer_out, count) != count) {
         perror("Error writing to output file");
         break;
      }
      count = sizeof(cmd_expect_after) - sizeof(char);
      if (write(fout, cmd_expect_after, count) != count) {
         perror("Error writing to output file");
         break;
      }

      if (send_command(fout, script_cmd_disable_echo, ARRAY_SIZE(script_cmd_disable_echo) - 1)) {
         ret = -1;
         break;
      }
      to = ">";
      for (;;) {
         __u8 buffer_in[BLOCK_SIZE];
         ssize_t i;
         size_t j;

         count = read(fin, buffer_in, sizeof(buffer_in));
         if (count < 0) {
            perror("Error reading from input file");
            break;
         }
         if (0 == count) {
            ret = 0;
            break;
         }
         j = snprintf(buffer_out, ARRAY_SIZE(buffer_out), "%s '", prefix_script_cmd_echo);
         for (i = 0; (i < count); i += 1) {
            __u8 v = buffer_in[i];
            if ((v >= 32) && (v < 127)
                /* Char to protect due to echo syntax */
                && (v != '\'') && (v != '\\')
                /* Char to protect for send command */
                && (v != '{') && (v != '}')) {
               buffer_out[j] = (char)(v);
               j += 1;
            }
            else {
               sprintf(&(buffer_out[j]), "\\x%02x", (unsigned int)(v));
               j += 4;
            }
         }
         j += snprintf(&(buffer_out[j]), ARRAY_SIZE(buffer_out) - j, "' %s '%s'", to, prm_output_file_name);
         if (send_command(fout, buffer_out, j)) {
            break;
         }
         to = ">>";
      }
      if (ret) {
         break;
      }
      ret = -1;
      if (send_command(fout, script_cmd_enable_echo, ARRAY_SIZE(script_cmd_enable_echo) - 1)) {
         ret = -1;
         break;
      }
      count = sizeof(cmd_msg_trx_successful) - sizeof(char);
      if (write(fout, cmd_msg_trx_successful, count) != count) {
         perror("Error writing to output file");
         break;
      }
      if ('1' == *prm_interact_after) {
         count = sizeof(cmd_interact_after) - sizeof(char);
         if (write(fout, cmd_interact_after, count) != count) {
            perror("Error writing to output file");
            break;
         }
      }
      count = sizeof(cmd_close) - sizeof(char);
      if (write(fout, cmd_close, count) != count) {
         perror("Error writing to output file");
         break;
      }
      ret = 0;
   } while (0);

   if (fin >= 0) {
      close(fin);
      fin = -1;
   }
   if (fout >= 0) {
      close(fout);
      fout = -1;
   }

   return (ret);
}
