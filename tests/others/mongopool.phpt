--TEST--
MongoPool: Check that it is all deprecated
--FILE--
<?php
var_dump( Mongo::setPoolSize( 4 ) );
var_dump( Mongo::getPoolSize() );
var_dump( Mongo::poolDebug() );

var_dump( MongoPool::setSize( 4 ) );
var_dump( MongoPool::getSize() );
var_dump( MongoPool::info() );
--EXPECTF--
%s: Function Mongo::setPoolSize() is deprecated in %smongopool.php on line 2
int(1)

%s: Function Mongo::getPoolSize() is deprecated in %smongopool.php on line 3
int(1)

%s: Function Mongo::poolDebug() is deprecated in %smongopool.php on line 4
array(0) {
}

%s: Function MongoPool::setSize() is deprecated in %smongopool.php on line 6
int(1)

%s: Function MongoPool::getSize() is deprecated in %smongopool.php on line 7
int(1)

%s: Function MongoPool::info() is deprecated in %smongopool.php on line 8
array(0) {
}
