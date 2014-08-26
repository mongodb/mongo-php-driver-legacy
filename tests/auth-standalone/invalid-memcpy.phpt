--TEST--
CID-13261: sizeof() mismatch duplicating context
--SKIPIF--
<?php require_once "tests/utils/auth-standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

function log_insert($server, $doc, $options) {
    echo __METHOD__, "\n";
}


$ctx = stream_context_create(
    array(
        "mongodb" => array(
            "log_insert" => "log_insert",
            "log_cmd_insert" => "log_insert",
        )
    )
);

$s = new MongoShellServer;
$host = $s->getStandaloneConfig(true);
$creds = $s->getCredentials();
$ret = $s->addStandaloneUser("db2", $creds["user"]->username, $creds["user"]->password);

$opts = array(
    "db" => "test",
    "username" => $creds["user"]->username,
    "password" => $creds["user"]->password,
);
$mc = new MongoClient($host, $opts, array("context" => $ctx));

$collection = $mc->db2->jobs;
$doc = array("example" => "document", "with" => "some", "fields" => "in it");
$retval = $collection->insert($doc);


echo "I'm alive\n";

?>
--EXPECTF--
log_insert
I'm alive
