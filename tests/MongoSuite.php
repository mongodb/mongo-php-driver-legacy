<?php

require_once 'MongoInt32Test.php';
require_once 'MongoInt64Test.php';

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

require_once 'MongoObjectsTest.php';
require_once 'MongoObjDBTest.php';

require_once 'RegressionTest1.php';

require_once 'MongoMemTest.php';
require_once 'CmdSymbolTest.php';
require_once 'SerializationTest.php';
require_once 'AuthTest.php';
require_once 'MinMaxKeyTest.php';
require_once 'MongoDBRefTest.php';

include 'MongoAuthTest.php';
include 'MongoGridFSClassicTest.php';
 
class MongoSuite extends PHPUnit_Framework_TestSuite
{
    public static function suite()
    {
        $suite = new MongoSuite('Mongo Tests');
      
        $suite->addTestSuite('MongoInt32Test');
        $suite->addTestSuite('MongoInt64Test');
        $suite->addTestSuite('MongoTest');
        $suite->addTestSuite('MongoDBTest');
        $suite->addTestSuite('MongoCollectionTest');
        $suite->addTestSuite('MongoCollectionTest2');
        $suite->addTestSuite('MongoCursorTest');
        
        // */
        // 5.1 is just going to mess up the file stuff
        if (floatval(phpversion()) >= 5.2) {
          $suite->addTestSuite('MongoGridFSTest');
          $suite->addTestSuite('MongoGridFSFileTest');
          $suite->addTestSuite('MongoGridFSCursorTest');
        }
        // */
        
        $suite->addTestSuite('MongoIdTest');
        $suite->addTestSuite('MongoCodeTest');
        $suite->addTestSuite('MongoRegexTest');
        $suite->addTestSuite('MongoBinDataTest');
        $suite->addTestSuite('MongoDateTest');
        $suite->addTestSuite('MongoTimestampTest');

        // */
        $suite->addTestSuite('MongoObjectsTest');
        $suite->addTestSuite('MongoObjDBTest');
        
        $suite->addTestSuite('RegressionTest1');
        $suite->addTestSuite('CmdSymbolTest');
        $suite->addTestSuite('SerializationTest');
	$suite->addTestSuite('MinMaxKeyTest');
	$suite->addTestSuite('MongoDBRefTest');

        // try adding an admin user
	exec("mongo tests/addUser.js", $output, $exit_code);
	if ($exit_code != 0) {
	  echo "\nNot running admin/auth tests\n";
	  echo implode("\n", $output);
	}
	else {
	  $suite->addTestSuite('AuthTest');
	  if (class_exists("MongoAuth")) {
	    $suite->addTestSuite('MongoAuthTest');
	  }
	  else {
            echo "\nAdd \$pwd/php/ to include_path to run admin/auth tests\n";
	  }
	}

        // */
        return $suite;
    }
 
    protected function setUp()
    {
        // paired
        // $this->sharedFixture = new Mongo('localhost:27017,localhost:27018', true, false, true);

        // normal
        $this->sharedFixture = new Mongo();

        $db = $this->sharedFixture->selectDB('phpunit');

        $c = $db->selectCollection('test');
        $c->insert(array('x'=>'y'));
        $c->ensureIndex('x');
        $c->drop();

        $ns = $db->selectCollection('system.namespaces');
        if ($ns->findOne(array('name' => 'phpunit.test')) != NULL) {
            echo "\n\nMongoCollection::drop() isn't working.  ".
                "Most likely, you are running an old version of ".
                "the database, which will cause a lot of tests to ".
                "fail.  Please consider upgrading.\n";
        }

        $this->sharedFixture->version_51 = "/5\.1\../";

        var_dump($this->sharedFixture->listDBs());
    }
 
    protected function tearDown()
    {
        $this->sharedFixture->dropDB("phpunit");
        $this->sharedFixture->close();

        // remove db user
        echo "\n";
        exec("mongo tests/deleteUser.js");
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
