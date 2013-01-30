--TEST--
Mongo::__construct() and $options
--SKIPIF--
<?php require dirname(__FILE__) . "/skipif.inc"; ?>"
<?php
if (ini_get("mongo.default_host") == hostname() && ini_get("mongo.default_port") == port()) {
    exit("SKIP This test is meaningless when connecting to localhost:27017");
}
?>
--FILE--
<?php
require_once dirname(__FILE__) . "/../utils.inc";

$host = hostname();
$port = port();
ini_set("mongo.default_host", $host);
ini_set("mongo.default_port", $port);

$opts = array(
    "replicaSet" => rsname(),
    "timeout"    => "42",
    "username"   => array("something"),
    "password"   => array("else"),
    "connect"    => "0",
);
$m = new Mongo(null, $opts);
var_dump($opts, $m);
echo "All done\n";
?>
--EXPECTF--
Notice: Array to string conversion in %smongoclient-ctor-002.php on line %d

Notice: Array to string conversion in %smongoclient-ctor-002.php on line %d
array(5) {
  ["replicaSet"]=>
  string(%d) "%s"
  ["timeout"]=>
  string(2) "42"
  ["username"]=>
  array(1) {
    [0]=>
    string(9) "something"
  }
  ["password"]=>
  array(1) {
    [0]=>
    string(4) "else"
  }
  ["connect"]=>
  string(1) "0"
}
object(Mongo)#%d (4) {
  ["connected"]=>
  bool(false)
  ["status"]=>
  NULL
  ["server":protected]=>
  NULL
  ["persistent":protected]=>
  NULL
}
All done

