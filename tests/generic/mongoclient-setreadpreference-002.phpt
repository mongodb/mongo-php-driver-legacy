--TEST--
MongoClient::setReadPreference() should set tags
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php require_once "tests/utils/server.inc"; ?>
<?php

$tagsets = array(
    /* no tagsets */
    array(),
    /* one tag set */
    array( array( 'dc' => 'east' ) ),
    array( array( 'dc' => 'east', 'use' => 'reporting' ) ),
    array( array() ),
    /* two tag sets */
    array( array( 'dc' => 'east', 'use' => 'reporting' ), array( 'dc' => 'west' ) ),
    /* two tag sets + empty one */
    array( array( 'dc' => 'east', 'use' => 'reporting' ), array( 'dc' => 'west' ), array() ),
);

foreach ($tagsets as $tagset) {
    $m = new_mongo_standalone();
    $m->setReadPreference(Mongo::RP_SECONDARY, $tagset);
    $rp = $m->getReadPreference();
    var_dump($rp);

    echo "---\n";
}
?>
--EXPECT--
array(1) {
  ["type"]=>
  string(9) "secondary"
}
---
array(2) {
  ["type"]=>
  string(9) "secondary"
  ["tagsets"]=>
  array(1) {
    [0]=>
    array(1) {
      ["dc"]=>
      string(4) "east"
    }
  }
}
---
array(2) {
  ["type"]=>
  string(9) "secondary"
  ["tagsets"]=>
  array(1) {
    [0]=>
    array(2) {
      ["dc"]=>
      string(4) "east"
      ["use"]=>
      string(9) "reporting"
    }
  }
}
---
array(2) {
  ["type"]=>
  string(9) "secondary"
  ["tagsets"]=>
  array(1) {
    [0]=>
    array(0) {
    }
  }
}
---
array(2) {
  ["type"]=>
  string(9) "secondary"
  ["tagsets"]=>
  array(2) {
    [0]=>
    array(2) {
      ["dc"]=>
      string(4) "east"
      ["use"]=>
      string(9) "reporting"
    }
    [1]=>
    array(1) {
      ["dc"]=>
      string(4) "west"
    }
  }
}
---
array(2) {
  ["type"]=>
  string(9) "secondary"
  ["tagsets"]=>
  array(3) {
    [0]=>
    array(2) {
      ["dc"]=>
      string(4) "east"
      ["use"]=>
      string(9) "reporting"
    }
    [1]=>
    array(1) {
      ["dc"]=>
      string(4) "west"
    }
    [2]=>
    array(0) {
    }
  }
}
---
