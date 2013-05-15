--TEST--
Unix domain socket connection string test
--SKIPIF--
<?php require_once dirname(__FILE__) . "/skipif.inc"; ?>
--FILE--
<?php

MongoLog::setModule(MongoLog::ALL);
MongoLog::setLevel(MongoLog::ALL);



echo "First one\n";
try {
    $m = new MongoClient("mongodb:///tmp/mongodb-27017.sock/?replicaSet=foo", array("connect" => 0));
} catch(Exception $e) {}


echo "\nSecond one\n";
try {
    $m = new MongoClient("mongodb:///tmp/mongodb-27017.sock/databasename?replicaSet=foo", array("connect" => 0));
} catch(Exception $e) {}


echo "\nThird one\n";
try {
    $m = new MongoClient("mongodb:///tmp/mongodb-27017.sock", array("connect" => 0));
} catch(Exception $e) {}


echo "\nForth one\n";
try {
    $m = new MongoClient("mongodb:///tmp/mongodb-27017.sock/databasename", array("connect" => 0));
} catch(Exception $e) {}
?>
--EXPECTF--
First one

Notice: PARSE   INFO: Parsing mongodb:///tmp/mongodb-27017.sock/?replicaSet=foo in %s on line %d

Notice: PARSE   INFO: - Found node: /tmp/mongodb-27017.sock:0 in %s on line %d

Notice: PARSE   INFO: - Connection type: STANDALONE in %s on line %d

Notice: PARSE   INFO: - Found option 'replicaSet': 'foo' in %s on line %d

Notice: PARSE   INFO: - Switching connection type: REPLSET in %s on line %d

Second one

Notice: PARSE   INFO: Parsing mongodb:///tmp/mongodb-27017.sock/databasename?replicaSet=foo in %s on line %d

Notice: PARSE   INFO: - Found node: /tmp/mongodb-27017.sock:0 in %s on line %d

Notice: PARSE   INFO: - Connection type: STANDALONE in %s on line %d

Notice: PARSE   INFO: - Found option 'replicaSet': 'foo' in %s on line %d

Notice: PARSE   INFO: - Switching connection type: REPLSET in %s on line %d

Notice: PARSE   INFO: - Found database name 'databasename' in %s on line %d

Third one

Notice: PARSE   INFO: Parsing mongodb:///tmp/mongodb-27017.sock in %s on line %d

Notice: PARSE   INFO: - Found node: /tmp/mongodb-27017.sock:0 in %s on line %d

Notice: PARSE   INFO: - Connection type: STANDALONE in %s on line %d

Forth one

Notice: PARSE   INFO: Parsing mongodb:///tmp/mongodb-27017.sock/databasename in %s on line %d

Notice: PARSE   INFO: - Found node: /tmp/mongodb-27017.sock:0 in %s on line %d

Notice: PARSE   INFO: - Connection type: STANDALONE in %s on line %d

Notice: PARSE   INFO: - Found database name 'databasename' in %s on line %d

