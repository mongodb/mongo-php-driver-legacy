--TEST--
Test for PHP-676: Collection level write concern overwritten by deprecated "safe" option
--SKIPIF--
<?php require_once dirname(__FILE__) . "/skipif.inc";?>
--FILE--
<?php
require_once dirname(__FILE__) . "/../utils.inc";
$m = new_mongo();

$oSemafor = $m->selectDb(dbname())->semafor;
$oSemafor->drop();
$oSemafor->w = 42;
$oSemafor->wtimeout = 3000;

try{
	$time = microtime(true);
	$x = $oSemafor->insert(array('createts' => microtime(true)), array('safe' => true));
} catch(MongoCursorException $e){
	var_dump($e->getMessage(), $e->getCode());
}
?>
--EXPECTF--
string(%s) "%s:%d: timeout"
int(4)
