--TEST--
Mongo::selectDB
--FILE--
<?php
$m = new Mongo();
$db = $m->selectDB('test');
echo is_object($db) ? '1' :'0' ;
?>
--EXPECT--
1
