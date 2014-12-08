--TEST--
MongoCollection::group()
--SKIPIF--
<?php $needs = "2.5.3"; require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$host = MongoShellServer::getStandaloneInfo();
$mc = new MongoClient($host);

$collection = $mc->selectCollection(dbname(), collname(__FILE__));
$collection->drop();
$collection->insert(array('x' => 1));
$collection->insert(array('x' => 1));
$collection->insert(array('x' => 2));

$result = $collection->group(
  array('x' => 1),
  array('n' => 0),
  'function (obj, prev) { prev.n += 1; }'
);

foreach ($result['retval'] as $group) {
  printf("Grouped %d document(s) where x = %d\n", $group['n'], $group['x']);
}
?>
===DONE===
<?php exit(0); ?>
--EXPECTF--
Grouped 2 document(s) where x = 1
Grouped 1 document(s) where x = 2
===DONE===
