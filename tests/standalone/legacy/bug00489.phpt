--TEST--
Test for PHP-489: ismaster() crashes for standalone servers
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php
require_once "tests/utils/server.inc";
MongoLog::setLevel(MongoLog::WARNING);
MongoLog::setModule(MongoLog::CON);
$dsn = MongoShellServer::getStandaloneInfo();
try {
    $m = new MongoClient($dsn, array("replicaSet" => true));
    echo "Have mongo object\n";
    $m->foo->bar->findOne();
} catch(MongoConnectionException $e) {
    var_dump($e->getMessage());
}
echo "I'm alive!\n";
?>
==DONE==
--EXPECTF--
Notice: CON     WARN: Can't find 'me' in ismaster response, possibly not a replicaset (%s:%d) in %s on line %d

Notice: CON     WARN: discover_topology: ismaster return with an error for %s:%d: [Not a replicaset member] in %s on line %d
string(26) "No candidate servers found"
I'm alive!
==DONE==
