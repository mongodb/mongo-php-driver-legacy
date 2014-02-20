--TEST--
Test for PHP-676: Collection level write concern overwritten by deprecated "safe" option
--SKIPIF--
<?php require_once "tests/utils/replicaset.inc" ?>
--INI--
error_reporting=0
--FILE--
<?php
require_once "tests/utils/server.inc";
$m = new_mongo();

$oSemafor = $m->selectDb(dbname())->semafor;
$oSemafor->drop();
$oSemafor->w = 42;
$oSemafor->wtimeout = 3;

try{
        $time = microtime(true);
        $x = $oSemafor->insert(array('createts' => microtime(true)), array('safe' => true));
        var_dump($x);
} catch(MongoCursorException $e){
    var_dump($e->getMessage(), $e->getCode());
}
?>
--EXPECTF--
string(%d) "%s:%d:%stime%S"
int(%d)
