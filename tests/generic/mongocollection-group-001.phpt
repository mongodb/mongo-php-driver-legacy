--TEST--
MongoCollection::group() with maxTimeMS option
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

configureFailPoint($mc, 'maxTimeAlwaysTimeOut', 1);

echo "\ngroup without maxTimeMS option\n";

$result = $collection->group(
  array('x' => 1),
  array('n' => 0),
  new MongoCode('function (obj, prev) { prev.n += 1; }')
);

foreach ($result['retval'] as $group) {
  printf("Grouped %d document(s) where x = %d\n", $group['n'], $group['x']);
}

echo "\ngroup with maxTimeMS option\n";

try {
  $result = $collection->group(
    array('x' => 1),
    array('n' => 0),
    new MongoCode('function (obj, prev) { prev.n += 1; }'),
    array('maxTimeMS' => 100)
  );

  foreach ($result['retval'] as $group) {
    printf("Grouped %d document(s) where x = %d\n", $group['n'], $group['x']);
  }
} catch (Exception $e) {
    printf("exception class: %s\n", get_class($e));
    printf("exception message: %s\n", $e->getMessage());
    printf("exception code: %d\n", $e->getCode());
}

?>
===DONE===
<?php exit(0); ?>
--EXPECTF--
Configured maxTimeAlwaysTimeOut fail point

group without maxTimeMS option
Grouped 2 document(s) where x = 1
Grouped 1 document(s) where x = 2

group with maxTimeMS option
exception class: MongoExecutionTimeoutException
exception message: %s:%d: operation exceeded time limit
exception code: 50
===DONE===
