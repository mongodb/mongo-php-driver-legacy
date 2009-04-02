--TEST--
MongoGridFS class - basic file functionality
--FILE--
<?php 

include_once "Mongo.php";

$m = new Mongo();

$db = $m->selectDB("phpt");
$grid = $db->getGridFS();
$grid->drop();

// check non-existent file
var_dump($grid->findOne(array('filename' => 'foop')));

// and with cursor?
$cursor = $grid->find(array('filename' => 'foop'));
var_dump($cursor->next());

$files = $db->selectCollection("fs.files");
$chunks = $db->selectCollection("fs.chunks");
$something = array('foo' => 'bar');

// remove file
$grid->storeFile("./tests/somefile");

$files->insert($something);
$chunks->insert($something);

$grid->remove(array("filename" => new MongoRegex('/somefile/')));

$cursor = $files->find();
foreach ($cursor as $v) {
  var_dump($v);
}
 
$cursor = $chunks->find();
foreach ($cursor as $v) {
  var_dump($v);
}

?>
--EXPECTF--
NULL
NULL
array(2) {
  ["_id"]=>
  object(MongoId)#%i (1) {
    ["id"]=>
    string(12) "%s"
  }
  ["foo"]=>
  string(3) "bar"
}
array(2) {
  ["_id"]=>
  object(MongoId)#%i (1) {
    ["id"]=>
    string(12) "%s"
  }
  ["foo"]=>
  string(3) "bar"
}
