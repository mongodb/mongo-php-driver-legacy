--TEST--
MongoCursor::batchSize() of 1 and -1 closes cursor after returning one document
--SKIPIF--
<?php require "tests/utils/standalone.inc"; ?>
--FILE--
<?php
require "tests/utils/server.inc";

function log_getmore($server, $cursorOptions) {
    printf("Issuing getmore with batchSize: %d\n", $cursorOptions['batch_size']);
}

$ctx = stream_context_create(array(
    "mongodb" => array(
        "log_getmore" => "log_getmore",
    ),
));

$host = MongoShellServer::getStandaloneInfo();
$m = new MongoClient($host, array(), array('context' => $ctx));

$c = $m->selectCollection(dbname(), collname(__FILE__));
$c->drop();

$c->insert(array('_id' => 1));
$c->insert(array('_id' => 2));
$c->insert(array('_id' => 3));

echo "Testing with batchSize = 1\n";

$cursor = $c->find()->batchSize(1);

printf("Cursor hasNext: %s\n", $cursor->hasNext() ? 'true' : 'false');
printf("Cursor is dead: %s\n", $cursor->dead() ? 'true' : 'false');
$document = $cursor->getNext();
printf("Found document with ID: %d\n", $document['_id']);

echo "\nTesting with batchSize = -1\n";

$cursor = $c->find()->batchSize(-1);

printf("Cursor hasNext: %s\n", $cursor->hasNext() ? 'true' : 'false');
printf("Cursor is dead: %s\n", $cursor->dead() ? 'true' : 'false');
$document = $cursor->getNext();
printf("Found document with ID: %d\n", $document['_id']);

?>
===DONE===
--EXPECT--
Testing with batchSize = 1
Cursor hasNext: true
Cursor is dead: true
Found document with ID: 1

Testing with batchSize = -1
Cursor hasNext: true
Cursor is dead: true
Found document with ID: 1
===DONE===
