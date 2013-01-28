<?php # vim: ft=php

function m(&$dsn, &$port, &$db, &$opts = array()) {
    $dsn  = "";
    $port = "27017";

    $type = $_ENV["MONGO_SERVER"];

    $prefix = "";
    switch($type) {
        case "BRIDGE_STANDALONE":
            $prefix = "BRIDGE_";
            /* break intentionally omitted */
        case "STANDALONE":
            $dsn                = $GLOBALS[$prefix."STANDALONE_HOSTNAME"];
            $port               = $GLOBALS["STANDALONE_PORT"];
            $db                 = $GLOBALS["STANDALONE_DATABASE"];
        return;

        case "BRIDGE_STANDALONE_AUTH":
            $prefix = "BRIDGE_";
        case "STANDALONE_AUTH":
            $dsn                = $GLOBALS[$prefix."STANDALONE_AUTH_HOSTNAME"];
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
            $db                 = $GLOBALS["REPLICASET_DATABASE"];
            $port               = $GLOBALS["REPLICASET_PRIMARY_PORT"];
            $opts["replicaSet"] = $GLOBALS["REPLICASET_NAME"];
        return;

        case "REPLICASET_AUTH":
            $dsn                = $GLOBALS["REPLICASET_AUTH_PRIMARY"];
            $port               = $GLOBALS["REPLICASET_AUTH_PORT"];
            $opts["replicaSet"] = $GLOBALS["REPLICASET_AUTH_NAME"];
            if ($db == "admin") {
                $opts["username"]   = $GLOBALS["REPLICASET_AUTH_ADMIN_USERNAME"];
                $opts["password"]   = $GLOBALS["REPLICASET_AUTH_ADMIN_PASSWORD"];
            } else {
                $opts["username"]   = $GLOBALS["REPLICASET_AUTH_USERNAME"];
                $opts["password"]   = $GLOBALS["REPLICASET_AUTH_PASSWORD"];
                $db                 = $GLOBALS["REPLICASET_AUTH_DATABASE"];
            }
        return;

        case "SHARDING":
            $dsn  = $GLOBALS["SHARDING_HOSTNAME"];
            $port = $GLOBALS["SHARDING_PORT"];
            $db   = $GLOBALS["SHARDING_DATABASE"];
        return;
    }
}

function mongo_helper($usedb = null, $useprefix = true, $useport = true, &$opts = array(), &$dsn)
{
    m($dsn, $port, $usedb, $opts);

    if (!$dsn) {
        die(sprintf("skip The '%s' environment is not configured to be executed", $_ENV["MONGO_SERVER"]));
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
}

function new_mongo($usedb = null, $useprefix = true, $useport = true, $opts = array())
{
	mongo_helper($usedb, $useprefix, $useport, $opts, $dsn);

    if ($opts) {
        $m = new MongoClient($dsn, $opts);
    } else {
        $m = new MongoClient($dsn);
    }
    return $m;
}

function old_mongo($usedb = null, $useprefix = true, $useport = true, $opts = array())
{
	mongo_helper($usedb, $useprefix, $useport, $opts, $dsn);

    if ($opts) {
        $m = new Mongo($dsn, $opts);
    } else {
        $m = new Mongo($dsn);
    }
    return $m;
}

function mongo($usedb = null, $useprefix = true, $useport = true, $opts = array())
{
	return old_mongo($usedb, $useprefix, $useport, $opts);
}

function isauth() {
    if (empty($_ENV["MONGO_SERVER"])) {
        return false;
    }
    return (bool)strstr($_ENV["MONGO_SERVER"], "AUTH");
}
function rsname($db = null) {
    m($dsn, $port, $db, $opts);
    return $opts["replicaSet"];
}
function dbname($db = null) {
    m($dsn, $port, $db, $opts);
    return $db;
}
function port($env = null) {
    if ($env) {
        $tmp = $_ENV["MONGO_SERVER"];
        $_ENV["MONGO_SERVER"] = $env;
    }

    m($dsn, $port, $db, $opts);

    if ($env) {
        $_ENV["MONGO_SERVER"] = $tmp;
    }
    return $port;
}

function hostname($env = null) {
    if ($env) {
        $tmp = $_ENV["MONGO_SERVER"];
        $_ENV["MONGO_SERVER"] = $env;
    }

    m($dsn, $port, $db, $opts);

    if ($env) {
        $_ENV["MONGO_SERVER"] = $tmp;
    }
    return $dsn;
}

function username($db = null) {
    m($dsn, $port, $db, $opts);
    if (isset($opts["username"])) {
        return $opts["username"];
    }
    return null;
}

function password($db = null) {
    m($dsn, $port, $db, $opts);
    if (isset($opts["password"])) {
        return $opts["password"];
    }
    return null;
}
function serverversion($m) {
    $data = $m->selectDB(dbname())->command(array('buildinfo'=>true));
    return $data["version"];
}

function logCallback($module, $level, $message)
{
    global $_LOG_CALLBACK_REGEX;

    if ($_LOG_CALLBACK_REGEX) {
        if (preg_match($_LOG_CALLBACK_REGEX, $message)) {
            echo $message, "\n";
        }
    } else {
        echo $message, "\n";
    }
}

function printLogs($module = MongoLog::ALL, $level = MongoLog::ALL, $containing = null)
{
    global $_LOG_CALLBACK_REGEX;

    MongoLog::setModule($module);
    MongoLog::setLevel($level);

    if ($containing) {
        $_LOG_CALLBACK_REGEX = $containing;
    }
    MongoLog::setCallback("logCallback");
}

/**
 * Return all replica set members in the given state.
 *
 * @param integer $state
 * @return array of member names (<host:port>)
 */
function get_members_in_state($state)
{
    $mongo = mongo();
    $status = $mongo->admin->command(array('replSetGetStatus' => true));
    $members = array();

    foreach ($status['members'] as $member) {
        if ($member['state'] == $state) {
            $members[] = $member['name'];
        }
    }

    return $members;
}

/**
 * Return all replica set members in the RECOVERING state.
 *
 * @return array of member names (<host:port>)
 */
function get_recovering()
{
    return get_members_in_state(3);
}

/**
 * Put a member into RECOVERING state or restore it to a normal state.
 *
 * @param string $member <host:port>
 * @param boolean $value Put member into RECOVERING state if true, normal otherwise
 */
function set_maintenance($member, $value)
{
    $mongo = new MongoClient($member);
    $mongo->admin->command(array('replSetMaintenance' => $value));
    $start = time();

    while ($value != in_array($member, get_recovering())) {
        if (time() - $start >= 10) {
            throw new RuntimeException(sprintf('Member %s never switched state', $member));
        }

        usleep(250);
    }
}

if (!function_exists("iterator_to_array")) {
    function iterator_to_array($it) {
        $retval = array();
        foreach($it as $v) {
            $retval[] = $v;
        }
        return $retval;
    }
}
if (!defined("E_DEPRECATED")) {
    define("E_DEPRECATED", E_STRICT);
}


$STANDALONE_HOSTNAME = "localhost";
$BRIDGE_STANDALONE_HOSTNAME = "";
$STANDALONE_AUTH_HOSTNAME = "";
$BRIDGE_STANDALONE_AUTH_HOSTNAME = "";

$STANDALONE_AUTH_ADMIN_USERNAME = "";
$STANDALONE_AUTH_ADMIN_PASSWORD = "";
$STANDALONE_AUTH_USERNAME = "";
$STANDALONE_AUTH_PASSWORD = "";
$STANDALONE_AUTH_DATABASE = "";
$STANDALONE_DATABASE      = "";

$REPLICASET_AUTH_ADMIN_USERNAME = "";
$REPLICASET_AUTH_ADMIN_PASSWORD = "";
$REPLICASET_AUTH_USERNAME = "";
$REPLICASET_AUTH_PASSWORD = "";
$STANDALONE_AUTH_DATABASE = "";

$REPLICASET_NAME       = "RS";
$REPLICASET_DATABASE = "";
$REPLICASET_PRIMARY    = "";
$REPLICASET_SECONDARY  = "";
$REPLICASET_ARBITER    = "";

$REPLICASET_AUTH_PRIMARY = "";
$REPLICASET_AUTH_ADMIN_USERNAME = "";
$REPLICASET_AUTH_ADMIN_PASSWORD = "";
$REPLICASET_AUTH_USERNAME = "";
$REPLICASET_AUTH_PASSWORD = "";
$REPLICASET_AUTH_DATABASE = "";

$SHARDING_HOSTNAME = "";
$SHARDING_DATABASE = "";

$STANDALONE_PORT =
    $STANDALONE_AUTH_PORT =
    $REPLICASET_PORT =
    $REPLICASET_PRIMARY_PORT =
    $REPLICASET_SECONDARY_PORT =
    $REPLICASET_ARBITER_PORT =
    $REPLICASET_AUTH_PORT =
    $SHARDING_PORT = 27017;

// Include user define config file to overwrite the defaults
require_once "mongo-test-cfg.inc";

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

if (empty($_ENV["MONGO_SERVER"])) {
    $self = $_SERVER["PHP_SELF"];
    $type = basename(dirname($self));
    $type = strtoupper($type);
    $type = str_replace("-", "_", $type);

    switch ($type) {
    case "GENERIC":
        $type = "STANDALONE";
        break;

    }
    $_ENV["MONGO_SERVER"] = $type;
}

