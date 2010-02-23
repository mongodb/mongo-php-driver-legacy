<?php

include 'tests/MongoTest.php';
include 'tests/MongoDBTest.php';
include 'tests/MongoCollectionTest.php';
include 'tests/MongoCursorTest.php';

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

require_once 'MongoRegressionTest1.php';

require_once 'MongoMemTest.php';
require_once 'CmdSymbolTest.php';
require_once 'SerializationTest.php';
require_once 'AuthTest.php';
require_once 'MinMaxKeyTest.php';
require_once 'MongoDBRefTest.php';


$testClass = array(
                   "MongoTest",
                   "MongoDBTest", 
                   "MongoCollectionTest", 
                   "MongoCursorTest",
                   "MongoDateTest",
                   "MongoBinDataTest",
                   "MongoRegexTest",
                   "MongoTimestampTest",
                   "MongoIdTest",
                   "MongoCodeTest",
                   "MongoGridFSTest",
                   "MongoGridFSFileTest",
                   "MongoGridFSCursorTest",
                   "MongoObjectsTest",
                   "MongoObjDBTest",
                   "MongoRegressionTest1",
                   "CmdSymbolTest",
                   "SerializationTest",
                   "AuthTest",
                   "MinMaxKeyTest",
                   "MongoDBRefTest"
                   );


foreach($testClass as $classname) {

  $clazz = new ReflectionClass($classname);
  $methods = $clazz->getMethods(ReflectionMethod::IS_PUBLIC);
  
  // filter methods starting with "test"
  foreach ($methods as $method) {
    if (substr($method->name, 0, 4) != "test") {
      continue;
    }
    
    echo "running $method->name\n";
    
    $m = new Mongo();
    $clazz2 = new ReflectionClass($classname);
    $prop = $clazz2->getProperty("sharedFixture");
    $prop->setAccessible(true);
  
    $test = $clazz2->newInstance($method->name);
    $prop->setValue($test, $m);
    $test->run();
  }

}

?>
