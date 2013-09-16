--TEST--
MongoDB::selectCollection() 
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$dsn = MongoShellServer::getStandaloneInfo();
$m = new MongoClient($dsn);

$db = $m->selectDb(dbname());
$col = $db->createCollection("dropcollection");
$col->insert(array('boo' => 'bar'));

var_dump($db->dropCollection(0));
var_dump($db->dropCollection('dropcollection'));
?>
--EXPECTF--
Warning: MongoDB::dropCollection(): expects parameter 1 to be an string or MongoCollection in %smongodb-dropcollection.php on line %d
NULL
array(4) {
  ["nIndexesWas"]=>
  int(1)
  ["msg"]=>
  string(30) "indexes dropped for collection"
  ["ns"]=>
  string(19) "test.dropcollection"
  ["ok"]=>
  float(1)
}
