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

echo "Selecting collection\n";
$coll = $mongo->selectCollection(dbname(), 'mongolog');
$coll->drop();
echo "Finished\n";
?>
--EXPECTF--
bool(true)
string(1) "f"
Selecting collection
int(4)
int(4)
string(13) "getting reply"
int(4)
int(4)
string(21) "getting cursor header"
int(4)
int(4)
string(19) "getting cursor body"
Finished
