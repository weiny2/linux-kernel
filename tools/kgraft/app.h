static inline void __attribute__((always_inline)) in_app_inline (void)
{
  static int local_static_data;
  printf ("in_app_inline: %d\n", local_static_data++);
}

void second_file (void);
