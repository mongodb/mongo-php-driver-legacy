--TEST--
Test for PHP-426: Connection pool not paying attention to authentication when using replicaSet=true
--SKIPIF--
<?php require_once "tests/utils/auth-replicaset.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

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
$response = $m->admin->command(array('buildInfo' => 1));
dump_these_keys($response, array('version', 'ok'));

try {
    $opts["password"] .= "THIS-PASSWORD-IS-WRONG";
    printLogs(MongoLog::CON, MongoLog::WARNING);
    $m = new MongoClient($cfg["dsn"], $opts+array("readPreference" => MongoClient::RP_SECONDARY_PREFERRED));
    echo "I still have a MongoClient object\n";
    // An exception should be thrown before the following code is executed
    $response = $m->admin->command(array('buildInfo' => 1));
    dump_these_keys($response, array('version', 'ok'));
} catch (MongoConnectionException $e) {
    printf("error message: %s\n", $e->getMessage());
    printf("error code: %d\n", $e->getCode());
}

?>
--EXPECTF--
array(2) {
  ["version"]=>
  string(%d) "%s"
  ["ok"]=>
  float(1)
}
%s failed
Couldn't connect to '%s:%d': %sfailed on database 'admin'%s
%s failed
Couldn't connect to '%s:%d': %sfailed on database 'admin'%s
%s failed
Couldn't connect to '%s:%d': %s failed on database 'admin'%s
discover_topology: couldn't create a connection for %s:%d;-;admin/root/%s;%d
discover_topology: couldn't create a connection for %s:%d;-;admin/root/%s;%d
discover_topology: couldn't create a connection for %s:%d;-;admin/root/%s;%d
error message: No candidate servers found
error code: 71
