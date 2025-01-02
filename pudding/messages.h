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
// MessageId: ERROR_EXCEPTION
//
// MessageText:
//
// %1!hs!
//
#define ERROR_EXCEPTION                  ((LONG)0x80041000L)

