--TEST--
MongoCursor::count() with hint option
--SKIPIF--
<?php $needs = "2.6.0"; require_once "tests/utils/standalone.inc";?>
--FILE--
<?php require_once "tests/utils/server.inc"; ?>
<?php

function log_query($server, $query, $info) {
    if (key($query) !== 'count') {
        return;
    }
    printf("Count hint: %s\n", array_key_exists('hint', $query) ? json_encode($query['hint']) : 'none');
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
$c->createIndex(array('y' => 1), array('name' => 'y_1', 'sparse' => true));

/* We use a non-empty query to ensure that the server uses the hinted index.
 * Without a query, the count is obtained directly from collection stats.
 */
printf("Count documents where x >= 0: %d\n", $c->find(array('x' => array('$gte' => 0)))->count());
echo "\n";
printf("Count documents where x >= 0, with hint: %d\n", $c->find(array('x' => array('$gte' => 0)))->hint('y_1')->count());

?>
===DONE===
--EXPECT--
Count hint: none
Count documents where x >= 0: 4

Count hint: "y_1"
Count documents where x >= 0, with hint: 2
===DONE===
