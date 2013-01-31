--TEST--
Test for PHP-602: Use real error codes for MongoConnectionException on ctor failure.
--FILE--
<?php
echo "ARRAY:\n";
try {
	$m = new MongoClient("localhost", array("connect" => false, "timeou" => 4));
} catch(Exception $e) {
	var_dump($e->getCode(), $e->getMessage());
}

try {
	$m = new MongoClient("localhost", array("connect" => false, "readPreference" => "nearest", "slaveOkay" => true));
} catch(Exception $e) {
	var_dump($e->getCode(), $e->getMessage());
}

try {
	$m = new MongoClient("localhost", array("connect" => false, "bogus"));
} catch(Exception $e) {
	var_dump($e->getCode(), $e->getMessage());
}

echo "STRING:\n";
try {
	$m = new MongoClient("mongodb://localhost/?readPreference");
} catch(Exception $e) {
	var_dump($e->getCode(), $e->getMessage());
}

try {
	$m = new MongoClient("mongodb://localhost/?=true");
} catch(Exception $e) {
	var_dump($e->getCode(), $e->getMessage());
}

try {
	$m = new MongoClient("mongodb://localhost/?timeou=4");
} catch(Exception $e) {
	var_dump($e->getCode(), $e->getMessage());
}

try {
	$m = new MongoClient("mongodb://localhost/?readPreference=nearest;slaveOkay=true");
} catch(Exception $e) {
	var_dump($e->getCode(), $e->getMessage());
}

echo "OTHERS:\n";
MongoCursor::$slaveOkay = true;
try {
	$m = new MongoClient("localhost", array("connect" => false, "readPreference" => "nearest"));
} catch(Exception $e) {
	var_dump($e->getCode(), $e->getMessage());
}

?>
--EXPECT--
ARRAY:
int(22)
string(64) "- Found unknown connection string option 'timeou' with value '4'"
int(23)
string(87) "You can not use both slaveOkay and read-preferences. Please switch to read-preferences."
int(25)
string(34) "Unrecognized or unsupported option"
STRING:
int(21)
string(29) "- Found an empty option value"
int(21)
string(28) "- Found an empty option name"
int(22)
string(64) "- Found unknown connection string option 'timeou' with value '4'"
int(23)
string(87) "You can not use both slaveOkay and read-preferences. Please switch to read-preferences."
OTHERS:
int(23)
string(87) "You can not use both slaveOkay and read-preferences. Please switch to read-preferences."
