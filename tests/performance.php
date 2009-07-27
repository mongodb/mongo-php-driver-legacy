<?php

define('PER_TRIAL', 5000);
define('BATCH_SIZE', 100);

$m  = new Mongo();
$db = $m->selectDB("test");

function micro_time()
{
  list($usec, $sec) = explode(" ", microtime());
  return (float)$usec + (float)$sec;
}

function none($c, $obj)
{
  $c->drop();
  go($c, $obj);

  $c->drop();
  go_find_one($c, $obj);
}

function index($c, $obj)
{
  $c->drop();
  $c->ensureIndex("x");
  go($c, $obj);

  $c->drop();
  $c->ensureIndex("x");
  go_find_one($c, $obj);
}

function batch($c, $obj)
{
  echo "{batch insert} $c\n";

  $c->drop();

  $batches = array();
  for($i=0;$i < PER_TRIAL;$i++){
    $batch = array();
    for($j=0;$j<BATCH_SIZE;$j++){
      $obj["x"]=$i;
      $batch[] = $obj;
      $i++;
    }
    $batches[] = $batch;
  }
  $num_batches = count($batches);

  $start = micro_time();
  for($i=0;$i < $num_batches;$i++){
    $c->batchInsert( $batches[$i] );
  }
  $c->findOne();
  $end = micro_time();

  $total = $end - $start;
  $ops = PER_TRIAL/$total;
  echo "total time: $total\n";
  echo "ops/sec: $ops\n";
  echo "------\n";
}

function go($c, $obj)
{
  echo "{insert} $c\n";

  $start = micro_time();
  for($i=0;$i < PER_TRIAL;$i++){
    $obj["x"]=$i;
    $c->insert( $obj );
  }
  $end = micro_time();
  $total = $end - $start;
  $ops = PER_TRIAL/$total;
  echo "total time: $total\n";
  echo "ops/sec: $ops\n";
  echo "------\n";
}

function go_find_one($c, $obj)
{
  echo "{insert/findOne} $c\n";

  $start = micro_time();
  for($i=0;$i < PER_TRIAL;$i++){
    $obj["x"]=$i;
    $c->insert( $obj );
  }
  $x=$c->findOne();

  $end = micro_time();
  $total = $end - $start;
  $ops = PER_TRIAL/$total;
  echo "total time: $total\n";
  echo "ops/sec: $ops\n";
  echo "------\n";
}


function find_one($c)
{
  echo "find_one $c\n";

  $query = array("x" => PER_TRIAL/2);
  $start = micro_time();
  for($i=0;$i<PER_TRIAL;$i++){
    $c->findOne( $query );
  }
  $end = micro_time();
  $total = $end - $start;
  $ops = PER_TRIAL/$total;
  echo "total time: $total\n";
  echo "ops/sec: $ops\n";
  echo "------\n";
}


function find($c)
{
  echo "{find} $c\n";

  $query = array("x" => PER_TRIAL/2);
  $start = micro_time();
  for($i=0;$i<PER_TRIAL;$i++){
    $cursor = $c->find( $query );
    while( $cursor->hasNext() ){
      $cursor->next();
    }
  }
  $end = micro_time();
  $total = $end - $start;
  $ops = PER_TRIAL/$total;
  echo "total time: $total\n";
  echo "ops/sec: $ops\n";
  echo "------\n";
}

function find_range($c)
{
  echo "{find_range} $c\n";

  $query = array("x" => array('$gt' => PER_TRIAL/2,
                              '$lt' => PER_TRIAL/2+BATCH_SIZE));

  $start = micro_time();
  for($i=0;$i<PER_TRIAL;$i++){
    $cursor = $c->find( $query );
    while( $cursor->hasNext() ){
      $cursor->next();
    }
  }
  $end = micro_time();
  $total = $end - $start;
  $ops = PER_TRIAL/$total;
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

batch($db->selectCollection("small_none"), $small);
batch($db->selectCollection("medium_none"), $medium);
batch($db->selectCollection("large_none"), $large);

none($db->selectCollection("small_none"), $small);
none($db->selectCollection("medium_none"), $medium);
none($db->selectCollection("large_none"), $large);


index($db->selectCollection("small_index"), $small);
index($db->selectCollection("medium_index"), $medium);
index($db->selectCollection("large_index"), $large);


find_one($db->selectCollection("small_none"));
find_one($db->selectCollection("medium_none"));
find_one($db->selectCollection("large_none"));

find_one($db->selectCollection("small_index"));
find_one($db->selectCollection("medium_index"));
find_one($db->selectCollection("large_index"));

find($db->selectCollection("small_none"));
find($db->selectCollection("medium_none"));
find($db->selectCollection("large_none"));

find($db->selectCollection("small_index"));
find($db->selectCollection("medium_index"));
find($db->selectCollection("large_index"));

find_range($db->selectCollection("small_index"));
find_range($db->selectCollection("medium_index"));
find_range($db->selectCollection("large_index"));

?>
