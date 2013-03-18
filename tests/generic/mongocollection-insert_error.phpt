--TEST--
MongoCollection::insert()
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
<?php die("skip unfixed bug"); ?>
--XFAIL--
Invalid options for ->insert() do not throw exception
--DESCRIPTION--
Test for giving improper arguments to the function
--FILE--
<?php
require_once "tests/utils/server.inc";
$m = mongo_standalone();
$c = $m->phpunit->col;
$c->drop();

// insert requires an array or object
$c->insert(); 
var_dump($c->findOne());
$c->drop();
$c->insert('hi'); 
var_dump($c->findOne());
$c->drop();
$c->insert(10); 
var_dump($c->findOne());
$c->drop();
$c->insert(false); 
var_dump($c->findOne());
$c->drop();

// will insert accept a numeric array?
$c->insert(array('foo','bar','baz'));
var_dump($c->findOne());
$c->drop();

// this should be fine
$obj = new StdClass();
$obj->hello = 'Hello, World!';
$c->insert($obj);
var_dump($c->findOne());
$c->drop();

// 2nd parameter should be array of options
$c->insert(array('yo'=>'ho'), true); 
var_dump($c->findOne());
$c->drop();

try
{
	$c->insert(array('yo2'=>'ho2'), array('invalid_option')); // I expect an exception here, but I don't get one?
	var_dump($c->findOne());
}
catch (Exception $e)
{
	echo $e->getMessage();
}
?>
--EXPECTF--
Warning: MongoCollection::insert() expects at least 1 parameter, 0 given in %smongocollection-insert_error.php on line %d
NULL

Warning: MongoCollection::insert() expects parameter 1 to be an array or object in %smongocollection-insert_error.php on line %d
NULL

Warning: MongoCollection::insert() expects parameter 1 to be an array or object in %smongocollection-insert_error.php on line %d
NULL

Warning: MongoCollection::insert() expects parameter 1 to be an array or object in %smongocollection-insert_error.php on line %d
NULL
array(4) {
  ["_id"]=>
  object(MongoId)#%d (1) {
    ["$id"]=>
    string(24) "%s"
  }
  [0]=>
  string(3) "foo"
  [1]=>
  string(3) "bar"
  [2]=>
  string(3) "baz"
}
array(2) {
  ["_id"]=>
  object(MongoId)#%d (1) {
    ["$id"]=>
    string(24) "%s"
  }
  ["hello"]=>
  string(13) "Hello, World!"
}
array(2) {
  ["_id"]=>
  object(MongoId)#%d (1) {
    ["$id"]=>
    string(24) "%s"
  }
  ["yo"]=>
  string(2) "ho"
}
Some Exception Message is expected
