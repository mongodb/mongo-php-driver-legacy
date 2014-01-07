--TEST--
Test for PHP-676: Collection level write concern overwritten by deprecated "safe" option
--SKIPIF--
<?php require_once "tests/utils/replicaset.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";
$m = new_mongo();

$oSemafor = $m->selectDb(dbname())->semafor;
$oSemafor->drop();
$oSemafor->w = 42;
$oSemafor->wtimeout = 30;

try{
        $time = microtime(true);
        $x = $oSemafor->insert(array('createts' => microtime(true)), array('safe' => true));
} catch(MongoCursorException $e){
    var_dump($e->getMessage(), $e->getCode());
}
?>
--EXPECTF--
%s: MongoCollection::insert(): The 'safe' option is deprecated, please use 'w' instead in %sbug00676.php on line %d
string(%d) "%s:%d: timeout%S"
int(4)
