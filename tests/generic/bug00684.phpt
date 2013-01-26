--TEST--
Test for PHP-:
--SKIPIF--
<?php require_once dirname(__FILE__) . "/skipif.inc" ?>
--FILE--
<?php
require_once dirname(__FILE__) . "/../utils.inc";

printLogs(MongoLog::ALL, MongoLog::ALL, "/timeout/i");


echo "sockettimeoutms\n";
$m = new_mongo(null, true, true, array("socketTimeoutMS" => 1, "connect" => false));

echo "sockettimeoutms lowercased\n";
$m = new_mongo(null, true, true, array("socketTimeoutMS" => 2, "connect" => false));

echo "sockettimeoutms\n";
$m = new MongoClient("localhost/?socketTimeoutMS=42", array("connect" => false));

echo "sockettimeoutms lowercased\n";
$m = new MongoClient("localhost/?sockettimeoutms=52", array("connect" => false));

?>
--EXPECT--
sockettimeoutms
- Found option 'socketTimeoutMS': 1
sockettimeoutms lowercased
- Found option 'socketTimeoutMS': 2
sockettimeoutms
Parsing localhost/?socketTimeoutMS=42
- Found option 'socketTimeoutMS': 42
sockettimeoutms lowercased
Parsing localhost/?sockettimeoutms=52
- Found option 'socketTimeoutMS': 52
