<?php
require_once 'MongoTest.php';
require_once 'MongoDBTest.php';
require_once 'MongoCollectionTest.php';
require_once 'MongoCursorTest.php';
require_once 'MongoGridFSTest.php';
require_once 'MongoGridFSFileTest.php';
require_once 'MongoGridFSCursorTest.php';

require_once 'MongoIdTest.php';
require_once 'MongoCodeTest.php';
require_once 'MongoRegexTest.php';
require_once 'MongoBinDataTest.php';
require_once 'MongoDateTest.php';

require_once 'MongoObjectsTest.php';
require_once 'MongoObjDBTest.php';

require_once 'MongoRegressionTest1.php';

require_once 'MongoMemTest.php';
require_once 'CmdSymbolTest.php';

include 'MongoAuthTest.php';
include 'MongoGridFSClassicTest.php';
 
class MongoSuite extends PHPUnit_Framework_TestSuite
{
    public static function suite()
    {
        $suite = new MongoSuite('Mongo Tests');
        
        $suite->addTestSuite('CmdSymbolTest');
        return $suite;
    }
 
    protected function setUp()
    {
        // paired
        // $this->sharedFixture = new Mongo('localhost:27017,localhost:27018', true, false, true);

        // normal
        $this->sharedFixture = new Mongo();
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
