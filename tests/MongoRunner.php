<?php
require_once 'MongoTest.php';
require_once 'MongoDBTest.php';
require_once 'MongoCollectionTest.php';
require_once 'MongoCollectionTest2.php';
require_once 'MongoCursorTest.php';
require_once 'MongoGridFSTest.php';
require_once 'MongoGridFSFileTest.php';
require_once 'MongoGridFSCursorTest.php';

require_once 'MongoIdTest.php';
require_once 'MongoCodeTest.php';
require_once 'MongoRegexTest.php';
require_once 'MongoBinDataTest.php';
require_once 'MongoDateTest.php';
require_once 'MongoTimestampTest.php';
require_once 'MongoInt32Test.php';
require_once 'MongoInt64Test.php';

require_once 'MongoObjectsTest.php';
require_once 'MongoObjDBTest.php';

require_once 'RegressionTest1.php';

require_once 'MongoMemTest.php';
require_once 'CmdSymbolTest.php';
require_once 'SerializationTest.php';
require_once 'MinMaxKeyTest.php';
require_once 'MongoDBRefTest.php';

include 'MongoAuthTest.php';
include 'MongoGridFSClassicTest.php';
 
class MongoRunner extends PHPUnit_Framework_TestSuite
{
    public static function suite()
    {
        $suite = new MongoRunner('Mongo Tests');
        
        $suite->addTestSuite('MongoCollectionTest2');
        return $suite;
    }
 
    protected function setUp()
    {
        // paired
        // $this->sharedFixture = new Mongo('localhost:27017,localhost:27018', true, false, true);

        // normal
        $this->sharedFixture = new Mongo();
        $this->sharedFixture->version_51 = "/5\.1\../";
    }
 
    protected function tearDown()
    {
        $this->sharedFixture->close();
    }
}

if (!function_exists('memory_get_usage')) {
  function memory_get_usage($arg=0) {
    return 0;
  }
}

if (!function_exists('json_encode')) {
  function json_encode($str) {
    return "$str";
  }
}

?>
