--TEST--
MongoDB::command() with maxTimeMS option
--SKIPIF--
<?php $needs = "2.5.3"; require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$host = MongoShellServer::getStandaloneInfo();
$mc = new MongoClient($host);

configureFailPoint($mc, 'maxTimeAlwaysTimeOut', 2);

$db = $mc->selectDB(dbname());
$collection = $db->selectCollection(collname(__FILE__));
$collection->drop();
$collection->insert(array('x' => 1));
$collection->insert(array('x' => 2));

echo "\naggregate command with maxTimeMS\n";

$result = $db->command(array(
    'aggregate' => collname(__FILE__),
    'pipeline' => array('$match' => array('x' => 1)),
    'maxTimeMS' => 100,
));
dump_these_keys($result, array('ok', 'errmsg', 'code'));

echo "\nserverStatus command with maxTimeMS\n";

$result = $db->command(array(
    'serverStatus' => 1,
    'maxTimeMS' => 100,
));
dump_these_keys($result, array('ok', 'errmsg', 'code'));

?>
===DONE===
--EXPECTF--
Configured maxTimeAlwaysTimeOut fail point

aggregate command with maxTimeMS
array(3) {
  ["ok"]=>
  float(0)
  ["errmsg"]=>
  string(29) "operation exceeded time limit"
  ["code"]=>
  int(50)
}

serverStatus command with maxTimeMS
array(3) {
  ["ok"]=>
  float(0)
  ["errmsg"]=>
  string(29) "operation exceeded time limit"
  ["code"]=>
  int(50)
}
===DONE===
