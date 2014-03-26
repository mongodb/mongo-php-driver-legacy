--TEST--
MongoCollection::aggregateCursor() with negative batchSize option
--SKIPIF--
<?php $needs = "2.5.3"; require_once "tests/utils/standalone.inc";?>
--FILE--
<?php
require "tests/utils/server.inc";

function log_query($server, $query, $info) {
    printf("Issuing command: %s\n", key($query));

    if (isset($query['cursor']['batchSize'])) {
        printf("Cursor batch size: %d\n", $query['cursor']['batchSize']);
    }
}

function log_getmore($server, $info) {
    echo "Issuing getmore\n";
}

$ctx = stream_context_create(array(
    'mongodb' => array(
        'log_query' => 'log_query',
        'log_getmore' => 'log_getmore',
    ),
));

$host = MongoShellServer::getStandaloneInfo();
$mc = new MongoClient($host, array(), array('context' => $ctx));

$collection = $mc->selectCollection(dbname(), collname(__FILE__));
$collection->drop();

for ($i = 0; $i < 10; $i++) {
    $collection->insert(array('article_id' => $i));
}

/* The following test current triggers error 16957 ("Cursor batchSize must not
   be negative"), but this might change in the future.
 *
 * See: https://github.com/mongodb/mongo/commit/a51f2688fa05672d999c997170847a3ee29a223b#diff-264fb70c85a638c671570970f3752bf3R55
 */

$cursor = $collection->aggregateCursor(
    array( array( '$limit' => 2 ) ),
    array( 'cursor' => array( 'batchSize' => -1 ) )
);

try {
    printf("Total results: %d\n", count(iterator_to_array($cursor)));
} catch (MongoResultException $e) {
    printf("exception message: %s\n", $e->getMessage());
    printf("exception code: %d\n", $e->getCode());
}

?>
===DONE===
--EXPECTF--
Issuing command: drop
Issuing command: aggregate
Cursor batch size: -1
exception message: %s:%d: exception: Cursor batchSize must not be negative
exception code: 16957
===DONE===
