--TEST--
MongoClient::__construct(): Connecting to one mongos (1)
--SKIPIF--
<?php if (!MONGO_STREAMS) { echo "skip This test requires streams support"; } ?>
<?php $needs = "2.5.5"; $needsOp = "lt" ?>
<?php require_once "tests/utils/mongos.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$cfg = MongoShellServer::getShardInfo();



function log_insert($server, $document, $insert_options) {
    global $newdoc;

    echo __METHOD__, "\n";
    var_dump($server);
    var_dump($newdoc == $document);
    var_dump($insert_options);

    echo "\n\n";
}
function log_query($server, $query, $cursor_options) {
    echo __METHOD__, "\n";

    var_dump($server, $query, $cursor_options);
    echo "\n\n";
}
function log_update($server, $criteria, $newobj, $insert_options, $cursor_options) {
    echo __METHOD__, "\n";

    var_dump($server);
    var_dump($criteria, $newobj, $insert_options, $cursor_options);
    echo "\n\n";
}
function log_delete($server, $criteria, $insert_options, $cursor_options) {
    echo __METHOD__, "\n";

    var_dump($server);
    var_dump($criteria, $insert_options, $cursor_options);
    echo "\n\n";
}



$ctx = stream_context_create(
    array(
        "mongodb" => array(
            "log_insert" => "log_insert",
            "log_query" => "log_query",
            "log_update" => "log_update",
            "log_delete" => "log_delete",
        )
    )
);
//stream_context_set_params($ctx, array("notification" => "stream_notification_callback", "notifications" => "stream_notification_callback"));

$mc = new MongoClient($cfg[0], array("readPreference" => MongoClient::RP_SECONDARY, "w" => 2), array("context" => $ctx));

$newdoc = array("example" => "document", "with" => "some", "fields" => "in it");
$mc->phpunit->jobs->insert($newdoc);
$mc->phpunit->jobs->find(array("_id" => $newdoc["_id"]))->count();
$obj = $mc->phpunit->jobs->findOne(array("_id" => $newdoc["_id"]), array("with"));
$obj["x"] = time();

$mc->phpunit->jobs->update(array("_id" => $obj["_id"]), $obj, array("w" => 1));
$mc->phpunit->jobs->remove(array("_id" => $obj["_id"]));

?>
--EXPECTF--
log_insert
array(5) {
  ["hash"]=>
  string(%d) "%s:%d;-;.;%d"
  ["type"]=>
  int(16)
  ["max_bson_size"]=>
  int(16777216)
  ["max_message_size"]=>
  int(%d)
  ["request_id"]=>
  int(%d)
}
bool(true)
NULL


log_query
array(5) {
  ["hash"]=>
  string(%d) "%s:%d;-;.;%d"
  ["type"]=>
  int(16)
  ["max_bson_size"]=>
  int(16777216)
  ["max_message_size"]=>
  int(%d)
  ["request_id"]=>
  int(%d)
}
array(3) {
  ["getlasterror"]=>
  int(1)
  ["w"]=>
  int(2)
  ["wtimeout"]=>
  int(10000)
}
array(5) {
  ["request_id"]=>
  int(4)
  ["skip"]=>
  int(0)
  ["limit"]=>
  int(-1)
  ["options"]=>
  int(0)
  ["cursor_id"]=>
  int(0)
}


log_query
array(5) {
  ["hash"]=>
  string(%d) "%s:%d;-;.;%d"
  ["type"]=>
  int(16)
  ["max_bson_size"]=>
  int(16777216)
  ["max_message_size"]=>
  int(%d)
  ["request_id"]=>
  int(%d)
}
array(2) {
  ["$query"]=>
  array(2) {
    ["count"]=>
    string(4) "jobs"
    ["query"]=>
    array(1) {
      ["_id"]=>
      object(MongoId)#%d (1) {
        ["$id"]=>
        string(24) "%s"
      }
    }
  }
  ["$readPreference"]=>
  array(1) {
    ["mode"]=>
    string(9) "secondary"
  }
}
array(5) {
  ["request_id"]=>
  int(5)
  ["skip"]=>
  int(0)
  ["limit"]=>
  int(-1)
  ["options"]=>
  int(4)
  ["cursor_id"]=>
  int(0)
}


log_query
array(5) {
  ["hash"]=>
  string(%d) "%s:%d;-;.;%d"
  ["type"]=>
  int(16)
  ["max_bson_size"]=>
  int(16777216)
  ["max_message_size"]=>
  int(%d)
  ["request_id"]=>
  int(%d)
}
array(2) {
  ["$query"]=>
  array(1) {
    ["_id"]=>
    object(MongoId)#%d (1) {
      ["$id"]=>
      string(24) "%s"
    }
  }
  ["$readPreference"]=>
  array(1) {
    ["mode"]=>
    string(9) "secondary"
  }
}
array(5) {
  ["request_id"]=>
  int(6)
  ["skip"]=>
  int(0)
  ["limit"]=>
  int(-1)
  ["options"]=>
  int(4)
  ["cursor_id"]=>
  int(0)
}


log_update
array(5) {
  ["hash"]=>
  string(%d) "%s:%d;-;.;%d"
  ["type"]=>
  int(16)
  ["max_bson_size"]=>
  int(16777216)
  ["max_message_size"]=>
  int(%d)
  ["request_id"]=>
  int(%d)
}
array(1) {
  ["_id"]=>
  object(MongoId)#%d (1) {
    ["$id"]=>
    string(24) "%s"
  }
}
array(3) {
  ["_id"]=>
  object(MongoId)#%d (1) {
    ["$id"]=>
    string(24) "%s"
  }
  ["with"]=>
  string(4) "some"
  ["x"]=>
  int(%d)
}
array(1) {
  ["w"]=>
  int(1)
}
array(2) {
  ["namespace"]=>
  string(12) "phpunit.jobs"
  ["flags"]=>
  int(0)
}


log_query
array(5) {
  ["hash"]=>
  string(%d) "%s:%d;-;.;%d"
  ["type"]=>
  int(16)
  ["max_bson_size"]=>
  int(16777216)
  ["max_message_size"]=>
  int(%d)
  ["request_id"]=>
  int(%d)
}
array(1) {
  ["getlasterror"]=>
  int(1)
}
array(5) {
  ["request_id"]=>
  int(8)
  ["skip"]=>
  int(0)
  ["limit"]=>
  int(-1)
  ["options"]=>
  int(0)
  ["cursor_id"]=>
  int(0)
}


log_delete
array(5) {
  ["hash"]=>
  string(%d) "%s:%d;-;.;%d"
  ["type"]=>
  int(16)
  ["max_bson_size"]=>
  int(16777216)
  ["max_message_size"]=>
  int(%d)
  ["request_id"]=>
  int(%d)
}
array(1) {
  ["_id"]=>
  object(MongoId)#%d (1) {
    ["$id"]=>
    string(24) "%s"
  }
}
array(0) {
}
array(2) {
  ["namespace"]=>
  string(12) "phpunit.jobs"
  ["flags"]=>
  int(0)
}


log_query
array(5) {
  ["hash"]=>
  string(%d) "%s:%d;-;.;%d"
  ["type"]=>
  int(16)
  ["max_bson_size"]=>
  int(16777216)
  ["max_message_size"]=>
  int(%d)
  ["request_id"]=>
  int(%d)
}
array(3) {
  ["getlasterror"]=>
  int(1)
  ["w"]=>
  int(2)
  ["wtimeout"]=>
  int(10000)
}
array(5) {
  ["request_id"]=>
  int(10)
  ["skip"]=>
  int(0)
  ["limit"]=>
  int(-1)
  ["options"]=>
  int(0)
  ["cursor_id"]=>
  int(0)
}


