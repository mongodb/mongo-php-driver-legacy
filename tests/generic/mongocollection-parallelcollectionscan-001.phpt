--TEST--
MongoConnection::parallelCollectionScan()
--SKIPIF--
<?php $needs = "2.5.5"; $engine = 'mmapv1'; ?>
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$dsn = MongoShellServer::getStandaloneInfo();
$mc = new MongoClient($dsn);
$c = $mc->selectCollection(dbname(), collname(__FILE__));
$c->drop();

foreach (range(1,5000) as $x) {
    $c->insert(array('x' => $x));
}

$document = $c->parallelCollectionScan(4);
var_dump($document);

foreach($document as $cursor) {
    var_dump($cursor);

    foreach($cursor as $entry) {
    }
    var_dump($entry);
}

$document = $mc->selectDb(dbname())->command(array("parallelCollectionScan" => collname(__FILE__), "numCursors" => 4), null, $hash);

foreach($document["cursors"] as $key => $retval) {
	echo "Cursor {$key}:\n";
	dump_these_keys($retval['cursor'], array('ns', 'firstBatch'));
    $cursor = MongoCommandCursor::createFromDocument($mc, $hash, $retval);
    var_dump($cursor);

    foreach($cursor as $entry) {
    }
    var_dump($entry);
}

?>
===DONE==
<?php exit(0) ?>
--EXPECTF--
array(4) {
  [0]=>
  object(MongoCommandCursor)#%d (0) {
  }
  [1]=>
  object(MongoCommandCursor)#%d (0) {
  }
  [2]=>
  object(MongoCommandCursor)#%d (0) {
  }
  [3]=>
  object(MongoCommandCursor)#%d (0) {
  }
}
object(MongoCommandCursor)#%d (0) {
}
array(2) {
  ["_id"]=>
  object(MongoId)#%d (1) {
    ["$id"]=>
    string(24) "%s"
  }
  ["x"]=>
  int(125)
}
object(MongoCommandCursor)#%d (0) {
}
array(2) {
  ["_id"]=>
  object(MongoId)#%d (1) {
    ["$id"]=>
    string(24) "%s"
  }
  ["x"]=>
  int(634)
}
object(MongoCommandCursor)#%d (0) {
}
array(2) {
  ["_id"]=>
  object(MongoId)#%d (1) {
    ["$id"]=>
    string(24) "%s"
  }
  ["x"]=>
  int(2679)
}
object(MongoCommandCursor)#%d (0) {
}
array(2) {
  ["_id"]=>
  object(MongoId)#%d (1) {
    ["$id"]=>
    string(24) "%s"
  }
  ["x"]=>
  int(5000)
}
Cursor 0:
array(2) {
  ["ns"]=>
  string(55) "test.generic/mongocollection-parallelcollectionscan-001"
  ["firstBatch"]=>
  array(0) {
  }
}
object(MongoCommandCursor)#10 (0) {
}
array(2) {
  ["_id"]=>
  object(MongoId)#%d (1) {
    ["$id"]=>
    string(24) "%s"
  }
  ["x"]=>
  int(125)
}
Cursor 1:
array(2) {
  ["ns"]=>
  string(55) "test.generic/mongocollection-parallelcollectionscan-001"
  ["firstBatch"]=>
  array(0) {
  }
}
object(MongoCommandCursor)#11 (0) {
}
array(2) {
  ["_id"]=>
  object(MongoId)#%d (1) {
    ["$id"]=>
    string(24) "%s"
  }
  ["x"]=>
  int(634)
}
Cursor 2:
array(2) {
  ["ns"]=>
  string(55) "test.generic/mongocollection-parallelcollectionscan-001"
  ["firstBatch"]=>
  array(0) {
  }
}
object(MongoCommandCursor)#10 (0) {
}
array(2) {
  ["_id"]=>
  object(MongoId)#%d (1) {
    ["$id"]=>
    string(24) "%s"
  }
  ["x"]=>
  int(2679)
}
Cursor 3:
array(2) {
  ["ns"]=>
  string(55) "test.generic/mongocollection-parallelcollectionscan-001"
  ["firstBatch"]=>
  array(0) {
  }
}
object(MongoCommandCursor)#11 (0) {
}
array(2) {
  ["_id"]=>
  object(MongoId)#%d (1) {
    ["$id"]=>
    string(24) "%s"
  }
  ["x"]=>
  int(5000)
}
===DONE==
