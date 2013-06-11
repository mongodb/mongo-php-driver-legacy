--TEST--
Test for PHP-735: Commands should inherit read preferences from MongoDB and MongoCollection objects
--SKIPIF--
<?php if (!MONGO_STREAMS) { echo "skip This test requires streams support"; } ?>
<?php if (!version_compare(phpversion(), "5.3", '>=')) exit("skip >= PHP 5.3 needed\n"); ?>
<?php require_once "tests/utils/mongos.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

function log_query($server, $query, $cursor_options) {
    var_dump($query);
}
MongoLog::setLevel(MongoLog::ALL);
MongoLog::setModule(MongoLog::ALL);
MongoLog::setCallback(function($module, $level, $msg) {
    if (strpos($msg, "command supports") !== false) {
        echo $msg, "\n";
        return;
    }
    if (strpos($msg, "forcing") !== false) {
        echo $msg, "\n";
        return;
    }
});


$ctx = stream_context_create(
    array(
        "mongodb" => array(
            "log_query" => "log_query",
        )
    )
);

$cfg = MongoShellServer::getShardInfo();
$mc = new MongoClient($cfg[0], array("w" => 2, "readPreference" => MongoClient::RP_NEAREST), array("context" => $ctx));

$db = $mc->selectDB(dbname());
$collection = $db->collection;

echo "===This should send 'nearest'===\n";
$db->command(array("distinct" => "people", "key" => "age"));
var_dump($db->getReadPreference());

echo "===These should send 'secondary'===\n";
$db->setReadPreference(MongoClient::RP_SECONDARY);
$db->command(array("distinct" => "people", "key" => "age"));
$db->collection->aggregate(array());
var_dump($db->getReadPreference(), $db->collection->getReadPreference());

echo "===These should send 'primaryPreferred'===\n";
$db->setReadPreference(MongoClient::RP_PRIMARY_PREFERRED);
$db->collection->aggregate(array());
$db->collection->count();
$db->collection->find()->count();
$db->collection->find()->explain();
var_dump($db->getReadPreference(), $db->collection->getReadPreference());


echo "===This should send 'nearest'===\n";
$collection->count(array());
$collection->aggregate(array());
$collection->count(array());
var_dump($collection->getReadPreference());

echo "===These should send 'secondary'===\n";
$collection->setReadPreference(MongoClient::RP_SECONDARY);
$collection->aggregate(array());
$collection->count();
$collection->find()->count();
$collection->find()->explain();
var_dump($collection->getReadPreference());

echo "===These should send 'primaryPreferred'===\n";
$collection->setReadPreference(MongoClient::RP_PRIMARY_PREFERRED);
$collection->aggregate(array());
$collection->find()->count();
$collection->find()->explain();
var_dump($collection->getReadPreference());

?>
--EXPECTF--
===This should send 'nearest'===
command supports Read Preferences
array(2) {
  ["$query"]=>
  array(2) {
    ["distinct"]=>
    string(6) "people"
    ["key"]=>
    string(3) "age"
  }
  ["$readPreference"]=>
  array(1) {
    ["mode"]=>
    string(7) "nearest"
  }
}
array(1) {
  ["type"]=>
  string(7) "nearest"
}
===These should send 'secondary'===
command supports Read Preferences
array(2) {
  ["$query"]=>
  array(2) {
    ["distinct"]=>
    string(6) "people"
    ["key"]=>
    string(3) "age"
  }
  ["$readPreference"]=>
  array(1) {
    ["mode"]=>
    string(9) "secondary"
  }
}
command supports Read Preferences
array(2) {
  ["$query"]=>
  array(2) {
    ["aggregate"]=>
    string(10) "collection"
    ["pipeline"]=>
    array(1) {
      [0]=>
      array(0) {
      }
    }
  }
  ["$readPreference"]=>
  array(1) {
    ["mode"]=>
    string(9) "secondary"
  }
}
array(1) {
  ["type"]=>
  string(9) "secondary"
}
array(1) {
  ["type"]=>
  string(9) "secondary"
}
===These should send 'primaryPreferred'===
command supports Read Preferences
array(2) {
  ["$query"]=>
  array(2) {
    ["aggregate"]=>
    string(10) "collection"
    ["pipeline"]=>
    array(1) {
      [0]=>
      array(0) {
      }
    }
  }
  ["$readPreference"]=>
  array(1) {
    ["mode"]=>
    string(16) "primaryPreferred"
  }
}
command supports Read Preferences
array(2) {
  ["$query"]=>
  array(1) {
    ["count"]=>
    string(10) "collection"
  }
  ["$readPreference"]=>
  array(1) {
    ["mode"]=>
    string(16) "primaryPreferred"
  }
}
command supports Read Preferences
array(2) {
  ["$query"]=>
  array(2) {
    ["count"]=>
    string(10) "collection"
    ["query"]=>
    object(stdClass)#%d (0) {
    }
  }
  ["$readPreference"]=>
  array(1) {
    ["mode"]=>
    string(16) "primaryPreferred"
  }
}
array(3) {
  ["$query"]=>
  object(stdClass)#%d (0) {
  }
  ["$explain"]=>
  bool(true)
  ["$readPreference"]=>
  array(1) {
    ["mode"]=>
    string(16) "primaryPreferred"
  }
}
array(1) {
  ["type"]=>
  string(16) "primaryPreferred"
}
array(1) {
  ["type"]=>
  string(16) "primaryPreferred"
}
===This should send 'nearest'===
command supports Read Preferences
array(2) {
  ["$query"]=>
  array(2) {
    ["count"]=>
    string(10) "collection"
    ["query"]=>
    array(0) {
    }
  }
  ["$readPreference"]=>
  array(1) {
    ["mode"]=>
    string(7) "nearest"
  }
}
command supports Read Preferences
array(2) {
  ["$query"]=>
  array(2) {
    ["aggregate"]=>
    string(10) "collection"
    ["pipeline"]=>
    array(1) {
      [0]=>
      array(0) {
      }
    }
  }
  ["$readPreference"]=>
  array(1) {
    ["mode"]=>
    string(7) "nearest"
  }
}
command supports Read Preferences
array(2) {
  ["$query"]=>
  array(2) {
    ["count"]=>
    string(10) "collection"
    ["query"]=>
    array(0) {
    }
  }
  ["$readPreference"]=>
  array(1) {
    ["mode"]=>
    string(7) "nearest"
  }
}
array(1) {
  ["type"]=>
  string(7) "nearest"
}
===These should send 'secondary'===
command supports Read Preferences
array(2) {
  ["$query"]=>
  array(2) {
    ["aggregate"]=>
    string(10) "collection"
    ["pipeline"]=>
    array(1) {
      [0]=>
      array(0) {
      }
    }
  }
  ["$readPreference"]=>
  array(1) {
    ["mode"]=>
    string(9) "secondary"
  }
}
command supports Read Preferences
array(2) {
  ["$query"]=>
  array(1) {
    ["count"]=>
    string(10) "collection"
  }
  ["$readPreference"]=>
  array(1) {
    ["mode"]=>
    string(9) "secondary"
  }
}
command supports Read Preferences
array(2) {
  ["$query"]=>
  array(2) {
    ["count"]=>
    string(10) "collection"
    ["query"]=>
    object(stdClass)#%d (0) {
    }
  }
  ["$readPreference"]=>
  array(1) {
    ["mode"]=>
    string(9) "secondary"
  }
}
array(3) {
  ["$query"]=>
  object(stdClass)#%d (0) {
  }
  ["$explain"]=>
  bool(true)
  ["$readPreference"]=>
  array(1) {
    ["mode"]=>
    string(9) "secondary"
  }
}
array(1) {
  ["type"]=>
  string(9) "secondary"
}
===These should send 'primaryPreferred'===
command supports Read Preferences
array(2) {
  ["$query"]=>
  array(2) {
    ["aggregate"]=>
    string(10) "collection"
    ["pipeline"]=>
    array(1) {
      [0]=>
      array(0) {
      }
    }
  }
  ["$readPreference"]=>
  array(1) {
    ["mode"]=>
    string(16) "primaryPreferred"
  }
}
command supports Read Preferences
array(2) {
  ["$query"]=>
  array(2) {
    ["count"]=>
    string(10) "collection"
    ["query"]=>
    object(stdClass)#%d (0) {
    }
  }
  ["$readPreference"]=>
  array(1) {
    ["mode"]=>
    string(16) "primaryPreferred"
  }
}
array(3) {
  ["$query"]=>
  object(stdClass)#%d (0) {
  }
  ["$explain"]=>
  bool(true)
  ["$readPreference"]=>
  array(1) {
    ["mode"]=>
    string(16) "primaryPreferred"
  }
}
array(1) {
  ["type"]=>
  string(16) "primaryPreferred"
}

