#include <stdio.h>
#include <php.h>

#include "test_mongo.h"
#include "php_mongo.h"

int test_mongo() {
  printf("running Mongo tests: ");
  run_test(test_php_mongo_free_cursor_node);
  printf("\n");
}

int test_php_mongo_free_cursor_node() {
  cursor_node *node1, *node2, *node3;
  list_entry *le;

  // alloc
  node1 = (cursor_node*)pemalloc(sizeof(cursor_node), 1);
  node2 = (cursor_node*)pemalloc(sizeof(cursor_node), 1);
  node3 = (cursor_node*)pemalloc(sizeof(cursor_node), 1);
  le = (list_entry*)emalloc(sizeof(list_entry));
  le->ptr = node1;
  
  // [node1][<->][NODE2][<->][node3]
  node1->prev = node3->next = 0;
  node1->next = node3->prev = node2;
  node2->prev = node1;
  node2->next = node3;

  php_mongo_free_cursor_node(node2, le);
  assert(le->ptr == node1);
  assert(node1->next == node3);
  assert(node3->prev == node1);
  assert(node1->prev == 0);
  assert(node3->next == 0);

  // [node1][<->][NODE2]
  node2 = node3;

  php_mongo_free_cursor_node(node2, le);

  assert(le->ptr == node1);
  assert(node1->next == 0);
  assert(node1->prev == 0);
    
  // [NODE1]
  php_mongo_free_cursor_node(node1, le);
  assert(le->ptr == 0);

  // realloc
  node1 = (cursor_node*)pemalloc(sizeof(cursor_node), 1);
  node2 = (cursor_node*)pemalloc(sizeof(cursor_node), 1);
  node1->prev = 0;
  node1->next = node2;
  node2->prev = node1;
  node2->next = 0;
  le->ptr = node1;
  
  // [NODE1][<->][node2]  
  php_mongo_free_cursor_node(node1, le);
  assert(le->ptr == node2);
  assert(node2->prev == 0);
  assert(node2->next == 0);  

  // [NODE2] (repeat of [NODE1], above)
  php_mongo_free_cursor_node(node2, le);
  assert(le->ptr == 0);

  efree(le);
}
