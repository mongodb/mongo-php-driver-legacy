--TEST--
MongoCursor::count() with maxTimeMS option
--SKIPIF--
<?php $needs = "2.6.0"; require_once "tests/utils/standalone.inc";?>
--FILE--
<?php require_once "tests/utils/server.inc"; ?>
<?php

function log_query($server, $query, $info) {
    if (key($query) !== 'count') {
        return;
    }
    printf("Count maxTimeMS: %s\n", array_key_exists('maxTimeMS', $query) ? $query['maxTimeMS'] : 'none');
}

$ctx = stream_context_create(array(
    'mongodb' => array(
        'log_query' => 'log_query',
    ),
));

$host = MongoShellServer::getStandaloneInfo();
$m = new MongoClient($host, array(), array('context' => $ctx));
$c = $m->selectCollection(dbname(), collname(__FILE__));
$c->drop();

$c->insert(array('x' => 1));
$c->insert(array('x' => 2, 'y' => 2));
$c->insert(array('x' => 3, 'y' => 3));
$c->insert(array('x' => 4));

printf("Count documents, no maxTimeMS: %d\n", $c->find()->count());
echo "\n";
printf("Count documents, maxTimeMS = 1000: %d\n", $c->find()->maxTimeMS(1000)->count());

?>
===DONE===
--EXPECT--
Count maxTimeMS: none
Count documents, no maxTimeMS: 4

Count maxTimeMS: 1000
Count documents, maxTimeMS = 1000: 4
===DONE===
