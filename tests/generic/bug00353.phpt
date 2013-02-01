--TEST--
Test for PHP-353: Iterating over a MongoCursor without _id field should not create an empty string.
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php
require_once "tests/utils/server.inc";
$d = mongo_standalone();
$c = $d->phpunit->collection;
$c->drop();

$c->insert( array( '_id' => 'test1', 'value' => 'ONE' ) );
$c->insert( array( '_id' => 'test2', 'value' => 'TWO' ) );
$c->insert( array( '_id' => 'test3', 'value' => 'THREE' ) );

foreach ( $c->find( array(), array( '_id' => 0 ) ) as $key => $value )
{
	var_dump( $key, $value );
}

var_dump( iterator_to_array( $c->find( array(), array( '_id' => 0 ) ) ) );
?>
--EXPECT--
int(0)
array(1) {
  ["value"]=>
  string(3) "ONE"
}
int(1)
array(1) {
  ["value"]=>
  string(3) "TWO"
}
int(2)
array(1) {
  ["value"]=>
  string(5) "THREE"
}
array(3) {
  [0]=>
  array(1) {
    ["value"]=>
    string(3) "ONE"
  }
  [1]=>
  array(1) {
    ["value"]=>
    string(3) "TWO"
  }
  [2]=>
  array(1) {
    ["value"]=>
    string(5) "THREE"
  }
}
