--TEST--
GridFS: getting files by ID
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php
require_once "tests/utils/server.inc";
    $m = mongo_standalone("phpunit");
    $db = $m->selectDB("phpunit");
    $db->dropCollection("fs.files");
    $db->dropCollection("fs.chunks");

    $gridfs = $db->getGridFS();

    $tempFileName = tempnam(sys_get_temp_dir(), "gridfs-delete");
    $tempFileData = '1234567890';
    file_put_contents($tempFileName, $tempFileData);

    $ids = array(
        "file0",
        452,
        true,
        new MongoID(),
        array( 'a', 'b' => 5 ),
    );

    foreach ($ids as $id) {
        $gridfs->storeFile($tempFileName, array('_id' => $id));
        $file = $gridfs->get($id);

        echo 'File exists with ID: ';
        var_dump($file->file['_id']);
        echo "\n";
    }

    unlink($tempFileName);
?>
--EXPECTF--
File exists with ID: string(5) "file0"

File exists with ID: int(452)

File exists with ID: bool(true)

File exists with ID: object(MongoId)#%d (1) {
  ["$id"]=>
  string(24) "%s"
}

File exists with ID: array(2) {
  [0]=>
  string(1) "a"
  ["b"]=>
  int(5)
}
