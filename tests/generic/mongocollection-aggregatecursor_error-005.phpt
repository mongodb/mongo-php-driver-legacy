--TEST--
MongoCollection::aggregateCursor() with null batchSize option
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

/* The following tests where batchSize is not a number would normally trigger
 * error 16956 ("cursor.batchSize must be a number") errors, but the driver
 * casts the option to a long value in get_batch_size_from_command().
 *
 * See: https://github.com/mongodb/mongo/commit/a51f2688fa05672d999c997170847a3ee29a223b#diff-264fb70c85a638c671570970f3752bf3R52
 */

$cursor = $collection->aggregateCursor(
    array(array('$limit' => 2)),
    array('cursor' => array('batchSize' => 0))
);
printf("Total results: %d\n", count(iterator_to_array($cursor)));

?>
===DONE===
--EXPECTF--
Issuing command: drop
Issuing command: aggregate
Cursor batch size: 0
Issuing getmore
Total results: 2
===DONE===
