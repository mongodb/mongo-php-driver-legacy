--TEST--
MongoCursor::count() inherits socket timeout from cursor
--SKIPIF--
<?php if (version_compare(phpversion(), "5.3.0", "lt")) exit("skip setCallback and closures are 5.3+"); ?>
<?php if (!MONGO_STREAMS) { echo "skip This test requires streams support"; } ?>
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php require_once "tests/utils/server.inc"; ?>
<?php

$host = MongoShellServer::getStandaloneInfo();
$m = new MongoClient($host);
$c = $m->selectCollection(dbname(), collname(__FILE__));

$c->drop();
$c->insert(array('x' => 1));
$c->insert(array('x' => 2));
$c->insert(array('x' => 3));
$c->insert(array('x' => 4));

printLogs(MongoLog::ALL, MongoLog::ALL, "/timeout/i");

printf("Count all documents, default timeout: %d\n", $c->find()->count());
echo "\n";
printf("Count all documents, timeout = 1000: %d\n", $c->find()->timeout(1000)->count());
?>
===DONE===
--EXPECTF--
Initializing cursor timeout to 30000 (from connection options)
Initializing cursor timeout to 30000 (from connection options)
No timeout changes for %s:%d;-;%s;%d
No timeout changes for %s:%d;-;%s;%d
Count all documents, default timeout: 4

Initializing cursor timeout to 30000 (from connection options)
Initializing cursor timeout to 30000 (from connection options)
Setting the stream timeout to 1.000000
Now setting stream timeout back to 30.000000
Setting the stream timeout to 1.000000
Now setting stream timeout back to 30.000000
Count all documents, timeout = 1000: 4
===DONE===