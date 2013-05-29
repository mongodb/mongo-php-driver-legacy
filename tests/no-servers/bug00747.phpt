--TEST--
Test for PHP-747: Improve numeric check for write concern option
--SKIPIF--
<?php if (version_compare(phpversion(), "5.3.0", "lt")) exit("skip setCallback and closures are 5.3+"); ?>
--FILE--
<?php
include 'tests/utils/server.inc';

printlogs(MongoLog::ALL, MongoLog::ALL, '/^- Found option \'w\'/');
$formats = array(
	'w=',
	'w=0',
	'w=1',
	'w=1-',
	'w=fasdfads',
	'w=873253',
	'w=-1',
	'w=majority',
	'w=allDCs',
	'w=3.141592654',
);

foreach($formats as $format) {
	try {
		$m = new MongoClient('mongodb://localhost/?' . $format, array('connect' => false ) );
	} catch (MongoConnectionException $e) {
		var_dump($e->getCode());
		var_dump($e->getMessage());
	}
	echo "\n";
}
?>
--EXPECT--
- Found option 'w': 0

- Found option 'w': 0

- Found option 'w': 1

- Found option 'w': '1-'

- Found option 'w': 'fasdfads'

- Found option 'w': 873253

- Found option 'w': -1
int(23)
string(55) "The value of 'w' needs to be 0 or higher (or a string)."

- Found option 'w': 'majority'

- Found option 'w': 'allDCs'

- Found option 'w': '3.141592654'
