#pragma once

//
// https://learn.microsoft.com/ja-jp/windows/win32/eventlog/message-text-files
//

//
//  Values are 32 bit values laid out as follows:
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//  +-+-+-+-+-+---------------------+-------------------------------+
//  |S|R|C|N|r|    Facility         |               Code            |
//  +-+-+-+-+-+---------------------+-------------------------------+
//
//  where
//
//      S - Severity - indicates success/fail
//
//          0 - Success
//          1 - Fail (COERROR)
//
//      R - reserved portion of the facility code, corresponds to NT's
//              second severity bit.
//
//      C - reserved portion of the facility code, corresponds to NT's
//              C field.
//
//      N - reserved portion of the facility code. Used to indicate a
//              mapped NT status value.
//
//      r - reserved portion of the facility code. Reserved for internal
//              use. Used to indicate HRESULT values that are not status
//              values, but are instead message ids for display strings.
//
//      Facility - is the facility code
//
//      Code - is the facility's status code
//
//
// Define the facility codes
//


//
// Define the severity codes
//


//
// MessageId: ID_MENU_EXIT
//
// MessageText:
//
// %1 を終了%0
//
#define ID_MENU_EXIT                     ((LONG)0x00040100L)

//
// MessageId: ID_STATUS_CONSOLE
//
// MessageText:
//
// コンソールから %1 がログインしています。%0
//
#define ID_STATUS_CONSOLE                ((LONG)0x00041000L)

//
// MessageId: ID_STATUS_REMOTE_USER
//
// MessageText:
//
// リモートから %1 がログインしています。%0
//
#define ID_STATUS_REMOTE_USER            ((LONG)0x00041001L)

//
// MessageId: ID_STATUS_REMOTE_HOST
//
// MessageText:
//
// %1 からログインしています。%0
//
#define ID_STATUS_REMOTE_HOST            ((LONG)0x00041002L)

//
// MessageId: ID_LOGIN_CONSOLE
//
// MessageText:
//
// コンソールから %1 がログインしました。%0
//
#define ID_LOGIN_CONSOLE                 ((LONG)0x00041003L)

//
// MessageId: ID_LOGIN_REMOTE_USER
//
// MessageText:
//
// リモートから %1 がログインしました。%0
//
#define ID_LOGIN_REMOTE_USER             ((LONG)0x00041004L)

//
// MessageId: ID_LOGIN_REMOTE_HOST
//
// MessageText:
//
// %1 からログインしました。%0
//
#define ID_LOGIN_REMOTE_HOST             ((LONG)0x00041005L)

//
// MessageId: ID_SESSION_MSG
//
// MessageText:
//
// %1(%2!d!) - Session %3!d!
//
#define ID_SESSION_MSG                   ((LONG)0x00041006L)

//
// MessageId: ID_WATCH_ADD
//
// MessageText:
//
// ファイル %1!.*s! が追加されました。
//
#define ID_WATCH_ADD                     ((LONG)0x00041100L)

//
// MessageId: ID_WATCH_REMOVE
//
// MessageText:
//
// ファイル %1!.*s! が削除されました。
//
#define ID_WATCH_REMOVE                  ((LONG)0x00041101L)

//
// MessageId: ID_WATCH_MODIFY
//
// MessageText:
//
// ファイル %1!.*s! が変更されました。
//
#define ID_WATCH_MODIFY                  ((LONG)0x00041102L)

//
// MessageId: ID_WATCH_RENAME_OLD
//
// MessageText:
//
// ファイル %1!.*s! の名前が変更されました。
//
#define ID_WATCH_RENAME_OLD              ((LONG)0x00041103L)

//
// MessageId: ID_WATCH_RENAME_NEW
//
// MessageText:
//
// ファイル %1!.*s! へ名前が変更されました。
//
#define ID_WATCH_RENAME_NEW              ((LONG)0x00041104L)

//
// MessageId: ID_COMMAND_EXIT
//
// MessageText:
//
// %1 %2
// コマンドは終了コード %3!d! で終了しました。
//
#define ID_COMMAND_EXIT                  ((LONG)0x00041200L)

//
// MessageId: ERROR_STD_EXCEPTION
//
// MessageText:
//
// %1!hs!
//
#define ERROR_STD_EXCEPTION              ((LONG)0x80041000L)

//
// MessageId: ERROR_SYS_EXCEPTION
//
// MessageText:
//
// %1!hs! (%2!#08x!)
//
#define ERROR_SYS_EXCEPTION              ((LONG)0x80041001L)

