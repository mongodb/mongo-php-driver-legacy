--TEST--
Test for PHP-506: Ensure Mongo constructor casts passwords to strings.
--FILE--
<?php
MongoLog::setLevel(MongoLog::ALL);
MongoLog::setModule(MongoLog::ALL);
$options = array('password' => 34534, 'username' => 'derickr', 'connect' => false);
$m = new MongoClient("mongodb://localhost", $options);
var_dump($options);
?>
--EXPECTF--
Notice: PARSE   INFO: Parsing %s in %sbug00506.php on line %d

Notice: PARSE   INFO: - Found node: localhost:27017 in %sbug00506.php on line %d

Notice: PARSE   INFO: - Connection type: STANDALONE in %sbug00506.php on line %d

Notice: PARSE   INFO: - Found option 'password': '34534' in %sbug00506.php on line %d

Notice: PARSE   INFO: - Found option 'username': 'derickr' in %sbug00506.php on line %d
array(3) {
  ["password"]=>
  int(34534)
  ["username"]=>
  string(7) "derickr"
  ["connect"]=>
  bool(false)
}
