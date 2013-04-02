--TEST--
Test for PHP-612: Impossible to provide a list of tagsets to the readPreferenceTags options
--SKIPIF--
<?php if (version_compare(phpversion(), "5.3.0", "lt")) exit("skip setCallback and closures are 5.3+"); ?>
<?php require_once dirname(__FILE__) . "/skipif.inc"; ?>
--INI--
error_reporting=-1
--FILE--
<?php
require_once "tests/utils/server.inc";
$rp = array(
    "replicaSet"         => "RS",
    "connect"            => false,
    "readPreference"     => MongoClient::RP_PRIMARY_PREFERRED,
    "readPreferenceTags" => array("dc:ny,important:B", "dc:sf", ""),
    "w"                  => "default",
    "wtimeout"           => 200,
);
printLogs(MongoLog::PARSE, MongoLog::INFO);

$mc = new MongoClient("mongodb://node1,node2", $rp);

echo "\nTry the tagset option as a string\n\n";

$rp["readPreferenceTags"] = "dc:sf,important:A";
$mc = new MongoClient("mongodb://node1,node2", $rp);
?>
--EXPECTF--
Parsing mongodb://node1,node2
- Found node: node1:27017
- Found node: node2:27017
- Connection type: MULTIPLE
- Found option 'replicaSet': 'RS'
- Switching connection type: REPLSET
- Found option 'readPreference': 'primaryPreferred'
- Found option 'readPreferenceTags': 'dc:ny,important:B'
- Found tag 'dc': 'ny'
- Found tag 'important': 'B'
- Found option 'readPreferenceTags': 'dc:sf'
- Found tag 'dc': 'sf'
- Found option 'readPreferenceTags': ''
- Found option 'w': 'default'
- Found option 'wTimeout' ('wTimeoutMS'): 200

Try the tagset option as a string

Parsing mongodb://node1,node2
- Found node: node1:27017
- Found node: node2:27017
- Connection type: MULTIPLE
- Found option 'replicaSet': 'RS'
- Switching connection type: REPLSET
- Found option 'readPreference': 'primaryPreferred'
- Found option 'readPreferenceTags': 'dc:sf,important:A'
- Found tag 'dc': 'sf'
- Found tag 'important': 'A'
- Found option 'w': 'default'
- Found option 'wTimeout' ('wTimeoutMS'): 200
