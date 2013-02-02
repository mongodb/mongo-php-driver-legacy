<?php
require_once "tests/utils/server.inc";
exit("skip not for travis");

$server = new MongoShellServer;
$server->restartMaster();
$server->close();

