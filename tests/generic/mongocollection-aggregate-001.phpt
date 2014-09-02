--TEST--
Test for PHP-914: MongoConnection::aggregate() needs an explain facility
--SKIPIF--
<?php $needs = "2.5.5"; ?>
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$dsn = MongoShellServer::getStandaloneInfo();
$mc = new MongoClient($dsn);
$c = $mc->selectCollection(dbname(), collname(__FILE__));
$c->drop();

foreach (range(1,5) as $x) {
    $c->insert(array('x' => $x));
}

$group = array('$group' => array('_id' => 1, 'count' => array('$sum' => 1)));
$project = array('$project' => array('count' => 1));

echo "pipeline array\n";
$retval = $c->aggregate(array($group));
var_dump($retval);
echo "pipeline array and options\n";
$retval = $c->aggregate(array($group), array("explain" => true));
var_dump(count($retval["stages"]));

echo "multiple pipelines in an array\n";
$retval = $c->aggregate(array($group, $project));
var_dump($retval);
echo "multiple pipelines in an array and options\n";
$retval = $c->aggregate(array($group, $project), array("explain" => true));
var_dump(count($retval["stages"]));

echo "Multiple pipelines with invalid pipe operator explain\n";
try {
    $retval = $c->aggregate($group, array("explain" => true));
    var_dump($retval);
    echo "FAILED - THAT SHOULD HAVE THROWN EXCEPTION\n";
} catch(MongoResultException $e) {
    echo $e->getMessage(), "\n";
}



?>
===DONE==
<?php exit(0) ?>
--EXPECTF--
pipeline array
array(2) {
  ["result"]=>
  array(1) {
    [0]=>
    array(2) {
      ["_id"]=>
      int(1)
      ["count"]=>
      int(5)
    }
  }
  ["ok"]=>
  float(1)
}
pipeline array and options
int(2)
multiple pipelines in an array
array(2) {
  ["result"]=>
  array(1) {
    [0]=>
    array(2) {
      ["_id"]=>
      int(1)
      ["count"]=>
      int(5)
    }
  }
  ["ok"]=>
  float(1)
}
multiple pipelines in an array and options
int(3)
Multiple pipelines with invalid pipe operator explain
%s:%d: exception: Unrecognized pipeline stage name: 'explain'
===DONE==
