#include "kernel/types.h"
#include "user/user.h"

#pragma GCC optimize("O0")
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"

int global_data = 123456;

void print_separator() {
  printf("\n==================================================\n");
}

void print_test_header(const char* title) {
  printf("\n==================== %s ====================\n", title);
}

void test_global_variable() {
  int temp;
  print_test_header("GLOBAL VARIABLE");
  
  print_test_header("Initial state");
  ppages(0, 0, 0);
  
  print_test_header("After reading");
  temp = global_data;
  ppages(&global_data, sizeof(global_data), 1);
  
  print_test_header("After modifying");
  global_data = 654321;
  ppages(&global_data, sizeof(global_data), 2);
  
  print_test_header("Final state");
  ppages(&global_data, sizeof(global_data), 3);
}

void test_stack_variable() {
  int stack_var = 123456;
  int temp;
  print_test_header("STACK VARIABLE");
  
  print_test_header("Initial state");
  ppages(0, 0, 0);
  
  print_test_header("After reading");
  temp = stack_var;
  ppages(&stack_var, sizeof(stack_var), 1);
  
  print_test_header("After modifying");
  stack_var = 654321;
  ppages(&stack_var, sizeof(stack_var), 2);
  
  print_test_header("Final state");
  ppages(&stack_var, sizeof(stack_var), 3);
}

void test_stack_array() {
  print_test_header("STACK ARRAY");
  int temp;
  int array_size = 200 * sizeof(int);
  
  print_test_header("Initial state");
  ppages(0, 0, 0);
  
  int stack_array[200];
  print_test_header("After allocation");
  ppages(0, 0, 0);
  
  print_test_header("After clearing flags");
  mppages(0, 0, 3);
  ppages(0, 0, 0);
  
  print_test_header("After reading");
  temp = stack_array[50];
  temp = stack_array[99];
  ppages(stack_array, array_size, 1);
  
  print_test_header("After writing");
  stack_array[50] = 999;
  stack_array[99] = 888;
  ppages(stack_array, array_size, 2);
  
  print_test_header("Final state");
  ppages(stack_array, array_size, 3);
}

void test_heap_array() {
  print_test_header("HEAP ARRAY");
  int temp;
  int array_size = 12000 * sizeof(int);
  
  print_test_header("Initial state");
  ppages(0, 0, 0);
  
  int* heap_array = malloc(array_size);
  print_test_header("After allocation");
  ppages(0, 0, 0);
  
  print_test_header("After clearing flags");
  mppages(0, 0, 3);
  ppages(0, 0, 0);
  
  print_test_header("After reading");
  temp = heap_array[0];   
  temp = heap_array[3000];
  temp = heap_array[6000];
  temp = heap_array[9000];
  ppages(heap_array, array_size, 1);
  
  print_test_header("After writing");
  heap_array[0] = 111;
  heap_array[3000] = 222;
  heap_array[6000] = 333;
  heap_array[9000] = 444;
  ppages(heap_array, array_size, 2);
  
  print_test_header("Final state");
  ppages(heap_array, array_size, 3);
  
  print_test_header("After freeing memory");
  free(heap_array);
  ppages(0, 0, 0);
}

int main() {
  test_global_variable();
  print_separator();
  
  test_stack_variable();
  print_separator();
  
  test_stack_array();
  print_separator();
  
  test_heap_array();
  print_separator();
  
  exit(0);
}