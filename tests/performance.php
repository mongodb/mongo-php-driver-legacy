<?php

$PER_TRIAL = 10000;
$BATCH_SIZE = 100;

include "Mongo.php";

$m  = new Mongo();
$db = $m->selectDB("test");

function micro_time()
{
  list($usec, $sec) = explode(" ", microtime());
  return (float)$usec + (float)$sec;
}

function none($c, $obj, $num, $findOne=false)
{
  $c->drop();

  if($findOne) {
    go_find_one($c, $obj, $num);
  } else {
    go($c, $obj, $num);
  }
}

function index($c, $obj, $num, $findOne=false)
{
  $c->drop();
  $c->ensureIndex("x");

  if($findOne) {
    go_find_one($c, $obj, $num);
  } else {
    go($c, $obj, $num);
  }
}

function batch($c, $obj, $PER_TRIAL)
{
  echo "{insert} $c\n";
  global $BATCH_SIZE;

  $c->drop();

  $start = micro_time();
  for($i=0;$i<$PER_TRIAL;$i++){
    $obja = array();
    for($j=0;$j<$BATCH_SIZE;$j++){
      $obj["x"]=$i;
      $obja[] = $obj;
    }
    $c->batchInsert( $obja );
  }
  $c->findOne();
  $end = micro_time();

  $total = $end - $start;
  $ops = $PER_TRIAL/$total;
  echo "total time: $total\n";
  echo "ops/sec: $ops\n";
  echo "------\n";
}

function go($c, $obj, $max)
{
  echo "{insert} $c\n";

  $start = micro_time();
  for($i=0;$i<$max;$i++){
    $obj["x"]=$i;
    $c->insert( $obj );
  }
  $end = micro_time();
  $total = $end - $start;
  $ops = $max/$total;
  echo "total time: $total\n";
  echo "ops/sec: $ops\n";
  echo "------\n";
}

function go_find_one($c, $obj, $max)
{
  echo "{insert/findOne} $c\n";

  $start = micro_time();
  for($i=0;$i<$max;$i++){
    $obj["x"]=$i;
    $c->insert( $obj );
  }
  $x=$c->findOne();

  $end = micro_time();
  $total = $end - $start;
  $ops = $max/$total;
  echo "total time: $total\n";
  echo "ops/sec: $ops\n";
  echo "------\n";
}


function find_one($c, $PER_TRIAL)
{
  echo "find_one $c\n";

  $query = array("x" => $PER_TRIAL/2);
  $start = micro_time();
  for($i=0;$i<$PER_TRIAL;$i++){
    $c->findOne( $query );
  }
  $end = micro_time();
  $total = $end - $start;
  $ops = $PER_TRIAL/$total;
  echo "total time: $total\n";
  echo "ops/sec: $ops\n";
  echo "------\n";
}


function find($c, $PER_TRIAL)
{
  echo "{find} $c\n";

  $query = array("x" => $PER_TRIAL/2);
  $start = micro_time();
  for($i=0;$i<$PER_TRIAL;$i++){
    $cursor = $c->find( $query );
    while( $cursor->hasNext() ){
      $cursor->next();
    }
  }
  $end = micro_time();
  $total = $end - $start;
  $ops = $PER_TRIAL/$total;
  echo "total time: $total\n";
  echo "ops/sec: $ops\n";
  echo "------\n";
}

function find_range($c, $PER_TRIAL)
{
  echo "{find_range} $c\n";
  global $BATCH_SIZE;

  $query = array("x" => array('$gt' => $PER_TRIAL/2,
                              '$lt' => $PER_TRIAL/2+$BATCH_SIZE));

  $start = micro_time();
  for($i=0;$i<$PER_TRIAL;$i++){
    $cursor = $c->find( $query );
    $cursor->hasNext();
    /*
    while( $cursor->hasNext() ){
      $cursor->next();
      }*/
  }
  $end = micro_time();
  $total = $end - $start;
  $ops = $PER_TRIAL/$total;
  echo "total time: $total\n";
  echo "ops/sec: $ops\n";
  echo "------\n";
}


$small  = array("x" => 0);
$medium = array("x" => 0, 
                "integer" => 5,
                "number" => 5.05,
                "boolean" => false,
                "array" => array( "test", "benchmark"));
$large  = array("x" => 0, 
                "base_url" => "http://www.example.com/test-me",
                "total_word_count" => 6743,
                "access_time" => new MongoDate(),
                "meta_tags" => array("description" => "i am a long description string",
                                     "author" => "Holly Man",
                                     "dynamically_created_meta_tag" => "who know\n what"
                                     ),
                "page_structure" => array("counted_tags" => 3450,
                                          "no_of_js_attached" => 10,
                                          "no_of_images" => 6
                                          ),
                "harvested_words" => array("10gen","web","open","source","application","paas",
                                        "platform-as-a-service","technology","helps",
                                        "developers","focus","building","mongodb","mongo",
                                        "10gen","web","open","source","application","paas",
                                        "platform-as-a-service","technology","helps",
                                        "developers","focus","building","mongodb","mongo",
                                        "10gen","web","open","source","application","paas",
                                        "platform-as-a-service","technology","helps",
                                        "developers","focus","building","mongodb","mongo",
                                        "10gen","web","open","source","application","paas",
                                        "platform-as-a-service","technology","helps",
                                        "developers","focus","building","mongodb","mongo",
                                        "10gen","web","open","source","application","paas",
                                        "platform-as-a-service","technology","helps",
                                        "developers","focus","building","mongodb","mongo",
                                        "10gen","web","open","source","application","paas",
                                        "platform-as-a-service","technology","helps",
                                        "developers","focus","building","mongodb","mongo",
                                        "10gen","web","open","source","application","paas",
                                        "platform-as-a-service","technology","helps",
                                        "developers","focus","building","mongodb","mongo",
                                        "10gen","web","open","source","application","paas",
                                        "platform-as-a-service","technology","helps",
                                        "developers","focus","building","mongodb","mongo",
                                        "10gen","web","open","source","application","paas",
                                        "platform-as-a-service","technology","helps",
                                        "developers","focus","building","mongodb","mongo",
                                        "10gen","web","open","source","application","paas",
                                        "platform-as-a-service","technology","helps",
                                        "developers","focus","building","mongodb","mongo",
                                        "10gen","web","open","source","application","paas",
                                        "platform-as-a-service","technology","helps",
                                        "developers","focus","building","mongodb","mongo",
                                        "10gen","web","open","source","application","paas",
                                        "platform-as-a-service","technology","helps",
                                        "developers","focus","building","mongodb","mongo",
                                        "10gen","web","open","source","application","paas",
                                        "platform-as-a-service","technology","helps",
                                        "developers","focus","building","mongodb","mongo",
                                        "10gen","web","open","source","application","paas",
                                        "platform-as-a-service","technology","helps",
                                        "developers","focus","building","mongodb","mongo",
                                        "10gen","web","open","source","application","paas",
                                        "platform-as-a-service","technology","helps",
                                        "developers","focus","building","mongodb","mongo",
                                        "10gen","web","open","source","application","paas",
                                        "platform-as-a-service","technology","helps",
                                        "developers","focus","building","mongodb","mongo",
                                        "10gen","web","open","source","application","paas",
                                        "platform-as-a-service","technology","helps",
                                        "developers","focus","building","mongodb","mongo",
                                        "10gen","web","open","source","application","paas",
                                        "platform-as-a-service","technology","helps",
                                        "developers","focus","building","mongodb","mongo",
                                        "10gen","web","open","source","application","paas",
                                        "platform-as-a-service","technology","helps",
                                        "developers","focus","building","mongodb","mongo",
                                        "10gen","web","open","source","application","paas",
                                        "platform-as-a-service","technology","helps",
                                        "developers","focus","building","mongodb","mongo"
                                        )
             );

none($db->selectCollection("small_none"), $small, 500000);
none($db->selectCollection("medium_none"), $medium, 500000);
none($db->selectCollection("large_none"), $large, 100000);

none($db->selectCollection("small_none"), $small, 500000, true);
none($db->selectCollection("medium_none"), $medium, 500000, true);
none($db->selectCollection("large_none"), $large, 100000, true);



index($db->selectCollection("small_index"), $small, 750000);
index($db->selectCollection("medium_index"), $medium, 500000);
index($db->selectCollection("large_index"), $large, 100000);

index($db->selectCollection("small_index"), $small, 500000, true);
index($db->selectCollection("medium_index"), $medium, 500000, true);
index($db->selectCollection("large_index"), $large, 100000, true);



batch($db->selectCollection("small_none"), $small, 10000);
batch($db->selectCollection("medium_none"), $medium, 10000);
batch($db->selectCollection("large_none"), $large, 10000);


find_one($db->selectCollection("small_none"), 1000);
find_one($db->selectCollection("medium_none"), 1000);
find_one($db->selectCollection("large_none"), 1000);

find_one($db->selectCollection("small_index"), 10000);
find_one($db->selectCollection("medium_index"), 10000);
find_one($db->selectCollection("large_index"), 10000);

find($db->selectCollection("small_none"), 1000);
find($db->selectCollection("medium_none"), 1000);
find($db->selectCollection("large_none"), 1000);

find($db->selectCollection("small_index"), 10000);
find($db->selectCollection("medium_index"), 10000);
find($db->selectCollection("large_index"), 10000);

find_range($db->selectCollection("small_index"), 10000);
find_range($db->selectCollection("medium_index"), 10000);

//segfaults
find_range($db->selectCollection("large_index"), 10000);

?>
