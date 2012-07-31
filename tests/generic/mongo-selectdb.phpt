--TEST--
Mongo::selectDB
--SKIPIF--
<?php require_once dirname(__FILE__) ."/skipif.inc"; ?>
--FILE--
<?php
require_once dirname(__FILE__) . "/../utils.inc";
$m = mongo();
$db = $m->selectDB('test');
echo is_object($db) ? '1' :'0' ;
?>
--EXPECT--
1
