<?php
MongoLog::setLevel(MongoLog::ALL);
MongoLog::setModule(MongoLog::ALL);
set_error_handler('foo'); function foo($a, $b) { echo $b, "\n"; }
$m = new Mongo("mongodb://user:user@whisky:13000/phpunit", array("replicaSet" => "seta"));
?>
