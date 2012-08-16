--TEST--
Test for MongoLog with callback
--SKIPIF--
<?php require_once dirname(__FILE__) ."/skipif.inc"; ?>
--FILE--
<?php
require_once dirname(__FILE__) . "/../utils.inc";

function f($module, $log, $m) {
    var_dump($module, $log, $m);
}

MongoLog::setModule(MongoLog::IO);
MongoLog::setLevel(MongoLog::FINE);
var_dump(MongoLog::setCallback("f"));


$mongo = mongo();


var_dump(MongoLog::getCallback());

$coll = $mongo->selectCollection(dbname(), 'mongolog');
$coll->drop();
?>
--EXPECTF--
bool(true)
string(1) "f"
int(4)
int(4)
string(17) "hearing something"
