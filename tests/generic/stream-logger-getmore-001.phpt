--TEST--
Stream Logger: log_getmore() (1)
--SKIPIF--
<?php if (!MONGO_STREAMS) { echo "skip This test requires streams support"; } ?>
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$dsn = MongoShellServer::getStandaloneInfo();


function log_getmore($server, $cursor_options) {
    echo __METHOD__, "\n";

    var_dump($server, $cursor_options);
}


$ctx = stream_context_create(
    array(
        "mongodb" => array(
            "log_getmore" => "log_getmore",
        )
    )
);

$mc = new MongoClient($dsn, array(), array("context" => $ctx));

foreach(range(0, 200) as $i) {
    $newdoc = array("example" => "document", "with" => "some", "fields" => "in it", "rand" => $i);
    $mc->phpunit->jobs->insert($newdoc);
}
$cursor = $mc->phpunit->jobs->find();
$i = 0;
foreach($cursor as $doc) {
    if (++$i == 101) {
        echo "Now I should getmore\n";
    } elseif ($i == 102) {
        echo "There should be a getmore query above\n";
    }
}
$cursor = $mc->phpunit->jobs->drop();


?>
--EXPECTF--
Now I should getmore
log_getmore
array(5) {
  ["hash"]=>
  string(%d) "%s:%d;-;.;%d"
  ["type"]=>
  int(1)
  ["max_bson_size"]=>
  int(16777216)
  ["max_message_size"]=>
  int(%d)
  ["request_id"]=>
  int(%d)
}
array(2) {
  ["request_id"]=>
  int(%d)
  ["cursor_id"]=>
  int(%d)
}
There should be a getmore query above
