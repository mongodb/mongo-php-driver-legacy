--TEST--
Test for PHP-722: Unable to ensureIndex() for attributes with numeric name
--SKIPIF--
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

function log_insert($server, $document, $insert_options) {
    echo __METHOD__, "\n";
    var_dump($document);
}
function log_query($server, $query, $cursor_options) {
    echo __METHOD__, "\n";
    var_dump($query);
}

$ctx = stream_context_create(
    array(
        "mongodb" => array(
            "log_insert" => "log_insert",
            "log_query" => "log_query",
        )
    )
);
$dsn = MongoShellServer::getStandaloneInfo();
$mc = new MongoClient($dsn, array(), array("context" => $ctx));

try {
    $c = $mc->test->ensureindex->ensureIndex(array('3899' => 1));
    var_dump($c);
} catch(Exception $e) {
    var_dump(get_class($e), $e->getMessage(), $e->getCode());
}
try {
    $c = $mc->test->ensureindex->ensureIndex(array(38 => '1'));
    var_dump($c);
} catch(Exception $e) {
    var_dump(get_class($e), $e->getMessage(), $e->getCode());
}
try {
    $c = $mc->test->ensureindex->ensureIndex(array('38' => '1'));
    var_dump($c);
} catch(Exception $e) {
    var_dump(get_class($e), $e->getMessage(), $e->getCode());
}
try {
    $c = $mc->test->ensureindex->ensureIndex((object)array(38 => '1'));
    var_dump($c);
} catch(Exception $e) {
    var_dump(get_class($e), $e->getMessage(), $e->getCode());
}

try {
    $mc->test->bug722->insert(array('1' => "data", '3' => "additional"));
    $c = $mc->test->ensureindex->ensureIndex((object)array('3' => 1));
    var_dump($c);
} catch(Exception $e) {
    var_dump(get_class($e), $e->getMessage(), $e->getCode());
}
?>
--EXPECTF--
log_insert
array(4) {
  ["ns"]=>
  string(16) "test.ensureindex"
  ["key"]=>
  array(1) {
    [3899]=>
    int(1)
  }
  ["name"]=>
  string(2) "_1"
  ["_id"]=>
  object(MongoId)#%d (1) {
    ["$id"]=>
    string(24) "%s"
  }
}
log_query
array(1) {
  ["getlasterror"]=>
  int(1)
}
array(4) {
  ["n"]=>
  int(0)
  ["connectionId"]=>
  int(%d)
  ["err"]=>
  NULL
  ["ok"]=>
  float(1)
}
log_insert
array(4) {
  ["ns"]=>
  string(16) "test.ensureindex"
  ["key"]=>
  array(1) {
    [38]=>
    string(1) "1"
  }
  ["name"]=>
  string(2) "_1"
  ["_id"]=>
  object(MongoId)#%d (1) {
    ["$id"]=>
    string(24) "%s"
  }
}
log_query
array(1) {
  ["getlasterror"]=>
  int(1)
}
array(4) {
  ["n"]=>
  int(0)
  ["connectionId"]=>
  int(%d)
  ["err"]=>
  NULL
  ["ok"]=>
  float(1)
}
log_insert
array(4) {
  ["ns"]=>
  string(16) "test.ensureindex"
  ["key"]=>
  array(1) {
    [38]=>
    string(1) "1"
  }
  ["name"]=>
  string(2) "_1"
  ["_id"]=>
  object(MongoId)#%d (1) {
    ["$id"]=>
    string(24) "%s"
  }
}
log_query
array(1) {
  ["getlasterror"]=>
  int(1)
}
array(4) {
  ["n"]=>
  int(0)
  ["connectionId"]=>
  int(%d)
  ["err"]=>
  NULL
  ["ok"]=>
  float(1)
}
log_insert
array(4) {
  ["ns"]=>
  string(16) "test.ensureindex"
  ["key"]=>
  object(stdClass)#%d (1) {
    [38]=>
    string(1) "1"
  }
  ["name"]=>
  string(2) "_1"
  ["_id"]=>
  object(MongoId)#%d (1) {
    ["$id"]=>
    string(24) "%s"
  }
}
log_query
array(1) {
  ["getlasterror"]=>
  int(1)
}
array(4) {
  ["n"]=>
  int(0)
  ["connectionId"]=>
  int(%d)
  ["err"]=>
  NULL
  ["ok"]=>
  float(1)
}
log_insert
array(3) {
  [1]=>
  string(4) "data"
  [3]=>
  string(10) "additional"
  ["_id"]=>
  object(MongoId)#%d (1) {
    ["$id"]=>
    string(24) "%s"
  }
}
log_query
array(1) {
  ["getlasterror"]=>
  int(1)
}
log_insert
array(4) {
  ["ns"]=>
  string(16) "test.ensureindex"
  ["key"]=>
  object(stdClass)#%d (1) {
    [3]=>
    int(1)
  }
  ["name"]=>
  string(2) "_1"
  ["_id"]=>
  object(MongoId)#%d (1) {
    ["$id"]=>
    string(24) "%s"
  }
}
log_query
array(1) {
  ["getlasterror"]=>
  int(1)
}
array(4) {
  ["n"]=>
  int(0)
  ["connectionId"]=>
  int(%d)
  ["err"]=>
  NULL
  ["ok"]=>
  float(1)
}
