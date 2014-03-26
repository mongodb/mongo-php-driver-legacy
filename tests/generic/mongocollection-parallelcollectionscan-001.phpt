--TEST--
MongoConnection::parallelCollectionScan()
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
var_dump($document);

foreach($document["cursors"] as $retval) {
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
array(2) {
  ["cursors"]=>
  array(4) {
    [0]=>
    array(2) {
      ["cursor"]=>
      array(3) {
        ["firstBatch"]=>
        array(0) {
        }
        ["ns"]=>
        string(%d) "%s"
        ["id"]=>
        object(MongoInt64)#%d (1) {
          ["value"]=>
          string(%d) "%d"
        }
      }
      ["ok"]=>
      bool(true)
    }
    [1]=>
    array(2) {
      ["cursor"]=>
      array(3) {
        ["firstBatch"]=>
        array(0) {
        }
        ["ns"]=>
        string(%d) "%s"
        ["id"]=>
        object(MongoInt64)#%d (1) {
          ["value"]=>
          string(%d) "%d"
        }
      }
      ["ok"]=>
      bool(true)
    }
    [2]=>
    array(2) {
      ["cursor"]=>
      array(3) {
        ["firstBatch"]=>
        array(0) {
        }
        ["ns"]=>
        string(%d) "%s"
        ["id"]=>
        object(MongoInt64)#%d (1) {
          ["value"]=>
          string(%d) "%d"
        }
      }
      ["ok"]=>
      bool(true)
    }
    [3]=>
    array(2) {
      ["cursor"]=>
      array(3) {
        ["firstBatch"]=>
        array(0) {
        }
        ["ns"]=>
        string(%d) "%s"
        ["id"]=>
        object(MongoInt64)#%d (1) {
          ["value"]=>
          string(%d) "%d"
        }
      }
      ["ok"]=>
      bool(true)
    }
  }
  ["ok"]=>
  float(1)
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
===DONE==
