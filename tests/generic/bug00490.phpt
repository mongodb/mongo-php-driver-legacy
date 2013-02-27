--TEST--
Test for PHP-490: "nolock" option in MongoDB::execute method
--SKIPIF--
<?php require_once dirname(__FILE__) . "/skipif.inc"; ?>
--FILE--
<?php
require_once dirname(__FILE__) . "/../utils.inc";
$c = new_mongo();
$db = $c->selectDb(dbname());

$func = 
    "function(greeting, name) { ".
        "return greeting+', '+name+', says '+greeter;".
    "}";
$scope = array("greeter" => "Fred");

$code = new MongoCode($func, $scope);

$opts = array('nolock' => false);
$response = $db->execute($code, array("Goodbye", "Joe"), $opts);
var_dump($opts, $response);
?>
--EXPECTF--
array(1) {
  ["nolock"]=>
  bool(false)
}
array(2) {
  ["retval"]=>
  string(23) "Goodbye, Joe, says Fred"
  ["ok"]=>
  float(1)
}
