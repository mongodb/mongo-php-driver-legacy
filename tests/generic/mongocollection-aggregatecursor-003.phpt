--TEST--
MongoCollection::aggregateCursor() with limit greater than batchSize
--SKIPIF--
<?php $needs = "2.5.3"; require_once "tests/utils/standalone.inc";?>
--FILE--
<?php
require "tests/utils/server.inc";

function log_query($server, $query, $info) {
    printf("Issuing command: %s\n", key($query));
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

$cursor = $collection->aggregateCursor(
    array(array('$limit' => 5)),
    array('cursor' => array('batchSize' => 2))
);

printf("Cursor class: %s\n", get_class($cursor));

foreach ($cursor as $key => $record) {
    var_dump($key);
    var_dump($record);
}

?>
===DONE===
--EXPECTF--
Issuing command: drop
Cursor class: MongoCommandCursor
Issuing command: aggregate
int(0)
array(2) {
  ["_id"]=>
  object(MongoId)#%d (1) {
    ["$id"]=>
    string(24) "5%s"
  }
  ["article_id"]=>
  int(0)
}
int(1)
array(2) {
  ["_id"]=>
  object(MongoId)#%d (1) {
    ["$id"]=>
    string(24) "5%s"
  }
  ["article_id"]=>
  int(1)
}
Issuing getmore
int(2)
array(2) {
  ["_id"]=>
  object(MongoId)#%d (1) {
    ["$id"]=>
    string(24) "5%s"
  }
  ["article_id"]=>
  int(2)
}
int(3)
array(2) {
  ["_id"]=>
  object(MongoId)#%d (1) {
    ["$id"]=>
    string(24) "5%s"
  }
  ["article_id"]=>
  int(3)
}
int(4)
array(2) {
  ["_id"]=>
  object(MongoId)#%d (1) {
    ["$id"]=>
    string(24) "5%s"
  }
  ["article_id"]=>
  int(4)
}
===DONE===
