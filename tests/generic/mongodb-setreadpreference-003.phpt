--TEST--
MongoDB::setReadPreference [3]
--SKIPIF--
<?php require_once dirname(__FILE__) ."/skipif.inc"; ?>
--FILE--
<?php require_once dirname(__FILE__) ."/skipif.inc"; ?>
<?php
$host = hostname();
$port = port();
$db   = dbname();

$baseString = sprintf("mongodb://%s:%d/%s", $host, $port, $db);

$a = array(
	/* no tagsets */
	array(),
	/* one tag set */
	array( array( 'dc' => 'east' ) ),
	array( array( 'dc' => 'east', 'use' => 'reporting' ) ),
	array( array() ),
	/* two tag sets */
	array( array( 'dc' => 'east', 'use' => 'reporting' ), array( 'dc' => 'west' ) ),
	/* two tag sets + empty one*/
	array( array( 'dc' => 'east', 'use' => 'reporting' ), array( 'dc' => 'west' ), array() ),
);

foreach ($a as $value) {
	$m = new mongo($baseString);
	$d = $m->phpunit;
	$d->setReadPreference(Mongo::RP_SECONDARY, $value);
	$rp = $d->getReadPreference();
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
      [0]=>
      string(7) "dc:east"
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
      [0]=>
      string(7) "dc:east"
      [1]=>
      string(13) "use:reporting"
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
      [0]=>
      string(7) "dc:east"
      [1]=>
      string(13) "use:reporting"
    }
    [1]=>
    array(1) {
      [0]=>
      string(7) "dc:west"
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
      [0]=>
      string(7) "dc:east"
      [1]=>
      string(13) "use:reporting"
    }
    [1]=>
    array(1) {
      [0]=>
      string(7) "dc:west"
    }
    [2]=>
    array(0) {
    }
  }
}
---
