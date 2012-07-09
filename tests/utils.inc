<?php # vim: ft=php

function m(&$dsn, &$port, &$opts = array()) {
    $dsn  = "";
    $port = "27017";
    $opts = array();

    if (!empty($_ENV["MONGO_OPTS"])) {
        $opts = json_decode($_ENV["MONGO_OPTS"]);
    }
    if (!empty($_ENV["MONGO_DSN"])) {
        $dsn = $_ENV["MONGO_DSN"];
        return;
    }

    $type = "STANDALONE";
    if (!empty($_ENV["MONGO_SERVER"])) {
        $type = $_ENV["MONGO_SERVER"];
    }
    switch($type) {
        case "STANDALONE":
            $dsn  = $GLOBALS["STANDALONE_HOSTNAME"];
            $port = $GLOBALS["STANDALONE_PORT"];
        return;

        case "REPLICASET":
            $dsn  = $GLOBALS["REPLICASET_PRIMARY"];
            $port = $GLOBALS["REPLICASET_PRIMARY_PORT"];
            $opts["replicaSet"] = $GLOBALS["REPLICASET_NAME"];
        return;
    }
}
function mongo($usedb = null, $useprefix = true, $useport = true) {

    m($dsn, $port, $opts);

    if ($useport) {
        $dsn .= ":$port";
    }

    if ($usedb) {
        if ($usedb[0] != "/") {
            $dsn .= "/";
        }
        $dsn .= $usedb;
    }

    if ($useprefix) {
        $dsn = "mongodb://$dsn";
    }

    if ($opts) {
        $m = new Mongo($dsn, $opts);
    }
    else {
        $m = new Mongo($dsn, $opts);
    }
    return $m;
}

function rs($db = null) {
    $_ENV["MONGO_SERVER"] = "REPLICASET";
    return mongo($db);
}



$STANDALONE_HOSTNAME = "standalone.local";

$REPLICASET_NAME       = "RS";
$REPLICASET_PRIMARY    = "primary.rs.local";
$REPLICASET_SECONDARY  = "secondary.rs.local";
$REPLICASET_ARBITER    = "tertiary.rs.local";

$STANDALONE_PORT =
    $REPLICASET_PORT =
    $REPLICASET_PRIMARY_PORT =
    $REPLICASET_SECONDARY_PORT =
    $REPLICASET_ARBITER_PORT = 27017;

$REPLICASET_HOSTNAME   = join(",", array_unique(array("$REPLICASET_PRIMARY:$REPLICASET_PRIMARY_PORT", "$REPLICASET_SECONDARY:$REPLICASET_SECONDARY_PORT", "$REPLICASET_ARBITER:$REPLICASET_PRIMARY_PORT")));

// Include user define config file to overwrite the defaults
require "mongo-test-cfg.inc";

