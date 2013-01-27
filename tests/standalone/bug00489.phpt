--TEST--
Test for PHP-489: ismaster() crashes for standalone servers
--SKIPIF--
<?php require_once dirname(__FILE__) . "/skipif.inc"; ?>
--FILE--
<?php
require_once dirname(__FILE__) . "/../utils.inc";
MongoLog::setLevel(MongoLog::WARNING);
MongoLog::setModule(MongoLog::CON);
try {
	$m = new Mongo(hostname(), array("replicaSet" => true));
} catch(MongoConnectionException $e) {
	var_dump($e->getMessage());
}
echo "I'm alive!\n";
?>
==DONE==
--EXPECTF--
Notice: CON	WARN: Host does not seem to be a replicaset member (%s:%d) in %s on line %d

Notice: CON	WARN: discover_topology: ismaster return with an error for %s:%d: [Host does not seem to be a replicaset member (%s:%d)] in %s on line %d
string(26) "No candidate servers found"
I'm alive!
==DONE==
