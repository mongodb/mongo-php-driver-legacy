--TEST--
Test for PHP-1077: Option handling for createIndexes command
--SKIPIF--
<?php $needs = "2.6.0"; $needsOp = "ge"; ?>
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
function log_query($server, $query, $info)
{
    if ( ! isset($query['createIndexes'])) {
        return;
    }

    var_dump($query);
}

$ctx = stream_context_create(
    array(
        'mongodb' => array( 'log_query' => 'log_query',)
    )
);

require_once 'tests/utils/server.inc';

$host = MongoShellServer::getStandaloneInfo();
$mc = new MongoClient($host, array(), array('context' => $ctx));

$collection = $mc->selectCollection(dbname(), collname(__FILE__));
$collection->drop();

// The "maxTimeMS" option should be placed on the top-level command document
$collection->createIndex(
    array('a' => 1),
    array('maxTimeMS' => 12345)
);

// Write concern options should not appear in the command document.
$collection->createIndex(
    array('b' => 1),
    array('fsync' => true)
);

$collection->createIndex(
    array('c' => 1),
    array('j' => true)
);

$collection->createIndex(
    array('d' => 1),
    array('safe' => true)
);

$collection->createIndex(
    array('e' => 1),
    array('w' => 1)
);

$collection->createIndex(
    array('f' => 1),
    array('wTimeoutMS' => 12345)
);

$collection->createIndex(
    array('g' => 1),
    array('wtimeout' => 12345)
);

// Driver-side timeout options should not appear in the command document.
$collection->createIndex(
    array('h' => 1),
    array('socketTimeoutMS' => 12345)
);

$collection->createIndex(
    array('i' => 1),
    array('timeout' => 12345)
);

?>
===DONE===
<?php exit(0); ?>
--EXPECTF--
array(3) {
  ["createIndexes"]=>
  string(%d) "%s"
  ["indexes"]=>
  array(1) {
    [0]=>
    array(2) {
      ["key"]=>
      array(1) {
        ["a"]=>
        int(1)
      }
      ["name"]=>
      string(3) "a_1"
    }
  }
  ["maxTimeMS"]=>
  int(12345)
}
array(2) {
  ["createIndexes"]=>
  string(%d) "%s"
  ["indexes"]=>
  array(1) {
    [0]=>
    array(2) {
      ["key"]=>
      array(1) {
        ["b"]=>
        int(1)
      }
      ["name"]=>
      string(3) "b_1"
    }
  }
}
array(2) {
  ["createIndexes"]=>
  string(%d) "%s"
  ["indexes"]=>
  array(1) {
    [0]=>
    array(2) {
      ["key"]=>
      array(1) {
        ["c"]=>
        int(1)
      }
      ["name"]=>
      string(3) "c_1"
    }
  }
}
array(2) {
  ["createIndexes"]=>
  string(%d) "%s"
  ["indexes"]=>
  array(1) {
    [0]=>
    array(2) {
      ["key"]=>
      array(1) {
        ["d"]=>
        int(1)
      }
      ["name"]=>
      string(3) "d_1"
    }
  }
}
array(2) {
  ["createIndexes"]=>
  string(%d) "%s"
  ["indexes"]=>
  array(1) {
    [0]=>
    array(2) {
      ["key"]=>
      array(1) {
        ["e"]=>
        int(1)
      }
      ["name"]=>
      string(3) "e_1"
    }
  }
}
array(2) {
  ["createIndexes"]=>
  string(%d) "%s"
  ["indexes"]=>
  array(1) {
    [0]=>
    array(2) {
      ["key"]=>
      array(1) {
        ["f"]=>
        int(1)
      }
      ["name"]=>
      string(3) "f_1"
    }
  }
}
array(2) {
  ["createIndexes"]=>
  string(%d) "%s"
  ["indexes"]=>
  array(1) {
    [0]=>
    array(2) {
      ["key"]=>
      array(1) {
        ["g"]=>
        int(1)
      }
      ["name"]=>
      string(3) "g_1"
    }
  }
}
array(2) {
  ["createIndexes"]=>
  string(%d) "%s"
  ["indexes"]=>
  array(1) {
    [0]=>
    array(2) {
      ["key"]=>
      array(1) {
        ["h"]=>
        int(1)
      }
      ["name"]=>
      string(3) "h_1"
    }
  }
}

Deprecated: MongoCollection::createIndex(): The 'timeout' option is deprecated, please use 'socketTimeoutMS' instead in %s on line %d
array(2) {
  ["createIndexes"]=>
  string(%d) "%s"
  ["indexes"]=>
  array(1) {
    [0]=>
    array(2) {
      ["key"]=>
      array(1) {
        ["i"]=>
        int(1)
      }
      ["name"]=>
      string(3) "i_1"
    }
  }
}
===DONE===
