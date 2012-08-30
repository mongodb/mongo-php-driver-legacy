--TEST--
Test for bug PHP-473: Sort by field "0" (zero char, ASCII 0x30) does not work
--SKIPIF--
<?php require_once dirname(__FILE__) . "/skipif.inc" ?>
--FILE--
<?php
require_once dirname(__FILE__) . "/../utils.inc";

$m = mongo();

$db = $m->selectDB(dbname());
$coll = $db->test_sort;
$coll->drop();
// insert test data
for ($i=0; $i<10; $i++) {
    $coll->insert(array("foo" =>array('0' => 'a' + $i, '1' => 'b' + $i, '2' => 'c' + $i)), array('safe' => true));
}

// ask user on which 'column' to sort
$column = "0";
$sort = -1;

// query and sort
$cursor = $coll->find()->sort(array($column => $sort));
var_dump(count(iterator_to_array($cursor)));

// the line below is a workaround for 1.2.10 or earlier, it casts the array to an object.
$cursor = $coll->find()->sort((object) array($column => $sort));
var_dump(count(iterator_to_array($cursor)));

?>
--EXPECTF--
int(10)
int(10)
