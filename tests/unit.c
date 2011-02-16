
#include <stdio.h>
#include <php.h>
#include <sapi/embed/php_embed.h>

#include "unit.h"

int main() {
  printf("Running tests...\n");

  PHP_EMBED_START_BLOCK(0, 0);

  test_mongo();
  
  PHP_EMBED_END_BLOCK();
  
  printf("Done.\n");  
  return 0;
}

int run_test(int (*t)(void)) {
  t();
  printf(".");
}
