--TEST--
MongoCollection::aggregate() $out operator and read preferences
--INI--
error_reporting=-1
--SKIPIF--
<?php $needs = "2.5.3" ?>
<?php require_once "tests/utils/replicaset.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$rs = MongoShellServer::getReplicasetInfo();

$mc = new MongoClient($rs["dsn"], array(
    "replicaSet" => $rs["rsname"],
    "readPreference" => MongoClient::RP_SECONDARY,
));

$c = $mc->selectCollection(dbname(), "aggout");
$cout = $mc->selectCollection(dbname(), "aggoutput");
$c->drop();
$cout->drop();

$data = array (
    "title" => "this is my title",
    "author" => "bob",
    "posted" => new MongoDate,
    "pageViews" => 5,
    "tags" => 
    array (
      0 => "fun",
      1 => "good",
      2 => "fun",
    ),
    "comments" => 
    array (
      0 => 
      array (
        "author" => "joe",
        "text" => "this is cool",
      ),
      1 => 
      array (
        "author" => "sam",
        "text" => "this is bad",
      ),
    ),
    "other" => 
    array (
      "foo" => 5,
    ),
);
$d = $c->insert($data, array("w" => true));

$ops = array(
    array(
        '$project' => array(
            "author" => 1,
            "tags"   => 1,
        )
    ),
    array('$unwind' => '$tags'),
    array(
        '$group' => array(
            "_id" => array("tags" => '$tags'),
            "authors" => array('$addToSet' => '$author'),
        )
    ),
    array('$out' => "aggoutput"),
);


$alone = $c->aggregate($ops);
var_dump($alone);
reset($ops);
$multiple = $c->aggregate(current($ops), next($ops), next($ops), next($ops));
var_dump($multiple);
$cout->setReadPreference(MongoClient::RP_PRIMARY);
var_dump($cout->findOne());
$c->drop();
$cout->drop();

?>
===DONE===
<?php exit(0); ?>
--EXPECTF--
Warning: MongoCollection::aggregate(): Forcing aggregate with $out to run on primary in %s on line %d
array(2) {
  ["result"]=>
  array(0) {
  }
  ["ok"]=>
  float(1)
}

Warning: MongoCollection::aggregate(): Forcing aggregate with $out to run on primary in %s on line %d
array(2) {
  ["result"]=>
  array(0) {
  }
  ["ok"]=>
  float(1)
}
array(2) {
  ["_id"]=>
  array(1) {
    ["tags"]=>
    string(4) "good"
  }
  ["authors"]=>
  array(1) {
    [0]=>
    string(3) "bob"
  }
}
===DONE===
