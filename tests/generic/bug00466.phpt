--TEST--
Test for PHP-466: Seed list of 1 replicaset member, and one standalone, with array("replicaSet" => true) fails.
--FILE--
<?php
MongoLog::setModule(MongoLog::ALL);
MongoLog::setLevel(MongoLog::ALL);
function foo($c, $m) { echo $m, "\n"; } set_error_handler('foo');
$m = new MongoClient("mongodb://whisky:13000", array( "connect" => false, "replicaSet" => true ));
$m = new MongoClient("mongodb://whisky:13000", array( "connect" => false, "replicaSet" => 'seta' ));
?>
--EXPECTF--
PARSE   INFO: Parsing mongodb://whisky:13000
PARSE   INFO: - Found node: whisky:13000
PARSE   INFO: - Connection type: STANDALONE
PARSE   WARN: - Found option 'replicaSet': true - Expected the name of the replica set
PARSE   INFO: - Switching connection type: REPLSET
PARSE   INFO: Parsing mongodb://whisky:13000
PARSE   INFO: - Found node: whisky:13000
PARSE   INFO: - Connection type: STANDALONE
PARSE   INFO: - Found option 'replicaSet': 'seta'
PARSE   INFO: - Switching connection type: REPLSET
