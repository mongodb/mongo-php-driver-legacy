<?php
require_once "tests/utils/server.inc";

$server = new MongoShellServer;
$server->makeShard(2);
$server->close();

