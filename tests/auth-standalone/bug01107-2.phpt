--TEST--
PHP-1107: Connection without connectTimeoutMS and socketTimeoutMS options
--SKIPIF--
<?php $needs = "2.7.0"; $needsOp = "<"; ?>
<?php require_once "tests/utils/auth-standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

printLogs(MongoLog::CON, MongoLog::ALL, '/start|timeout/');

$s = new MongoShellServer;
$host = $s->getStandaloneConfig(true);
$creds = $s->getCredentials();
$ret = $s->addStandaloneUser("db2", $creds["user"]->username, $creds["user"]->password);

$opts = array(
    "db" => "test",
    "username" => $creds["user"]->username,
    "password" => $creds["user"]->password,
);

$mc = new MongoClient($host, $opts);

echo "Dummy query\n";
$c = $mc->selectCollection(dbname(), collname(__FILE__));
$c->findOne();

echo "I'm alive\n";

?>
--EXPECTF--
Connecting to tcp://%s:%d (%s:%d;-;%s;%d) with connection timeout: 60.000000
Setting stream timeout to 30.000000
ismaster: start
Setting the stream timeout to 60.000000
Now setting stream timeout back to 30.000000
Setting the stream timeout to 60.000000
Now setting stream timeout back to 30.000000
get_server_version: start
Setting the stream timeout to 60.000000
Now setting stream timeout back to 30.000000
Setting the stream timeout to 60.000000
Now setting stream timeout back to 30.000000
getnonce: start
Setting the stream timeout to 60.000000
Now setting stream timeout back to 30.000000
Setting the stream timeout to 60.000000
Now setting stream timeout back to 30.000000
authenticate: start
Setting the stream timeout to 60.000000
Now setting stream timeout back to 30.000000
Setting the stream timeout to 60.000000
Now setting stream timeout back to 30.000000
Setting the stream timeout to 60.000000
Now setting stream timeout back to 30.000000
Setting the stream timeout to 60.000000
Now setting stream timeout back to 30.000000
Dummy query
Initializing cursor timeout to 30000 (from connection options)
No timeout changes for %s:%d;-;%s;%d
No timeout changes for %s:%d;-;%s;%d
I'm alive
