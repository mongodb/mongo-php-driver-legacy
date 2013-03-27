--TEST--
Stream Logger: log_batchinsert() #001
--SKIPIF--
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$dsn = MongoShellServer::getStandaloneInfo();


function log_batchinsert($server, $docs, $info, $options) {
    echo __METHOD__, "\n";

    var_dump($server, count($docs), $info, $options);
}


$ctx = stream_context_create(
    array(
        "mongodb" => array(
            "log_batchinsert" => "log_batchinsert",
        )
    )
);

$mc = new MongoClient($dsn, array(), array("context" => $ctx));

$docs = array();
foreach(range(0, 200) as $i) {
    $docs[] = array("example" => "document", "with" => "some", "fields" => "in it", "rand" => $i);
}
$mc->phpunit->jobs->batchinsert($docs, array("continueOnError" => 2));
$cursor = $mc->phpunit->jobs->drop();


?>
--EXPECTF--
log_batchinsert
array(5) {
  ["hash"]=>
  string(%d) "%s:%d;-;X;%d"
  ["type"]=>
  int(1)
  ["max_bson_size"]=>
  int(16777216)
  ["max_message_size"]=>
  int(48000000)
  ["request_id"]=>
  int(%d)
}
int(201)
array(1) {
  ["flags"]=>
  int(2)
}
array(1) {
  ["continueOnError"]=>
  int(2)
}
