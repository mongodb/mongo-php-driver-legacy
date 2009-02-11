<?php

require "src/php/mongo.php";

class TestMongo {

  var $m;

  public function __construct() {
    assert_options(ASSERT_ACTIVE, true);
    assert_options(ASSERT_BAIL, true);
    $this->m = new Mongo();
  }

  public function __destruct() {
    $this->m->close();
  }

  public function getAuthTest() {
    $auth = $this->m->getAuth( "admin", "kristina", "fred" );
    assert( $auth );

    $dbs = $auth->listDBs();
    assert( $dbs );

    $ok = $auth->setLogging( LOG_RW );
    assert( $ok );

    $ok = $auth->setTracing( TRACE_SOME );
    assert( $ok );
    $ok = $auth->setTracing( TRACE_OFF );
    assert( $ok );

    $ok = $auth->setQueryTracing( TRACE_ON );
    assert( $ok );
    $ok = $auth->setQueryTracing( TRACE_OFF );
    assert( $ok );

    $ok = $auth->logout();
    assert( $ok );
  }

  public function db() {
    $db = $this->m->selectDB( "foo" );
    assert( $db );

    assert( PROFILING_SLOW );
    $ok = $db->setProfilingLevel( PROFILING_SLOW );
    assert( $ok == 0 );

    $ok = $db->getProfilingLevel();
    assert( $ok );

    $ok = $db->setProfilingLevel( PROFILING_OFF );
  }


  public function collection() {
    $coll = $this->m->selectDB( "foo" )->selectCollection( "bar" );
    assert( $coll );
    $coll->drop();

    assert( "$coll" == "foo.bar" );
    for( $i=0; $i<10; $i++ )
      $coll->insert( array( "i" => $i, "value" => "hi" ) );

    assert( $coll->count() == 10 );
    $x = $coll->findOne();
    assert( $x );
    assert( $x[ "value" ] == "hi" );

    $cursor = $coll->find()->sort( array( "i" => 1 ) );
    assert( $cursor );
    $i = 0;
    while( $cursor->hasNext() ) {
      $obj = $cursor->next();
      assert( $obj[ "i" ] == $i );
      $i++;
    }
  }

  public function run() {
    $this->getAuthTest();
    $this->db();
    $this->collection();
  }

}

$tests = new TestMongo();
$tests->run();

?>
