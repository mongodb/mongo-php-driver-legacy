--TEST--
Test for PHP-306: MongoID::__set_state does not work.
--SKIPIF--
<?php require_once dirname(__FILE__) ."/skipif.inc"; ?>
--FILE--
<?php
$n = new MongoId('4f06e55e44670ab92b000000');
var_export($n);
echo "\n";

$a = MongoId::__set_state(array(
   '$id' => '4f06e55e44670ab92b000000',
));
var_dump($a);
?>
--EXPECT--
MongoId::__set_state(array(
   '$id' => '4f06e55e44670ab92b000000',
))
object(MongoId)#2 (1) {
  ["$id"]=>
  string(24) "4f06e55e44670ab92b000000"
}
