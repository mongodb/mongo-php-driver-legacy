<?php

define( "PROFILING_OFF", 0 );
define( "PROFILING_SLOW", 1 );
define( "PROFILING_ON", 2 );

class MongoDB {

  var $connection = NULL;
  var $name = NULL;

  public function __construct( Mongo $conn, $name ) {
    $this->connection = $conn->connection;
    $this->name = $name;
  }

  public function __toString() {
    return $this->name;
  }


  /**
   * Get an authenticated session.
   * @param string $username 
   * @param string $password
   * @return MongoAuth if login was successful, false if unsuccessful
   */
  public function getAuth( $username, $password ) {
    return MongoAuth::getAuth( $this->connection, $this->name, $username, $password );
  }

  /**
   * Fetches toolkit for dealing with files stored in this database.
   * @param string $prefix name of the file collection
   * @return gridfs a new gridfs object for this database
   */
  public function getGridfs( $prefix = "fs" ) {
    return new MongoGridfs( $this, $prefix );
  }

  /** 
   * Returns the name of the database currently in use.
   * @return string the name of the database
   */
  public function getName() {
    return $this->name;
  }

  /**
   * Gets this database's profiling level.
   * @return int the profiling level
   */
  public function getProfilingLevel() {
    $data = array( MongoUtil::$PROFILE => -1 );
    $x = MongoUtil::dbCommand( $this->connection, $data, $this->name );
    if( $x[ "ok" ] == 1 )
      return $x[ "was" ];
    else
      return false;
  }

  /**
   * Sets this database's profiling level.
   * @return int the old profiling level
   */
  public function setProfilingLevel( $level ) {
    $data = array( MongoUtil::$PROFILE => (int)$level );
    $x = MongoUtil::dbCommand( $this->connection, $data, $this->name );
    if( $x[ "ok" ] == 1 ) {
      return $x[ "was" ];
    }
    return false;
  }

  /**
   * Drops this database.
   * @return array db response
   */
  public function drop() {
    $data = array( MongoUtil::$DROP_DATABASE => $this->name );
    return MongoUtil::dbCommand( $this->connection, $data );
  }

  /**
   * Repairs and compacts this database.
   * @param bool $preserve_cloned_files if cloned files should be kept if the repair fails (optional)
   * @param bool $backup_original_files if original files should be backed up (optional)
   * @return array db response
   */
  function repair( $preserve_cloned_files = false, $backup_original_files = false ) {
    $data = array( MongoUtil::$REPAIR_DATABASE => 1 );
    if( $preserve_cloned_files )
      $data[ "preserveClonedFilesOnFailure" ] = true;
    if( $backup_original_files )
      $data[ "backupOriginalFiles" ] = true;
    return MongoUtil::dbCommand( $this->connection, $data, $this->name );
  }


  /** 
   * Gets a collection.
   * @param string $name the name of the collection
   */
  public function selectCollection( $name ) {
    return new MongoCollection( $this, $name );
  }

  /** 
   * Creates a collection.
   * @param string $name the name of the collection
   * @param bool $capped if the collection should be a fixed size
   * @param int $size if the collection is fixed size, its size in bytes
   * @param int $max if the collection is fixed size, the maximum 
   *     number of elements to store in the collection
   * @return Collection a collection object representing the new collection
   */
  public function createCollection( $name, $capped = false, $size = 0, $max = 0 ) {
    $data = array( MongoUtil::$CREATE_COLLECTION => $name );
    if( $capped && $size ) {
      $data[ "capped" ] = true;
      $data[ "size" ] = $size;
      if( $max )
        $data[ "max" ] = $max;
    }

    MongoUtil::dbCommand( $this->connection, $data );
    return new MongoCollection( $this, $name );
  }

  /**
   * Drops a collection.
   * @param mongo_collection $coll
   * @return array the db response
   */
  public function dropCollection( MongoCollection $coll ) {
    return $coll->drop();
  }

}

?>
