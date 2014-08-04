--TEST--
Test for PHP-1156: Log when connection timeout uses default_socket_timeout
--SKIPIF--
<?php require_once "tests/utils/standalone.inc" ?>
<?php if (!version_compare(phpversion(), "5.3", '>=')) echo "skip >= PHP 5.3 needed\n"; ?>
<?php if (!MONGO_STREAMS) { echo "skip This test requires streams support"; } ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

printLogs(MongoLog::CON, MongoLog::FINE, "/timeout/");

$host = MongoShellServer::getStandaloneInfo();
$mc = new MongoClient($host, array('connectTimeoutMS' => 0));
echo "Connected\n";

?>
===DONE===
<?php exit(0); ?>
--EXPECTF--
Connecting to tcp://%s:%d (%s:%d;-;.;%d) without connection timeout (default_socket_timeout will be used)
Setting stream timeout to 30.000000
No timeout changes for %s:%d;-;.;%d
No timeout changes for %s:%d;-;.;%d
No timeout changes for %s:%d;-;.;%d
No timeout changes for %s:%d;-;.;%d
No timeout changes for %s:%d;-;.;%d
No timeout changes for %s:%d;-;.;%d
Connected
===DONE===
