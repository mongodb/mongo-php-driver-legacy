<?php # vim: ft=php

function m(&$dsn, &$port, &$db, &$opts = array()) {
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
            $dsn                = $GLOBALS["STANDALONE_HOSTNAME"];
            $port               = $GLOBALS["STANDALONE_PORT"];
        return;

        case "STANDALONE_AUTH":
            $dsn                = $GLOBALS["STANDALONE_AUTH_HOSTNAME"];
            $port               = $GLOBALS["STANDALONE_AUTH_PORT"];
            if ($db == "admin") {
                $opts["username"]   = $GLOBALS["STANDALONE_AUTH_ADMIN_USERNAME"];
                $opts["password"]   = $GLOBALS["STANDALONE_AUTH_ADMIN_PASSWORD"];
            }
            else {
                $db                 = $GLOBALS["STANDALONE_AUTH_DATABASE"];
                $opts["username"]   = $GLOBALS["STANDALONE_AUTH_USERNAME"];
                $opts["password"]   = $GLOBALS["STANDALONE_AUTH_PASSWORD"];
            }
        return;

        case "REPLICASET":
            $dsn                = $GLOBALS["REPLICASET_PRIMARY"];
            $port               = $GLOBALS["REPLICASET_PRIMARY_PORT"];
            $opts["replicaSet"] = $GLOBALS["REPLICASET_NAME"];
        return;

        case "REPLICASET_AUTH":
            $dsn                = $GLOBALS["REPLICASET_AUTH_PRIMARY"];
            $port               = $GLOBALS["REPLICASET_AUTH_PORT"];
            $opts["replicaSet"] = $GLOBALS["REPLICASET_NAME"];
            if ($db == "admin") {
                $opts["username"]   = $GLOBALS["REPLICASET_AUTH_ADMIN_USERNAME"];
                $opts["password"]   = $GLOBALS["REPLICASET_AUTH_ADMIN_PASSWORD"];
            } else {
                $opts["username"]   = $GLOBALS["REPLICASET_AUTH_USERNAME"];
                $opts["password"]   = $GLOBALS["REPLICASET_AUTH_PASSWORD"];
                $db                 = $GLOBALS["REPLICASET_AUTH_DATABASE"];
            }
        return;
    }
}
function mongo($usedb = null, $useprefix = true, $useport = true, $creds = array()) {

    m($dsn, $port, $usedb, $opts);

    if (!$dsn) {
        throw new Exception("Don't have a host to connect to");
    }

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
        $m = new Mongo($dsn);
    }

    return $m;
}

function rs($db = null) {
    $_ENV["MONGO_SERVER"] = "REPLICASET";
    return mongo($db);
}


function auth($usedb = null, $creds = array(), $useprefix = true, $useport = true) {
    return mongo($usedb, $useprefix, $useport, $creds);
}
function dbname() {
    if (empty($_ENV["MONGO_SERVER"])) {
        return;
    }
    switch($_ENV["MONGO_SERVER"]) {
        case "STANDALONE_AUTH":
            return $GLOBALS["STANDALONE_AUTH_DATABASE"];
        return;

        case "REPLICASET_AUTH":
            return $GLOBALS["REPLICASET_AUTH_DATABASE"];
        return;

        default:
            return "phpunit";
    }
}
function port() {
    m($dsn, $port, $db, $opts);
    return $port;
}

function hostname() {
    m($dsn, $port, $db, $opts);
    return $dsn;
}

function username() {
    m($dsn, $port, $db, $opts);
    if (isset($opts["username"])) {
        return $opts["username"];
    }
    return null;
}

function password() {
    m($dsn, $port, $db, $opts);
    if (isset($opts["password"])) {
        return $opts["password"];
    }
    return null;
}

$STANDALONE_HOSTNAME = "localhost";
$STANDALONE_AUTH_HOSTNAME = "";
$STANDALONE_AUTH_ADMIN_USERNAME = "";
$STANDALONE_AUTH_ADMIN_PASSWORD = "";
$STANDALONE_AUTH_USERNAME = "";
$STANDALONE_AUTH_PASSWORD = "";
$STANDALONE_AUTH_DATABASE = "";

$REPLICASET_AUTH_ADMIN_USERNAME = "";
$REPLICASET_AUTH_ADMIN_PASSWORD = "";
$REPLICASET_AUTH_USERNAME = "";
$REPLICASET_AUTH_PASSWORD = "";
$STANDALONE_AUTH_DATABASE = "";

$REPLICASET_NAME       = "RS";
$REPLICASET_PRIMARY    = "";
$REPLICASET_SECONDARY  = "";
$REPLICASET_ARBITER    = "";

$REPLICASET_AUTH_PRIMARY = "";
$REPLICASET_AUTH_ADMIN_USERNAME = "";
$REPLICASET_AUTH_ADMIN_PASSWORD = "";
$REPLICASET_AUTH_USERNAME = "";
$REPLICASET_AUTH_PASSWORD = "";
$REPLICASET_AUTH_DATABASE = "";

$STANDALONE_PORT =
    $STANDALONE_AUTH_PORT =
    $REPLICASET_PORT =
    $REPLICASET_PRIMARY_PORT =
    $REPLICASET_SECONDARY_PORT =
    $REPLICASET_ARBITER_PORT =
    $REPLICASET_AUTH_PORT = 27017;

// Include user define config file to overwrite the defaults
require "mongo-test-cfg.inc";

$RS_HOSTS = array();
if ($REPLICASET_PRIMARY) {
    $RS_HOSTS[] = "$REPLICASET_PRIMARY:$REPLICASET_PRIMARY_PORT";
}
if ($REPLICASET_SECONDARY) {
    $RS_HOSTS[] = "$REPLICASET_SECONDARY:$REPLICASET_SECONDARY_PORT";
}

if ($REPLICASET_ARBITER) {
    $RS_HOSTS[] = "$REPLICASET_ARBITER:$REPLICASET_ARBITER_PORT";
}

$REPLICASET_HOSTNAME   = join(",", $RS_HOSTS);

