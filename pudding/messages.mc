;#pragma once
;
;//
;// https://learn.microsoft.com/ja-jp/windows/win32/eventlog/message-text-files
;//
;

MessageIdTypedef = LONG
SeverityNames = (SEVERITY_SUCCESS=0x0 SEVERITY_ERROR=0x2)
FacilityNames = (FACILITY_ITF=0x4)
LanguageNames = (Japanese=0x11:messages_011)

MessageId=0x0100
Severity=SEVERITY_SUCCESS
Facility=FACILITY_ITF
SymbolicName=ID_MENU_EXIT
Language=Japanese
%1 ���I��%0
.

MessageId=0x1000
Severity=SEVERITY_SUCCESS
Facility=FACILITY_ITF
SymbolicName=ID_STATUS_CONSOLE
Language=Japanese
�R���\�[������ %1 �����O�C�����Ă��܂��B%0
.

MessageId=
SymbolicName=ID_STATUS_REMOTE_USER
Language=Japanese
�����[�g���� %1 �����O�C�����Ă��܂��B%0
.

MessageId=
SymbolicName=ID_STATUS_REMOTE_HOST
Language=Japanese
%1 ���烍�O�C�����Ă��܂��B%0
.

MessageId=
SymbolicName=ID_LOGIN_CONSOLE
Language=Japanese
�R���\�[������ %1 �����O�C�����܂����B%0
.

MessageId=
SymbolicName=ID_LOGIN_REMOTE_USER
Language=Japanese
�����[�g���� %1 �����O�C�����܂����B%0
.

MessageId=
SymbolicName=ID_LOGIN_REMOTE_HOST
Language=Japanese
%1 ���烍�O�C�����܂����B%0
.

MessageId=0x1000
Severity=SEVERITY_ERROR
Facility=FACILITY_ITF
SymbolicName=ERROR_EXCEPTION
Language=Japanese
%1!hs!
.
