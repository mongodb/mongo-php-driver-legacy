--TEST--
MongoCursor::batchSize() with lesser limit, requesting more results than exist
--SKIPIF--
<?php require "tests/utils/standalone.inc"; ?>
--FILE--
<?php
require "tests/utils/server.inc";

function log_getmore($server, $cursorOptions) {
    printf("(Issuing getmore, batchSize: %d)\n", $cursorOptions['batch_size']);
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

for ($i = 0; $i < 20; $i++) {
    $c->insert(array('_id' => $i));
}

$cursor = $c->find(array('_id' => array('$gte' => 5)))->batchSize(100)->limit(20);

echo "Iterating with getNext()\n";

while ($r = $cursor->getNext()) {
    echo $r['_id'], ' ';
}

echo "\n\nIterating with hasNext() and getNext()\n";

$cursor->reset();

while ($cursor->hasNext()) {
    $r = $cursor->getNext();
    echo $r['_id'], ' ';
}

echo "\n\nIterating with foreach\n";

foreach ($cursor as $r) {
    echo $r['_id'], ' ';
}

echo "\n\nIterating with iterator_to_array()\n";

echo implode(
    array_map(
        function($r) { return $r['_id']; },
        iterator_to_array($cursor)
    ),
    ' '
);

?>
--EXPECT--
Iterating with getNext()
5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 

Iterating with hasNext() and getNext()
5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 

Iterating with foreach
5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 

Iterating with iterator_to_array()
5 6 7 8 9 10 11 12 13 14 15 16 17 18 19
