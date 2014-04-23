--TEST--
Connection strings: Test fsync over standalone server
--SKIPIF--
<?php if (!MONGO_STREAMS) { echo "skip This test requires streams support"; } ?>
<?php $needs = "2.5.5"; $needsOp = "lt" ?>
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$host = MongoShellServer::getStandaloneInfo();


function log_query($server, $query, $cursor_options) {
    var_dump($query);
}

$ctx = stream_context_create(
    array(
        "mongodb" => array(
            "log_query" => "log_query",
        )
    )
);
//stream_context_set_params($ctx, array("notification" => "stream_notification_callback", "notifications" => "stream_notification_callback"));

$mc = new MongoClient($host, array("fsync" => true), array("context" => $ctx));

echo "Fsync enabled by default\n";
$doc = array("doc" => "ument");
$mc->test->bug572->insert($doc);
$mc->test->bug572->update(array("_id" => $doc["_id"]), array("updated" => "doc"));
$mc->test->bug572->remove(array("_id" => $doc["_id"]));

echo "Setting it to false, per-query\n";
$doc = array("doc" => "ument");
$mc->test->bug572->insert($doc, array("fsync" => false));
$mc->test->bug572->update(array("_id" => $doc["_id"]), array("updated" => "doc"), array("fsync" => false));
$mc->test->bug572->remove(array("_id" => $doc["_id"]), array("fsync" => false));

echo "Setting it to false, per-query, and w=0 to force no-gle\n";
$doc = array("doc" => "ument");
$mc->test->bug572->insert($doc, array("fsync" => false, "w" => 0));
$mc->test->bug572->update(array("_id" => $doc["_id"]), array("updated" => "doc"), array("fsync" => false, "w" => 0));
$mc->test->bug572->remove(array("_id" => $doc["_id"]), array("fsync" => false, "w" => 0));

$mc = new MongoClient($host, array("fsync" => false), array("context" => $ctx));

echo "Fsync disabled by default\n";
$doc = array("doc" => "ument");
$mc->test->bug572->insert($doc);
$mc->test->bug572->update(array("_id" => $doc["_id"]), array("updated" => "doc"));
$mc->test->bug572->remove(array("_id" => $doc["_id"]));

echo "Setting it to true, per-query\n";
$doc = array("doc" => "ument");
$mc->test->bug572->insert($doc, array("fsync" => true));
$mc->test->bug572->update(array("_id" => $doc["_id"]), array("updated" => "doc"), array("fsync" => true));
$mc->test->bug572->remove(array("_id" => $doc["_id"]), array("fsync" => true));

$mc = new MongoClient($host, array("fsync" => false, "w" => 0), array("context" => $ctx));

echo "Fsync disabled by default, and gle\n";
$doc = array("doc" => "ument");
$mc->test->bug572->insert($doc);
$mc->test->bug572->update(array("_id" => $doc["_id"]), array("updated" => "doc"));
$mc->test->bug572->remove(array("_id" => $doc["_id"]));

echo "Setting it to true, per-query, with gle=0\n";
$doc = array("doc" => "ument");
$mc->test->bug572->insert($doc, array("fsync" => true));
$mc->test->bug572->update(array("_id" => $doc["_id"]), array("updated" => "doc"), array("fsync" => true));
$mc->test->bug572->remove(array("_id" => $doc["_id"]), array("fsync" => true));

?>
--EXPECTF--
Fsync enabled by default
array(2) {
  ["getlasterror"]=>
  int(1)
  ["fsync"]=>
  bool(true)
}
array(2) {
  ["getlasterror"]=>
  int(1)
  ["fsync"]=>
  bool(true)
}
array(2) {
  ["getlasterror"]=>
  int(1)
  ["fsync"]=>
  bool(true)
}
Setting it to false, per-query
array(1) {
  ["getlasterror"]=>
  int(1)
}
array(1) {
  ["getlasterror"]=>
  int(1)
}
array(1) {
  ["getlasterror"]=>
  int(1)
}
Setting it to false, per-query, and w=0 to force no-gle
Fsync disabled by default
array(1) {
  ["getlasterror"]=>
  int(1)
}
array(1) {
  ["getlasterror"]=>
  int(1)
}
array(1) {
  ["getlasterror"]=>
  int(1)
}
Setting it to true, per-query
array(2) {
  ["getlasterror"]=>
  int(1)
  ["fsync"]=>
  bool(true)
}
array(2) {
  ["getlasterror"]=>
  int(1)
  ["fsync"]=>
  bool(true)
}
array(2) {
  ["getlasterror"]=>
  int(1)
  ["fsync"]=>
  bool(true)
}
Fsync disabled by default, and gle
Setting it to true, per-query, with gle=0
array(2) {
  ["getlasterror"]=>
  int(1)
  ["fsync"]=>
  bool(true)
}
array(2) {
  ["getlasterror"]=>
  int(1)
  ["fsync"]=>
  bool(true)
}
array(2) {
  ["getlasterror"]=>
  int(1)
  ["fsync"]=>
  bool(true)
}

