--TEST--
Test for PHP-426: Connection pool not paying attention to authentication when using replicaSet=true
--SKIPIF--
<?php require_once dirname(__FILE__) . "/skipif.inc"; ?>
--FILE--
<?php
//require_once dirname(__FILE__) . "/../debug.inc";
require_once dirname(__FILE__) . "/../utils.inc";

function get_user($m, $username) {
	$db = $m->selectDB(dbname());
	$c = $db->selectCollection("system.users");

	return $c->findOne(array("user" => $username));
}


$m = mongo("admin");
var_dump($m);
var_dump(get_user($m, username()));

$password = password("admin");
var_dump($password);
// Intentionally supply wrong password to test we don't get a valid connection back
$REPLICASET_AUTH_ADMIN_PASSWORD = $STANDALONE_AUTH_ADMIN_PASSWORD = "THIS-PASSWORD-IS-WRONG";

try {
	$m = mongo("admin");
} catch (MongoConnectionException $e) {
	echo $e->getMessage(), "\n";
}
var_dump($m);
var_dump(get_user($m, username()));

?>
--EXPECTF--
object(Mongo)#%d (4) {
  ["connected"]=>
  bool(true)
  ["status"]=>
  NULL
  ["server":protected]=>
  NULL
  ["persistent":protected]=>
  NULL
}
array(4) {
  ["_id"]=>
  object(MongoId)#%d (1) {
	["$id"]=>
	string(24) "%s"
  }
  ["user"]=>
  string(%d) "%s"
  ["readOnly"]=>
  bool(false)
  ["pwd"]=>
  string(32) "%s"
}
string(%d) "%s"
No candidate servers found
object(Mongo)#%d (4) {
  ["connected"]=>
  bool(true)
  ["status"]=>
  NULL
  ["server":protected]=>
  NULL
  ["persistent":protected]=>
  NULL
}
array(4) {
  ["_id"]=>
  object(MongoId)#%d (1) {
	["$id"]=>
	string(24) "%s"
  }
  ["user"]=>
  string(%d) "%s"
  ["readOnly"]=>
  bool(false)
  ["pwd"]=>
  string(32) "%s"
}
