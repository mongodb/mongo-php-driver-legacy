<html>
<head><title>Tutorial</title></head>
<body>
<?php

require_once "mongo.php";

$doc = array( "name" => "MongoDB",
              "type" => "database",
              "count" => 1,
              "info" => array( "x" => 203,
                               "y" => 102
                               )
              );

$m = new Mongo(); // connects to localhost:27017
$collection = $m->selectDB( "foo" )->selectCollection( "bar" );
$collection->insert( $doc );

$obj = $collection->findOne();
echo "findone: " . MongoUtil::toJSON( $obj ) . "<br/>";


for($i=0; $i<100; $i++) {
  $collection->insert( array( "i" => $i ) );
}

echo "count: " . $collection->count() . "<br/>";

echo "find:<br/>";
$cursor = $collection->find();
while( $cursor->hasNext() ) {
  $obj = $cursor->next();
  echo MongoUtil::toJSON( $obj ) . "<br/>";
}

$query = array( "i" => 71 );
$cursor = $collection->find( $query );

echo "find i=71:<br/>";
while( $cursor->hasNext() ) {
  echo MongoUtil::toJSON( $cursor->next() ) . "<br/>";
}

// note the escaped "$"
$query = array( "i" => array( "\$gt" => 50 ) );
$cursor = $collection->find( $query );

echo "find i>50:<br/>";
while( $cursor->hasNext() ) {
  echo MongoUtil::toJSON( $cursor->next() ) . "<br/>";
}

$collection->ensureIndex( array( "i" => 1 ) );  // create index on "i", ascending

$m->close();

?>
</body>
</html>
