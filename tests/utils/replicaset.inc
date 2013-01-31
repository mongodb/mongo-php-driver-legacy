<?php
require_once "tests/utils/server.inc";

$server = new MongoShellServer;
$server->makeReplicaset(4, 28000);
$server->close();

