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

$db = $m->dropMe;
$db->data->insert(array("document" => "to drop"));

var_dump($db->data->find()->count());
$db->drop();
$db = $m->listDbs();
foreach($db["databases"] as $database) {
    if ($database["name"] == "dropMe") {
        echo "FAILED to drop the db!\n";
    }
}
?>
===DONE===
--EXPECT--
int(1)
===DONE===
