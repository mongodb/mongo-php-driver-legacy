--TEST--
Test for PHP-426: Connection pool not paying attention to authentication when using replicaSet=true
--SKIPIF--
<?php if (version_compare(phpversion(), "5.3.0", "lt")) exit("skip The connected property is 5.3+"); ?>
<?php require_once "tests/utils/auth-replicaset.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";
writeLogs(__FILE__);

function get_user($m, $username) {
    $db = $m->selectDB("admin");
    $c = $db->selectCollection("system.users");

    $user = $c->findOne(array("user" => $username));
    return array(
        "_id"      => $user["_id"],
        "user"     => $user["user"],
        "readOnly" => $user["readOnly"],
        "pwd"      => $user["pwd"],
    );
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
    printLogs(MongoLog::CON, MongoLog::WARNING);
    $m = new MongoClient($cfg["dsn"], $opts+array("readPreference" => MongoClient::RP_SECONDARY_PREFERRED));
    echo "I still have a MongoClient object\n";
    $m->admin->test->findOne();
} catch (MongoConnectionException $e) {
    echo $e->getMessage(), "\n";
}

?>
===DONE===
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
authentication failed
Couldn't connect to '%s:%d': Authentication failed on database 'admin' with username 'root': auth %s
authentication failed
Couldn't connect to '%s:%d': Authentication failed on database 'admin' with username 'root': auth %s
authentication failed
Couldn't connect to '%s:%d': Authentication failed on database 'admin' with username 'root': auth %s
discover_topology: couldn't create a connection for %s:%d;-;admin/root/%s;%d
discover_topology: couldn't create a connection for %s:%d;-;admin/root/%s;%d
discover_topology: couldn't create a connection for %s:%d;-;admin/root/%s;%d
No candidate servers found
===DONE===

