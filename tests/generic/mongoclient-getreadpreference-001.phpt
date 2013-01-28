--TEST--
Mongo::getReadPreference [1]
--SKIPIF--
<?php require_once dirname(__FILE__) ."/skipif.inc"; ?>
--FILE--
<?php require_once dirname(__FILE__) ."/skipif.inc"; ?>
<?php
$host = hostname();
$port = port();
$db   = dbname();

$baseString = sprintf("mongodb://%s:%d/%s?readPreference=", $host, $port, $db);

$a = array(
    'primary',
    'secondary',
);

$b = array(
    '',
    '&readPreferenceTags=dc:west',
    '&readPreferenceTags=dc:west,use:reporting',
    '&readPreferenceTags=',
    '&readPreferenceTags=dc:west,use:reporting&readPreferenceTags=dc:east',
);

foreach ($a as $value) {
    foreach ($b as $tags) {
        $m = new mongo($baseString . $value . $tags, array( 'connect' => false ) );
        $rp = $m->getReadPreference();
        var_dump($rp);
        echo "---\n";
    }
}
?>
--EXPECT--
array(1) {
  ["type"]=>
  string(7) "primary"
}
---
array(2) {
  ["type"]=>
  string(7) "primary"
  ["tagsets"]=>
  array(1) {
    [0]=>
    array(1) {
      ["dc"]=>
      string(4) "west"
    }
  }
}
---
array(2) {
  ["type"]=>
  string(7) "primary"
  ["tagsets"]=>
  array(1) {
    [0]=>
    array(2) {
      ["dc"]=>
      string(4) "west"
      ["use"]=>
      string(9) "reporting"
    }
  }
}
---
array(2) {
  ["type"]=>
  string(7) "primary"
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
  string(7) "primary"
  ["tagsets"]=>
  array(2) {
    [0]=>
    array(2) {
      ["dc"]=>
      string(4) "west"
      ["use"]=>
      string(9) "reporting"
    }
    [1]=>
    array(1) {
      ["dc"]=>
      string(4) "east"
    }
  }
}
---
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
      string(4) "west"
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
      string(4) "west"
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
      string(4) "west"
      ["use"]=>
      string(9) "reporting"
    }
    [1]=>
    array(1) {
      ["dc"]=>
      string(4) "east"
    }
  }
}
---
