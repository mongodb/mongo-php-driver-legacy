<?php
require_once "tests/utils/server.inc";

$server = new MongoShellServer;
$server->makeStandalone(30000);
$server->close();

