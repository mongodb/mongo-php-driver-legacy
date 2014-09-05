--TEST--
MongoDate works with var_export()
--SKIPIF--
<?php require_once dirname(__FILE__) . "/skipif.inc"; ?>
--FILE--
<?php
$date = new MongoDate(12345, 67890);
var_export($date);
echo "\n";

$exported = MongoDate::__set_state(array(
   'sec' => '12345',
   'usec' => '67000',
));
var_dump($exported);
?>
--EXPECT--
MongoDate::__set_state(array(
   'sec' => 12345,
   'usec' => 67000,
))
object(MongoDate)#2 (2) {
  ["sec"]=>
  int(12345)
  ["usec"]=>
  int(67000)
}
