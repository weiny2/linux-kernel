#include <stdio.h>
#include "app.h"

static int local_data;
int global_data;

static void __attribute__((noinline)) in_app (void)
{
  printf ("in_app\n");
  in_app_inline ();
  local_data = 42;
}

static inline void __attribute__((always_inline)) in_app_inline_twice (void)
{
  global_data++;
  in_app_inline ();
}

void in_app_global (void)
{
  printf ("in_app_global\n");
  in_app();
  in_app_inline_twice ();
  global_data = 43;
}

int main ()
{
  in_app_global();
  second_file ();
  printf ("local_data = %d\n", local_data);
  printf ("global_data = %d\n", global_data);
  return 0;
}
