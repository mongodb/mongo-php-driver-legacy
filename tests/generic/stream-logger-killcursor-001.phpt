--TEST--
Stream Logger: log_killcursor() (1)
--SKIPIF--
<?php if (!MONGO_STREAMS) { echo "skip This test requires streams support"; } ?>
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$dsn = MongoShellServer::getStandaloneInfo();


function log_killcursor($server, $cursor_options) {
    echo __METHOD__, "\n";

    var_dump($server, $cursor_options);
}

$ctx = stream_context_create(
    array(
        "mongodb" => array(
            "log_killcursor" => "log_killcursor",
        )
    )
);

$mc = new MongoClient($dsn, array(), array("context" => $ctx));

foreach(range(0, 200) as $i) {
    $newdoc = array("example" => "document", "with" => "some", "fields" => "in it", "rand" => $i);
    $mc->phpunit->jobs->insert($newdoc, array("w" => 0));
}

$cursor = $mc->phpunit->jobs->find();
$i = 0;
foreach($cursor as $doc) {
    if (++$i == 100) {
        // Stop before the getmore so we have an unfinished cursor
        break;
    }
}
// Trigger kill cursor
$cursor = null;
echo "Cursor unset\n";

$cursor = $mc->phpunit->jobs->drop();


?>
--EXPECTF--
log_killcursor
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
array(1) {
  ["cursor_id"]=>
  int(%d)
}
Cursor unset
