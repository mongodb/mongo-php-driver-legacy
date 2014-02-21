--TEST--
Connection strings: Test fsync over standalone server
--SKIPIF--
<?php if (!MONGO_STREAMS) { echo "skip This test requires streams support"; } ?>
<?php $needs = "2.5.5";?>
<?php if (version_compare(PHP_VERSION, "5.4.0", "le")) { exit("skip This test requires PHP version PHP5.4+"); }?>
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";
require_once "tests/utils/stream-notifications.inc";

function dump_writeConcern(MongoNotifications $mn) {
    $meta = $mn->getLastInsertMeta();
    var_dump($meta['write_options']['writeConcern']);
}

$host = MongoShellServer::getStandaloneInfo();


$mn = new MongoNotifications;
$ctx = stream_context_create(
    array(),
    array(
        "notification" => array($mn, "update")
    )
);

$mc = new MongoClient($host, array("fsync" => true), array("context" => $ctx));

echo "Fsync enabled by default\n";
$doc = array("doc" => "ument");
$mc->test->bug572->insert($doc);
dump_writeConcern($mn);
$mc->test->bug572->update(array("_id" => $doc["_id"]), array("updated" => "doc"));
dump_writeConcern($mn);
$mc->test->bug572->remove(array("_id" => $doc["_id"]));
dump_writeConcern($mn);

echo "Setting it to false, per-query\n";
$doc = array("doc" => "ument");
$mc->test->bug572->insert($doc, array("fsync" => false));
dump_writeConcern($mn);
$mc->test->bug572->update(array("_id" => $doc["_id"]), array("updated" => "doc"), array("fsync" => false));
dump_writeConcern($mn);
$mc->test->bug572->remove(array("_id" => $doc["_id"]), array("fsync" => false));
dump_writeConcern($mn);

echo "Setting it to false, per-query, and w=0 to force no-gle\n";
$doc = array("doc" => "ument");
$mc->test->bug572->insert($doc, array("fsync" => false, "w" => 0));
dump_writeConcern($mn);
$mc->test->bug572->update(array("_id" => $doc["_id"]), array("updated" => "doc"), array("fsync" => false, "w" => 0));
dump_writeConcern($mn);
$mc->test->bug572->remove(array("_id" => $doc["_id"]), array("fsync" => false, "w" => 0));
dump_writeConcern($mn);

$mc = new MongoClient($host, array("fsync" => false), array("context" => $ctx));

echo "Fsync disabled by default\n";
$doc = array("doc" => "ument");
$mc->test->bug572->insert($doc);
dump_writeConcern($mn);
$mc->test->bug572->update(array("_id" => $doc["_id"]), array("updated" => "doc"));
dump_writeConcern($mn);
$mc->test->bug572->remove(array("_id" => $doc["_id"]));
dump_writeConcern($mn);

echo "Setting it to true, per-query\n";
$doc = array("doc" => "ument");
$mc->test->bug572->insert($doc, array("fsync" => true));
dump_writeConcern($mn);
$mc->test->bug572->update(array("_id" => $doc["_id"]), array("updated" => "doc"), array("fsync" => true));
dump_writeConcern($mn);
$mc->test->bug572->remove(array("_id" => $doc["_id"]), array("fsync" => true));
dump_writeConcern($mn);

$mc = new MongoClient($host, array("fsync" => false, "w" => 0), array("context" => $ctx));

echo "Fsync disabled by default, and gle\n";
$doc = array("doc" => "ument");
$mc->test->bug572->insert($doc);
dump_writeConcern($mn);
$mc->test->bug572->update(array("_id" => $doc["_id"]), array("updated" => "doc"));
dump_writeConcern($mn);
$mc->test->bug572->remove(array("_id" => $doc["_id"]));
dump_writeConcern($mn);

echo "Setting it to true, per-query, with gle=0\n";
$doc = array("doc" => "ument");
$mc->test->bug572->insert($doc, array("fsync" => true));
dump_writeConcern($mn);
$mc->test->bug572->update(array("_id" => $doc["_id"]), array("updated" => "doc"), array("fsync" => true));
dump_writeConcern($mn);
$mc->test->bug572->remove(array("_id" => $doc["_id"]), array("fsync" => true));
dump_writeConcern($mn);

?>
--EXPECTF--
Fsync enabled by default
array(3) {
  ["fsync"]=>
  bool(true)
  ["j"]=>
  bool(false)
  ["w"]=>
  int(1)
}
array(3) {
  ["fsync"]=>
  bool(true)
  ["j"]=>
  bool(false)
  ["w"]=>
  int(1)
}
array(3) {
  ["fsync"]=>
  bool(true)
  ["j"]=>
  bool(false)
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
Fsync disabled by default
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
  bool(true)
  ["j"]=>
  bool(false)
  ["w"]=>
  int(1)
}
array(3) {
  ["fsync"]=>
  bool(true)
  ["j"]=>
  bool(false)
  ["w"]=>
  int(1)
}
array(3) {
  ["fsync"]=>
  bool(true)
  ["j"]=>
  bool(false)
  ["w"]=>
  int(1)
}
Fsync disabled by default, and gle
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
  bool(true)
  ["j"]=>
  bool(false)
  ["w"]=>
  int(0)
}
array(3) {
  ["fsync"]=>
  bool(true)
  ["j"]=>
  bool(false)
  ["w"]=>
  int(0)
}
array(3) {
  ["fsync"]=>
  bool(true)
  ["j"]=>
  bool(false)
  ["w"]=>
  int(0)
}
