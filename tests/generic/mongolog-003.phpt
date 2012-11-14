--TEST--
Test for MongoLog with callback (>= PHP 5.3).
--SKIPIF--
<?php if (!version_compare(phpversion(), "5.3", '>=')) echo "skip >= PHP 5.3 needed\n"; ?>
<?php require_once dirname(__FILE__) ."/skipif.inc"; ?>
--FILE--
<?php
require_once dirname(__FILE__) . "/../utils.inc";

function f($module, $moduleName, $log, $logName, $m) {
    printf("%-7s %-4s: %s\n", $moduleName, $logName, $m);
}

var_dump(MongoLog::getCallback());

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
bool(false)
bool(true)
string(1) "f"
Selecting collection
IO      FINE: getting reply
IO      FINE: getting cursor header
IO      FINE: getting cursor body
Finished
