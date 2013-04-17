--TEST--
Connection strings: toString.
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
<?php exit("skip This test doesn't make whole lot of sense"); ?>
<?php
require_once "tests/utils/server.inc";

if (isset($_ENV["MONGO_SERVER"]) && $_ENV["MONGO_SERVER"] == "REPLICASET") {
    $port = $REPLICASET_PRIMARY_PORT;
} else {
    $port = $STANDALONE_PORT;
}

if ($port != "27017") {
    die("skip this tests attempts to connect to the standard port");
}
?>
--FILE--
<?php
require_once "tests/utils/server.inc";

if (isset($_ENV["MONGO_SERVER"]) && $_ENV["MONGO_SERVER"] == "REPLICASET") {
    $host = $REPLICASET_PRIMARY;
    $ip = gethostbyname($host);
    $port = $REPLICASET_PRIMARY_PORT;
} else {
    $host = $STANDALONE_HOSTNAME;
    $ip = gethostbyname($host);
    $port = $STANDALONE_PORT;
}

$a = new Mongo("$host,$ip", false);
echo $a, "\n";
$a = new Mongo("$host,$ip");
echo $a, "\n";
$a = new Mongo("$host:$port,$ip:$port");
echo $a, "\n\n";

$a = new Mongo("$host,$ip", false);
echo $a->__toString(), "\n";
$a = new Mongo("$host,$ip");
echo $a->__toString(), "\n";
$a = new Mongo("$host:$port,$ip:$port");
echo $a->__toString(), "\n\n";

$a = new Mongo("mongodb://localhostalocalhostalocalhostalocalhostalocalhostalocalhostalocalhostalocalhostalocalhostalocalhostalocalhostalocalhostalocalhostalocalhostalocalhostalocalhostalocalhostalocalhostalocalhostalocalhostalocalhostalocalhostalocalhostalocalhostalocalhosta:27018,$host:$port");
var_dump($a->__toString());

?>
--EXPECTF--
[%s:%d],[%d.%d.%d.%d:%d]
%s:%d,[%d.%d.%d.%d:%d]
%s:%d,[%d.%d.%d.%d:%d]

[%s:%d],[%d.%d.%d.%d:%d]
%s:%d,[%d.%d.%d.%d:%d]
%s:%d,[%d.%d.%d.%d:%d]

string(%d) "[localhostalocalhostalocalhostalocalhostalocalhostalocalhostalocalhostalocalhostalocalhostalocalhostalocalhostalocalhostalocalhostalocalhostalocalhostalocalhostalocalhostalocalhostalocalhostalocalhostalocalhostalocalhostalocalhostalocalhostalocalhosta:27018],%s:%d"
