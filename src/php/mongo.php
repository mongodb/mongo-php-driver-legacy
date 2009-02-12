<?php

class Mongo {

  var $connection = NULL;

  /** Creates a new database connection.
   * @param string $host the host name (optional)
   * @param int $port the db port (optional)
   */
  public function __construct( $host = NULL, $port = NULL ) {
    if( !$host ) {
      $host = get_cfg_var( "mongo.default_host" );
      if( !$host ) {
        trigger_error( "no hostname given and no default hostname", E_USER_ERROR );
      }
    }
    if( !$port ) {
      $port = get_cfg_var( "mongo.default_port" );
    }
    $auto_reconnect = MongoUtil::getConfig( "mongo.auto_reconnect" );

    $addr = "$host:$port";
    $this->connection = mongo_connect( $addr, $auto_reconnect );

    if( !$this->connection ) {
      trigger_error( "couldn't connect to mongo", E_USER_WARNING );
    }
  }

  public function __toString() {
    return $this->host . ":" . $this->port;
  }

  /**
   * Gets an authenticated session.
   * @param string $db name of the db to log in to
   * @param string $username
   * @param string $password
   */
  public function getAuth( $db, $username, $password ) {
    $db = $this->selectDB( $db );
    return $db->getAuth( $username, $password );
  }

  /** 
   * Gets a database.
   * @param string $dbname the database name
   * @return MongoDB a new db object
   */
  public function selectDB( $dbname = NULL ) {
    if( $dbname == NULL || $dbname == "" ) {
      trigger_error( "Invalid database name.", E_USER_WARNING );
      return false;
    }
    return new MongoDB( $this, $dbname );
  }

  /**
   * Drops a database.
   * @param MongoDB $db the database to drop
   * @return array db response
   */
  public function dropDB( MongoDB $db ) {
    return $db->drop();
  }

  /**
   * Repairs and compacts a database.
   * @param MongoDB $db the database to drop
   * @param bool $preserve_cloned_files if cloned files should be kept if the repair fails
   * @param bool $backup_original_files if original files should be backed up
   * @return array db response
   */
  public function repairDB( MongoDB $db, $preserve_cloned_files = false, $backup_original_files = false ) {
    return $db->repair( $preserve_cloned_files, $backup_original_files );
  }

  /**
   * Check if there was an error on the most recent db operation performed.
   * @return string the error, if there was one, or NULL
   */
  public function lastError() {
    $result = MongoUtil::dbCommand( $this->connection, array( MongoUtil::$LAST_ERROR => 1 ), MongoUtil::$ADMIN );
    return $result[ "err" ];
  }

  /**
   * Checks for the last error thrown during a database operation.
   * @return array the error and the number of operations ago it occured
   */
  public function prevError() {
    $result = MongoUtil::dbCommand( $this->connection, array( MongoUtil::$PREV_ERROR => 1 ), MongoUtil::$ADMIN );
    unset( $result[ "ok" ] );
    return $result;
  }

  /**
   * Clears any flagged errors on the connection.
   * @param bool if successful
   */
  public function resetError() {
    $result = MongoUtil::dbCommand( $this->connection, array( MongoUtil::$RESET_ERROR => 1 ), MongoUtil::$ADMIN );
    return (bool)$result[ "ok" ];
  }

  /**
   * Closes this database connection.
   * @return bool if the connection was successfully closed
   */
  public function close() {
    mongo_close( $this->connection );
  }

}

include_once "mongo_auth.php";
require_once "mongo_db.php";
require_once "mongo_collection.php";
require_once "mongo_cursor.php";
require_once "mongo_util.php";
require_once "gridfs.php";

?>
