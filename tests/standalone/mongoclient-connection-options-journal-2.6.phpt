--TEST--
Connection strings: Test journal over standalone server
--SKIPIF--
<?php if (!MONGO_STREAMS) { echo "skip This test requires streams support"; } ?>
<?php $needs = "2.5.5"; ?>
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";
require_once "tests/utils/stream-notifications.inc";

$host = MongoShellServer::getStandaloneInfo();


$mn = new MongoNotifications;
$ctx = stream_context_create(
    array(),
    array(
        "notification" => array($mn, "update")
    )
);


$mc = new MongoClient($host, array("journal" => true), array("context" => $ctx));

echo "journal enabled by default\n";
$doc = array("doc" => "ument");
$mc->test->bug572->insert($doc);
var_dump($mn->getLastInsertMeta()["write_options"]["writeConcern"]);
$mc->test->bug572->update(array("_id" => $doc["_id"]), array("updated" => "doc"));
var_dump($mn->getLastInsertMeta()["write_options"]["writeConcern"]);
$mc->test->bug572->remove(array("_id" => $doc["_id"]));
var_dump($mn->getLastInsertMeta()["write_options"]["writeConcern"]);

echo "Setting it to false, per-query\n";
$doc = array("doc" => "ument");
$mc->test->bug572->insert($doc, array("j" => false));
var_dump($mn->getLastInsertMeta()["write_options"]["writeConcern"]);
$mc->test->bug572->update(array("_id" => $doc["_id"]), array("updated" => "doc"), array("j" => false));
var_dump($mn->getLastInsertMeta()["write_options"]["writeConcern"]);
$mc->test->bug572->remove(array("_id" => $doc["_id"]), array("j" => false));
var_dump($mn->getLastInsertMeta()["write_options"]["writeConcern"]);

echo "Setting it to false, per-query, and w=0 to force no-gle\n";
$doc = array("doc" => "ument");
$mc->test->bug572->insert($doc, array("j" => false, "w" => 0));
var_dump($mn->getLastInsertMeta()["write_options"]["writeConcern"]);
$mc->test->bug572->update(array("_id" => $doc["_id"]), array("updated" => "doc"), array("j" => false, "w" => 0));
var_dump($mn->getLastInsertMeta()["write_options"]["writeConcern"]);
$mc->test->bug572->remove(array("_id" => $doc["_id"]), array("j" => false, "w" => 0));
var_dump($mn->getLastInsertMeta()["write_options"]["writeConcern"]);

$mc = new MongoClient($host, array("journal" => false), array("context" => $ctx));

echo "journal disabled by default\n";
$doc = array("doc" => "ument");
$mc->test->bug572->insert($doc);
var_dump($mn->getLastInsertMeta()["write_options"]["writeConcern"]);
$mc->test->bug572->update(array("_id" => $doc["_id"]), array("updated" => "doc"));
var_dump($mn->getLastInsertMeta()["write_options"]["writeConcern"]);
$mc->test->bug572->remove(array("_id" => $doc["_id"]));
var_dump($mn->getLastInsertMeta()["write_options"]["writeConcern"]);

echo "Setting it to true, per-query\n";
$doc = array("doc" => "ument");
$mc->test->bug572->insert($doc, array("j" => true));
var_dump($mn->getLastInsertMeta()["write_options"]["writeConcern"]);
$mc->test->bug572->update(array("_id" => $doc["_id"]), array("updated" => "doc"), array("j" => true));
var_dump($mn->getLastInsertMeta()["write_options"]["writeConcern"]);
$mc->test->bug572->remove(array("_id" => $doc["_id"]), array("j" => true));
var_dump($mn->getLastInsertMeta()["write_options"]["writeConcern"]);

$mc = new MongoClient($host, array("journal" => false, "w" => 0), array("context" => $ctx));

echo "journal disabled by default, and gle\n";
$doc = array("doc" => "ument");
$mc->test->bug572->insert($doc);
var_dump($mn->getLastInsertMeta()["write_options"]["writeConcern"]);
$mc->test->bug572->update(array("_id" => $doc["_id"]), array("updated" => "doc"));
var_dump($mn->getLastInsertMeta()["write_options"]["writeConcern"]);
$mc->test->bug572->remove(array("_id" => $doc["_id"]));
var_dump($mn->getLastInsertMeta()["write_options"]["writeConcern"]);

echo "Setting it to true, per-query, with gle=0\n";
$doc = array("doc" => "ument");
$mc->test->bug572->insert($doc, array("j" => true));
var_dump($mn->getLastInsertMeta()["write_options"]["writeConcern"]);
$mc->test->bug572->update(array("_id" => $doc["_id"]), array("updated" => "doc"), array("j" => true));
var_dump($mn->getLastInsertMeta()["write_options"]["writeConcern"]);
$mc->test->bug572->remove(array("_id" => $doc["_id"]), array("j" => true));
var_dump($mn->getLastInsertMeta()["write_options"]["writeConcern"]);

?>
--EXPECTF--
journal enabled by default
array(3) {
  ["fsync"]=>
  bool(false)
  ["j"]=>
  bool(true)
  ["w"]=>
  int(1)
}
array(3) {
  ["fsync"]=>
  bool(false)
  ["j"]=>
  bool(true)
  ["w"]=>
  int(1)
}
array(3) {
  ["fsync"]=>
  bool(false)
  ["j"]=>
  bool(true)
  ["w"]=>
  int(1)
}
Setting it to false, per-query
array(3) {
  ["fsync"]=>
  bool(false)
  ["j"]=>
  bool(false)
  ["w"]=>
  int(1)
}
array(3) {
  ["fsync"]=>
  bool(false)
  ["j"]=>
  bool(false)
  ["w"]=>
  int(1)
}
array(3) {
  ["fsync"]=>
  bool(false)
  ["j"]=>
  bool(false)
  ["w"]=>
  int(1)
}
Setting it to false, per-query, and w=0 to force no-gle
array(3) {
  ["fsync"]=>
  bool(false)
  ["j"]=>
  bool(false)
  ["w"]=>
  int(0)
}
array(3) {
  ["fsync"]=>
  bool(false)
  ["j"]=>
  bool(false)
  ["w"]=>
  int(0)
}
array(3) {
  ["fsync"]=>
  bool(false)
  ["j"]=>
  bool(false)
  ["w"]=>
  int(0)
}
journal disabled by default
array(3) {
  ["fsync"]=>
  bool(false)
  ["j"]=>
  bool(false)
  ["w"]=>
  int(1)
}
array(3) {
  ["fsync"]=>
  bool(false)
  ["j"]=>
  bool(false)
  ["w"]=>
  int(1)
}
array(3) {
  ["fsync"]=>
  bool(false)
  ["j"]=>
  bool(false)
  ["w"]=>
  int(1)
}
Setting it to true, per-query
array(3) {
  ["fsync"]=>
  bool(false)
  ["j"]=>
  bool(true)
  ["w"]=>
  int(1)
}
array(3) {
  ["fsync"]=>
  bool(false)
  ["j"]=>
  bool(true)
  ["w"]=>
  int(1)
}
array(3) {
  ["fsync"]=>
  bool(false)
  ["j"]=>
  bool(true)
  ["w"]=>
  int(1)
}
journal disabled by default, and gle
array(3) {
  ["fsync"]=>
  bool(false)
  ["j"]=>
  bool(false)
  ["w"]=>
  int(0)
}
array(3) {
  ["fsync"]=>
  bool(false)
  ["j"]=>
  bool(false)
  ["w"]=>
  int(0)
}
array(3) {
  ["fsync"]=>
  bool(false)
  ["j"]=>
  bool(false)
  ["w"]=>
  int(0)
}
Setting it to true, per-query, with gle=0
array(3) {
  ["fsync"]=>
  bool(false)
  ["j"]=>
  bool(true)
  ["w"]=>
  int(0)
}
array(3) {
  ["fsync"]=>
  bool(false)
  ["j"]=>
  bool(true)
  ["w"]=>
  int(0)
}
array(3) {
  ["fsync"]=>
  bool(false)
  ["j"]=>
  bool(true)
  ["w"]=>
  int(0)
}
--EXPECTF--
