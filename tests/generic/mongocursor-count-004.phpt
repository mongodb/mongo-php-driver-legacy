--TEST--
MongoCursor::count() with empty query
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php require_once "tests/utils/server.inc"; ?>
<?php

function log_query($server, $query, $info) {
    if (key($query) !== 'count') {
        return;
    }
    printf("Count query: %s\n", array_key_exists('query', $query) ? json_encode($query['query']) : 'none');
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
$c->insert(array('x' => 2));
$c->insert(array('x' => 3));
$c->insert(array('x' => 4));

printf("Count documents with default query: %d\n", $c->find()->count());
echo "\n";
printf("Count documents with empty array query: %d\n", $c->find()->addOption('$query', array())->count());
echo "\n";
printf("Count documents with empty object query: %d\n", $c->find()->addOption('$query', new stdClass())->count());

?>
===DONE===
--EXPECT--
Count query: none
Count documents with default query: 4

Count query: none
Count documents with empty array query: 4

Count query: none
Count documents with empty object query: 4
===DONE===
