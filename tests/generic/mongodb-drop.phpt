--TEST--
MongoDB::drop() Drop a database
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php require_once "tests/utils/server.inc"; ?>
<?php

require_once "tests/utils/server.inc";

$dsn = MongoShellServer::getStandaloneInfo();
$m = new MongoClient($dsn);

$db = $m->selectDb(dbname());
$db->drop();

$db->collection->insert(array('_id' => 1));
var_dump($db->collection->count());

$db->drop();

$list = $m->listDBs();

foreach($list['databases'] as $database) {
    if ($database['name'] == (string) $db) {
        printf("FAILED to drop databse: %s\n", $db);
    }
}
?>
===DONE===
--EXPECT--
int(1)
===DONE===
