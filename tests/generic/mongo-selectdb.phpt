--TEST--
Mongo::selectDB
--FILE--
<?php
require __DIR__ . "/../utils.inc";
$m = mongo();
$db = $m->selectDB('test');
echo is_object($db) ? '1' :'0' ;
?>
--EXPECT--
1
