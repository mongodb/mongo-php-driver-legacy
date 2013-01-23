--TEST--
MongoDB::setReadPreference() should set tags
--SKIPIF--
<?php require_once dirname(__FILE__) ."/skipif.inc"; ?>
--FILE--
<?php require_once dirname(__FILE__) . "/../utils.inc"; ?>
<?php

$a = array(
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

foreach ($a as $value) {
    $m = new_mongo();
    $db = $m->phpunit;
    $db->setReadPreference(Mongo::RP_SECONDARY, $value);
    $rp = $db->getReadPreference();
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
