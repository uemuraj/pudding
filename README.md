# Pudding

`pudding` �� Windows ���O�I���Z�b�V�����ƃf�B���N�g�����Ď����A�C�x���g�ɉ����ĔC�ӂ̃R�}���h���������s���郆�[�e�B���e�B�ł��B

## ����

- Windows�̃Z�b�V�����C�x���g�i���O�I��/���O�I�t/���b�N/�A�����b�N/�����[�g�ڑ����j�����m
- �C�x���g���ƂɃR�}���h��X�N���v�g���������s
- pudding.ini �ɂ��_��Ȑݒ�
- �ݒ�t�@�C����f�B���N�g���̕ύX�������Ď��E�������f
- �^�X�N�g���C�A�C�R���ɂ���ԕ\���E����
- custard �ȂǊO���A�v���P�[�V�����Ƃ̘A�g���\

## �C���X�g�[���E�z�u

1. `pudding.exe` ��C�ӂ̃f�B���N�g���ɔz�u���܂��B
2. �����f�B���N�g���� `pudding.ini`�i�ݒ�t�@�C���j��u���܂��B

## �ݒ�t�@�C���ipudding.ini�j�̗�

```ini
[WTS_SESSION_LOGON]
CommandLine = cmd.exe /c echo %DATE% %TIME% WTS_SESSION_LOGON >> pudding.log

[WTS_SESSION_LOGOFF]
CommandLine = cmd.exe /c echo %DATE% %TIME% WTS_SESSION_LOGOFF >> pudding.log

[WTS_SESSION_LOCK]
CommandLine = custard.exe "xoxb-your-slack-token" "#general" "�Z�b�V���������b�N����܂���"
WorkDirectory = %USERPROFILE%\Desktop
```

- `CommandLine` �c ���s����R�}���h�i�����܂ށj
- `WorkDirectory` �c �R�}���h�̍�ƃf�B���N�g���i�ȗ��j

## ���p���@

1. pudding.exe ���N�����܂��B
2. �^�X�N�g���C�ɃA�C�R�����\������܂��B
3. Windows�̃Z�b�V�����C�x���g�������Apudding.ini �̊Y���Z�N�V�����ɋL�ڂ��ꂽ�R�}���h���������s����܂��B
4. pudding.ini ��ҏW����ƁA�t�@�C���Ď��ɂ�莩���ōēǂݍ��݂���܂��B

## custard �ɂ���

`custard` �� Slack �ʒm�Ȃǂ̊O���T�[�r�X�A�g���ȒP�ɍs���� Windows �A�v���P�[�V�����ł��B

### ����

- Slack API �𗘗p�������b�Z�[�W���M�@�\�iBot�g�[�N���E�`�����l���E���b�Z�[�W�w��j
- pudding �ȂǑ��̃A�v���P�[�V��������R�}���h���C���ŌĂяo���\
- C++20 �W�����C�u�����x�[�X�̌y�ʎ���

### ���p���@

1. `custard.exe` ��C�ӂ̃f�B���N�g���ɔz�u���܂��B
2. �R�}���h���C������ȉ��̂悤�Ɏ��s���܂��B

   ```
   custard.exe "xoxb-your-slack-token" "#general" "�ʒm���������b�Z�[�W"
   ```

   - `"xoxb-your-slack-token"` �c Slack Bot�̃g�[�N��
   - `"#general"` �c �ʒm��`�����l��
   - `"�ʒm���������b�Z�[�W"` �c ���e���郁�b�Z�[�W

3. pudding �� `pudding.ini` �� `CommandLine` ���� custard.exe ���Ăяo�����ƂŁA�Z�b�V�����C�x���g����Slack�ʒm�ł��܂��B

## Pudding �� custard �̘A�g

### ���p��ipudding.ini�j

Slack�ʒm�Ȃǂ��s�������ꍇ�� `custard.exe` �� pudding.ini �� `CommandLine` �ɋL�q���܂��B

```ini
[WTS_REMOTE_CONNECT]
CommandLine = custard.exe "xoxb-your-slack-token" "#general" "�����[�g�ڑ�: ���[�U�[=%PUDDING_REMOTE_USER% �z�X�g=%PUDDING_REMOTE_HOST%"
```

```ini
[WTS_SESSION_UNLOCK]
CommandLine = custard.exe "xoxb-your-slack-token" "#general" "�Z�b�V���������b�N��������܂���"
```

## Pudding �ŗL�̊��ϐ�

pudding �̓Z�b�V�����C�x���g�������Ƀ����[�g�A�N�Z�X�������ϐ��Ƃ��Đݒ肵�܂��B
�R�}���h���s���� custard �ȂǊO���c�[���� `%PUDDING_REMOTE_USER%` �� `%PUDDING_REMOTE_HOST%` �𗘗p�ł��܂��B

| ���ϐ���              | ���e                       |
|------------------------|----------------------------|
| PUDDING_REMOTE_USER    | �����[�g�ڑ����̃��[�U�[�� |
| PUDDING_REMOTE_HOST    | �����[�g�ڑ����̃z�X�g��   |

## �r���h���@

Visual Studio 2022 �ȍ~�Ń\�����[�V�������J���A�r���h���Ă��������B  
