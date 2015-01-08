/**
 *  Copyright 2013-2014 MongoDB, Inc.
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
 */
#ifndef __MONGO_CURSOR_H__
#define __MONGO_CURSOR_H__ 1

void mongo_init_MongoCommandCursor(TSRMLS_D);
zval *php_mongo_commandcursor_instantiate(zval *object TSRMLS_DC);
void mongo_command_cursor_init(mongo_command_cursor *cmd_cursor, char *ns, zval *zlink, zval *zcommand TSRMLS_DC);
int php_mongocommandcursor_is_valid(mongo_command_cursor *cmd_cursor);
int php_mongocommandcursor_load_current_element(mongo_command_cursor *cmd_cursor TSRMLS_DC);
int php_mongocommandcursor_advance(mongo_command_cursor *cmd_cursor TSRMLS_DC);
void php_mongocommandcursor_fetch_batch_if_first_is_empty(mongo_cursor *cmd_cursor TSRMLS_DC);
int php_mongo_validate_cursor_on_command(zval *command TSRMLS_DC);
int php_mongo_enforce_cursor_on_command(zval *command TSRMLS_DC);

void php_mongo_command_cursor_init_from_document(zval *zlink, mongo_command_cursor *cmd_cursor, char *hash, zval *document TSRMLS_DC);
#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: fdm=marker
 * vim: noet sw=4 ts=4
 */
