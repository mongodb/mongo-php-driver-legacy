<?php
/**
 *  Copyright 2009 10gen, Inc.
 * 
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 * 
 *  http://www.apache.org/licenses/LICENSE-2.0
 * 
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 * PHP version 5 
 *
 * @category Database
 * @package  Mongo
 * @author   Kristina Chodorow <kristina@10gen.com>
 * @license  http://www.apache.org/licenses/LICENSE-2.0  Apache License 2
 * @version  CVS: 000000
 * @link     http://www.mongodb.org
 */

require_once "Mongo/Util.php";

/**
 * Gets an authenticated database connection.
 *
 * A typical usage would be:
 * <code>
 * // initial login
 * $auth_connection = new MongoAuth();
 * $auth->login("mydb", "joe", "mypass");
 * if (!$auth_connection->loggedIn) {
 *    return $auth_connection->error;
 * }
 * setcookie("username", "joe");
 * setcookie("password", MongoAuth::getHash("joe", "mypass"));
 * </code>
 *
 * Then, for subsequent sessions, the cookies can be used to log in:
 * <code>
 * $username = $_COOKIE['username'];
 * $password = $_COOKIE['password'];
 * $auth_connection = new MongoAuth();
 * $auth_connection->login("mydb", $username, $password, false);
 * </code>
 *
 * @category Database
 * @package  Mongo
 * @author   Kristina Chodorow <kristina@10gen.com>
 * @license  http://www.apache.org/licenses/LICENSE-2.0  Apache License 2
 * @link     http://www.mongodb.org
 */
class MongoAuth extends Mongo
{

    public $db;

    /**
     * Communicates with the database to log in a user.
     * 
     * @param MongoDB $db       database
     * @param string  $username username
     * @param string  $pwd      plaintext password
     *
     * @return array the database response
     */
    protected static function getUser($db, $username, $pwd) 
    {
        $ns = "${db}.system.users";

        // get the nonce
        $result = $db->command(array(MongoUtil::NONCE => 1 ));
        if (!$result["ok"]) {
            return $result;
        }
        $nonce = $result[ "nonce" ];

        // create a digest of nonce/username/pwd
        $digest = md5($nonce . $username . $pwd);
        $data   = array(MongoUtil::AUTHENTICATE => 1, 
                        "user" => $username, 
                        "nonce" => $nonce,
                        "key" => $digest);

        // send everything to the db and pray
        return $db->command($data);
    }

    /**
     * Creates a hashed string from the username and password.
     * This string can be passed to MongoAuth->__construct as the password
     * with $plaintext set to false in order to login.
     *
     * @param string $username the username
     * @param string $password the password
     *
     * @return string the md5 hash of the username and password
     */
    public static function getHash($username, $password) 
    {
        return md5("${username}:mongo:${password}");
    }

    /**
     * Logs in to a given database.
     *
     * @param string $db        the name of the db
     * @param string $username  the username
     * @param string $password  the password
     * @param string $plaintext if the password is encrypted     
     *
     * @return bool if login was successful
     */
    public function login($db, 
                          $username, 
                          $password, 
                          $plaintext=true) 
    {
        $this->db = $this->selectDB((string)$db);

        if ($plaintext) {
            $hash = MongoAuth::getHash($username, $password);
        } else {
            $hash = $password;
        }

        $result = MongoAuth::getUser($this->db, $username, $hash);

        if ($result['ok'] != 1) {
            $this->error = 'couldn\'t log in';
            $this->code  = -3;
        }
        return $this->loggedIn = (bool)$result['ok'];
    }


    /**
     * Creates a new user.
     * This will not overwrite existing users, use MongoAuth::changePassword
     * to change a user's password.
     *
     * @param string $username the new user's username
     * @param string $password the new user's password
     * 
     * @return boolean if the new user was successfully created
     */
    public function addUser($username, $password) 
    {
        $c      = $this->db->selectCollection('system.users');
        $exists = $c->findOne(array('user' => $username));
        if ($exists) {
            return false;
        }
        $newUser = array('user' => $username,
                         'pwd' => MongoAuth::getHash($username, $password));
        $c->insert($newUser);
        return true;
    }



    /**
     * Changes a user's password.
     *
     * @param string $username the username
     * @param string $oldpass  the old password
     * @param string $newpass  the new password
     *
     * @return array whether the change was successful
     */
    public function changePassword($username, $oldpass, $newpass) 
    {
        $c    = $this->db->selectCollection('system.users');
        $user = $c->findOne(array('user' => $username));
        if (!$user) {
            return array('ok' => -2.0,
                         'errmsg' => 'no user with username $username found');
        }
        if ($user['pwd'] == MongoAuth::getHash($username, $oldpass)) {
            $user['pwd'] = MongoAuth::getHash($username, $newpass);
            $c->update(array('user'=>$username), $user);
            return array('ok' => 1.0);
        }
        return array('ok' => -1.0,
                     'errmsg' => 'incorrect old password');
    }


    /**
     * Delete a user.
     *
     * @param string $username the user to delete
     *
     * @return boolean if the user was deleted
     */
    public function deleteUser($username) 
    {
        $c = $this->db->selectCollection('system.users');
        $b = $c->remove(array('user' => (string)$username), true);
        // force the database to do the remove asap
        $c->findOne();
        return $b;
    }


    /**
     * Ends authenticated session.
     *
     * @return array database response
     */
    public function logout() 
    {
        return $this->db->command(array(MongoUtil::LOGOUT => 1));
    }

}

