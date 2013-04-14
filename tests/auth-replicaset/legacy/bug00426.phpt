--TEST--
Test for PHP-426: Connection pool not paying attention to authentication when using replicaSet=true
--SKIPIF--
<?php if (version_compare(phpversion(), "5.3.0", "lt")) exit("skip The connected property is 5.3+"); ?>
<?php require_once "tests/utils/auth-replicaset.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

function get_user($m, $username) {
    $db = $m->selectDB("admin");
    $c = $db->selectCollection("system.users");

    return $c->findOne(array("user" => $username));
}

$s = new MongoShellServer;
$cfg = $s->getReplicaSetConfig(true);
$creds = $s->getCredentials();

$opts = array(
    "db" => "admin",
    "username" => $creds["admin"]->username,
    "password" => $creds["admin"]->password,
    "replicaSet" => true,
);
$m = new MongoClient($cfg["dsn"], $opts+array("readPreference" => MongoClient::RP_SECONDARY_PREFERRED));
var_dump($m);
var_dump(get_user($m, $creds["admin"]->username));

try {
    $opts["password"] .= "THIS-PASSWORD-IS-WRONG";
    $m = new MongoClient($cfg["dsn"], $opts+array("readPreference" => MongoClient::RP_SECONDARY_PREFERRED));
} catch (MongoConnectionException $e) {
	echo $e->getMessage(), "\n";
}
var_dump($m);
var_dump(get_user($m, $creds["admin"]->username));

?>
--EXPECTF--
object(MongoClient)#%d (4) {
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
No candidate servers found
object(MongoClient)#%d (4) {
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
