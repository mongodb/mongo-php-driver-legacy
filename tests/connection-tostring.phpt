--TEST--
Connection strings: toString.
--FILE--
<?php
$a = new Mongo("localhost,127.0.0.2", false);
echo $a, "\n";
$a = new Mongo("localhost,127.0.0.2");
echo $a, "\n";
$a = new Mongo("localhost:27017,127.0.0.2:27017");
echo $a, "\n\n";

$a = new Mongo("localhost,127.0.0.2", false);
echo $a->__toString(), "\n";
$a = new Mongo("localhost,127.0.0.2");
echo $a->__toString(), "\n";
$a = new Mongo("localhost:27017,127.0.0.2:27017");
echo $a->__toString(), "\n\n";

$a = new Mongo("mongodb://localhost:27018,localhost:27017,localhost:27019");
var_dump($a->__toString());

$a = new Mongo("mongodb://localhostalocalhostalocalhostalocalhostalocalhostalocalhostalocalhostalocalhostalocalhostalocalhostalocalhostalocalhostalocalhostalocalhostalocalhostalocalhostalocalhostalocalhostalocalhostalocalhostalocalhostalocalhostalocalhostalocalhostalocalhosta:27018,localhost:27017");
var_dump($a->__toString());

--EXPECT--
[localhost:27017],[127.0.0.2:27017]
localhost:27017,[127.0.0.2:27017]
localhost:27017,[127.0.0.2:27017]

[localhost:27017],[127.0.0.2:27017]
localhost:27017,[127.0.0.2:27017]
localhost:27017,[127.0.0.2:27017]

string(51) "[localhost:27018],localhost:27017,[localhost:27019]"
string(274) "[localhostalocalhostalocalhostalocalhostalocalhostalocalhostalocalhostalocalhostalocalhostalocalhostalocalhostalocalhostalocalhostalocalhostalocalhostalocalhostalocalhostalocalhostalocalhostalocalhostalocalhostalocalhostalocalhostalocalhostalocalhosta:27018],localhost:27017"
