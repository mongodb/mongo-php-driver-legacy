--TEST--
Test for PHP-1460: Query with limit leaves open cursors on server (getNext/hasNext iteration)
--SKIPIF--
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

function getNumOpenCursors(MongoClient $mc) {
    $result = $mc->admin->command(array('serverStatus' => 1));

    if (isset($result['metrics']['cursor']['open']['total'])) {
        return (integer) $result['metrics']['cursor']['open']['total'];
    }

    if (isset($result['cursors']['totalOpen'])) {
        return (integer) $result['cursors']['totalOpen'];
    }

    throw new RuntimeException('serverStatus did not return cursor metrics');
}

function log_killcursor($server, $info) {
    printf("Killing cursor: %d\n", $info['cursor_id']);
}

function log_getmore($server, $info) {
    printf("Getmore on cursor: %d\n", $info['cursor_id']);
}

$ctx = stream_context_create(array(
    'mongodb' => array(
        'log_getmore' => 'log_getmore',
        'log_killcursor' => 'log_killcursor',
    ),
));

$host = MongoShellServer::getStandaloneInfo();
$mc = new MongoClient($host, array(), array('context' => $ctx));

$c = $mc->selectCollection(dbname(), collname(__FILE__));
$c->drop();

for ($i = 0; $i < 15; $i++) {
    $c->insert(array('_id' => $i));
}

$numOpenCursorsBeforeQuery = getNumOpenCursors($mc);
printf("Number of open cursors before query: %d\n", $numOpenCursorsBeforeQuery);

$cursor = $c->find()->limit(10)->batchSize(5);
printf("Cursor is dead: %s\n", $cursor->dead() ? 'true' : 'false');

while ($cursor->hasNext()) {
    $document = $cursor->getNext();
    printf("Found document: %d\n", $document['_id']);
    if ($document['_id'] === 4 || $document['_id'] === 9) {
        printf("Cursor is dead: %s\n", $cursor->dead() ? 'true' : 'false');
    }
}

printf("Cursor is dead: %s\n", $cursor->dead() ? 'true' : 'false');

$numOpenCursorsAfterQuery = getNumOpenCursors($mc);
printf("Number of open cursors after query: %d\n", $numOpenCursorsAfterQuery);
printf("Same number of cursors open before and after query: %s\n", $numOpenCursorsBeforeQuery === $numOpenCursorsAfterQuery ? 'true' : 'false');

?>
===DONE===
--EXPECTF--
Number of open cursors before query: %d
Cursor is dead: false
Found document: 0
Found document: 1
Found document: 2
Found document: 3
Found document: 4
Cursor is dead: false
Getmore on cursor: %d
Found document: 5
Found document: 6
Found document: 7
Found document: 8
Found document: 9
Cursor is dead: false
Killing cursor: %d
Cursor is dead: true
Number of open cursors after query: %d
Same number of cursors open before and after query: true
===DONE===
