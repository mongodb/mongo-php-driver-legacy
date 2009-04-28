<?php
require_once 'MongoTest.php';
require_once 'MongoDBTest.php';
require_once 'MongoCollectionTest.php';
require_once 'MongoCursorTest.php';
require_once 'MongoGridFSTest.php';
require_once 'MongoGridFSFileTest.php';
require_once 'MongoGridFSCursorTest.php';
require_once 'MongoUtilTest.php';
require_once 'MongoRegressionTest1.php';
 
class MongoSuite extends PHPUnit_Framework_TestSuite
{
    public static function suite()
    {
        $suite = new MongoSuite('Mongo Tests');

        $suite->addTestSuite('MongoTest');
        $suite->addTestSuite('MongoDBTest');
        $suite->addTestSuite('MongoCollectionTest');
        $suite->addTestSuite('MongoCursorTest');
        $suite->addTestSuite('MongoGridFSTest');
        $suite->addTestSuite('MongoGridFSFileTest');
        $suite->addTestSuite('MongoGridFSCursorTest');
        $suite->addTestSuite('MongoUtilTest');
        $suite->addTestSuite('MongoRegressionTest1');

        return $suite;
    }
 
    protected function setUp()
    {
        $this->sharedFixture = new Mongo();
    }
 
    protected function tearDown()
    {
        $this->sharedFixture->close();
    }
}
?>
