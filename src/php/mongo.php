<?php

dl("libmongo.so");

include "src/php/mongo_auth.php";

class mongo {

  var $connection = NULL;

  /** Creates a new database connection.
   * @param string $host the host name (optional)
   * @param int $port the db port (optional)
   */
  public function __construct( $host = NULL, $port = NULL ) {
    if( !$host ) {
      $host = get_cfg_var( "mongo.default_host" );
    }
    if( !$port ) {
      $port = get_cfg_var( "mongo.default_port" );
    }
    $auto_reconnect = get_cfg_var( "mongo.auto_reconnect" );

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
  public function get_auth( $db, $username, $password ) {
    $db = $this->select_db( $db );
    return $db->get_auth( $username, $password );
  }

  /** 
   * Gets a database.
   * @param string $dbname the database name
   * @return mongo_db a new db object
   */
  public function select_db( $dbname = NULL ) {
    if( $dbname == NULL || $dbname == "" ) {
      trigger_error( "Invalid database name.", E_USER_WARNING );
      return false;
    }
    return new mongo_db( $this, $dbname );
  }

  /**
   * Drops a database.
   * @param mongo_db $db the database to drop
   * @return array db response
   */
  public function drop_db( mongo_db $db ) {
    return $db->drop();
  }

  /**
   * Repairs and compacts a database.
   * @param mongo_db $db the database to drop
   * @param bool $preserve_cloned_files if cloned files should be kept if the repair fails
   * @param bool $backup_original_files if original files should be backed up
   * @return array db response
   */
  public function repair_db( mongo_db $db, $preserve_cloned_files = false, $backup_original_files = false ) {
    return $db->repair( $preserve_cloned_files, $backup_original_files );
  }

  /**
   * Check if there was an error on the most recent db operation performed.
   * @return string the error, if there was one, or NULL
   */
  public function last_error() {
    $result = mongo_util::db_command( $this->connection, array( mongo_util::$LAST_ERROR => 1 ), mongo_util::$ADMIN );
    return $result[ "err" ];
  }

  /**
   * Checks for the last error thrown during a database operation.
   * @return array the error and the number of operations ago it occured
   */
  public function prev_error() {
    $result = mongo_util::db_command( $this->connection, array( mongo_util::$PREV_ERROR => 1 ), mongo_util::$ADMIN );
    unset( $result[ "ok" ] );
    return $result;
  }

  /**
   * Clears any flagged errors on the connection.
   * @param bool if successful
   */
  public function reset_error() {
    $result = mongo_util::db_command( $this->connection, array( mongo_util::$RESET_ERROR => 1 ), mongo_util::$ADMIN );
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

require "src/php/mongo_db.php";
require "src/php/mongo_collection.php";
require "src/php/mongo_cursor.php";
require "src/php/mongo_util.php";
require "src/php/gridfs.php";

?>
