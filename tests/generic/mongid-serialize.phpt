--TEST--
MongoID: serialize() and unserialize()
--FILE--
<?php
$id = new MongoId;
$s = serialize($id);
$newid = unserialize($s);
var_dump($newid == $id);
?>
--EXPECTF--
bool(true)
