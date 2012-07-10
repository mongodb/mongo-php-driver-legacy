--TEST--
Test for bug #402: MongoCollection::validate(true) doesn't set the correct scan-all flag.
--SKIPIF--
<?php require __DIR__ ."/skipif.inc"; ?>
--FILE--
<?php
require __DIR__ ."/../utils.inc";
$m = mongo();
$c = $m->phpunit->col;
$c->insert( array( 'test' => 'foo' ) );

$r = $c->validate();
var_dump( array_key_exists( 'warning', $r ) );
var_dump( $r['warning'] );

$r = $c->validate( true );
var_dump( array_key_exists( 'warning', $r ) );

$c->drop();
$r = $c->validate();
var_dump( $r );
?>
--EXPECT--
bool(true)
string(79) "Some checks omitted for speed. use {full:true} option to do more thorough scan."
bool(false)
array(2) {
  ["errmsg"]=>
  string(12) "ns not found"
  ["ok"]=>
  float(0)
}
