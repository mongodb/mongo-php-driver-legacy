--TEST--
Test for PHP-485: Update (and other methods) in safemode crash under certain conditions.
--SKIPIF--
<?php require_once "tests/utils/replicaset.inc" ?>
--INI--
report_memleaks=off
--FILE--
<?php
require_once "tests/utils/server.inc";
// Connect to mongo
$m = mongo();
$collection = $m->selectCollection(dbname(), 'crash');
$collection->drop();

// Load the problem record.
$arrayData = file_get_contents('tests/data-files/bug00485-data.txt');
$doc = unserialize($arrayData);

$collection->insert( $doc );

$doc = $collection->findOne(array('_id' => new MongoId('4ffe06d19da778b67809666a')));

$collection->update(array('_id' => $doc['_id']), array('$set' => array('image.id' => new MongoId('50470e396e6adf8f4a000039'))));
$res = $collection->save($doc, array('safe' => 1));
var_dump($res);
echo "DONE\n";
?>
--EXPECTF--
array(6) {
  ["updatedExisting"]=>
  bool(true)
  ["n"]=>
  int(1)
  ["lastOp"]=>
  object(MongoTimestamp)#%d (2) {
    ["sec"]=>
    int(%d)
    ["inc"]=>
    int(%d)
  }
  ["connectionId"]=>
  int(%d)
  ["err"]=>
  NULL
  ["ok"]=>
  float(1)
}
DONE
