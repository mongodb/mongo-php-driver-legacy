--TEST--
MongoCommandCursor::getReadPreference() returns read preferences
--SKIPIF--
<?php require_once __DIR__ . "/skipif.inc"; ?>
--FILE--
<?php

$modes = array(
    MongoClient::RP_PRIMARY,
    MongoClient::RP_SECONDARY,
);

$tagParams = array(
    '',
    '&readPreferenceTags=dc:west',
    '&readPreferenceTags=dc:west,use:reporting',
    '&readPreferenceTags=',
    '&readPreferenceTags=dc:west,use:reporting&readPreferenceTags=dc:east',
);

foreach ($modes as $mode) {
    foreach ($tagParams as $tagParam) {
        $uri = sprintf('mongodb://localhost:27017/?readPreference=%s%s', $mode, $tagParam);
        $mc = new MongoClient($uri, array('connect' => false));
        $cc = new MongoCommandCursor($mc, 'test.foo', array());
        var_dump($cc->getReadPreference());
        echo "---\n";
    }
}

?>
===DONE===
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
===DONE===
