--TEST--
PHP-1107: Use lower socket time out for connection phase
--SKIPIF--
<?php if (version_compare(phpversion(), "5.3.0", "lt")) exit("skip setCallback and closures are 5.3+"); ?>
<?php if (!MONGO_STREAMS) { echo "skip This test requires streams support"; } ?>
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
	"socketTimeoutMS" => 30987,
	"connectTimeoutMS" => 1234,
);

$mc = new MongoClient($host, $opts);

echo "Dummy query\n";
$c = $mc->selectCollection(dbname(), collname(__FILE__));
$c->findOne();

echo "I'm alive\n";

?>
--EXPECTF--
Connecting to tcp://%s:%d (127.0.0.1:%d;-;%s;%d) with connection timeout: 1.234000
Setting stream timeout to 30.987000
ismaster: start
Setting the stream timeout to 1.234000
Now setting stream timeout back to 30.987000
No timeout changes for %s:%d;-;%s;%d
get_server_version: start
Setting the stream timeout to 1.234000
Now setting stream timeout back to 30.987000
No timeout changes for %s:%d;-;%s;%d
getnonce: start
Setting the stream timeout to 1.234000
Now setting stream timeout back to 30.987000
No timeout changes for %s:%d;-;%s;%d
authenticate: start
Setting the stream timeout to 1.234000
Now setting stream timeout back to 30.987000
No timeout changes for %s:%d;-;%s;%d
Setting the stream timeout to 1.234000
Now setting stream timeout back to 30.987000
No timeout changes for %s:%d;-;%s;%d
Dummy query
No timeout changes for %s:%d;-;%s;%d
No timeout changes for %s:%d;-;%s;%d
I'm alive
