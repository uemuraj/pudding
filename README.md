# Pudding

`pudding` は Windows ログオンセッションとディレクトリを監視し、イベントに応じて任意のコマンドを自動実行するユーティリティです。

## 特徴

- Windowsのセッションイベント（ログオン/ログオフ/ロック/アンロック/リモート接続等）を検知
- イベントごとにコマンドやスクリプトを自動実行
- pudding.ini による柔軟な設定
- 設定ファイルやディレクトリの変更を自動監視・即時反映
- タスクトレイアイコンによる状態表示・操作
- custard など外部アプリケーションとの連携も可能

## インストール・配置

1. `pudding.exe` を任意のディレクトリに配置します。
2. 同じディレクトリに `pudding.ini`（設定ファイル）を置きます。

## 設定ファイル（pudding.ini）の例

```ini
[WTS_SESSION_LOGON]
CommandLine = cmd.exe /c echo %DATE% %TIME% WTS_SESSION_LOGON >> pudding.log

[WTS_SESSION_LOGOFF]
CommandLine = cmd.exe /c echo %DATE% %TIME% WTS_SESSION_LOGOFF >> pudding.log

[WTS_SESSION_LOCK]
CommandLine = custard.exe "xoxb-your-slack-token" "#general" "セッションがロックされました"
WorkDirectory = %USERPROFILE%\Desktop
```

- `CommandLine` … 実行するコマンド（引数含む）
- `WorkDirectory` … コマンドの作業ディレクトリ（省略可）

## 利用方法

1. pudding.exe を起動します。
2. タスクトレイにアイコンが表示されます。
3. Windowsのセッションイベント発生時、pudding.ini の該当セクションに記載されたコマンドが自動実行されます。
4. pudding.ini を編集すると、ファイル監視により自動で再読み込みされます。

## custard について

`custard` は Slack 通知などの外部サービス連携を簡単に行える Windows アプリケーションです。

### 特徴

- Slack API を利用したメッセージ送信機能（Botトークン・チャンネル・メッセージ指定）
- pudding など他のアプリケーションからコマンドラインで呼び出し可能
- C++20 標準ライブラリベースの軽量実装

### 利用方法

1. `custard.exe` を任意のディレクトリに配置します。
2. コマンドラインから以下のように実行します。

   ```
   custard.exe "xoxb-your-slack-token" "#general" "通知したいメッセージ"
   ```

   - `"xoxb-your-slack-token"` … Slack Botのトークン
   - `"#general"` … 通知先チャンネル
   - `"通知したいメッセージ"` … 投稿するメッセージ

3. pudding の `pudding.ini` の `CommandLine` から custard.exe を呼び出すことで、セッションイベント等をSlack通知できます。

## Pudding と custard の連携

### 利用例（pudding.ini）

Slack通知などを行いたい場合に `custard.exe` を pudding.ini の `CommandLine` に記述します。

```ini
[WTS_REMOTE_CONNECT]
CommandLine = custard.exe "xoxb-your-slack-token" "#general" "リモート接続: ユーザー=%PUDDING_REMOTE_USER% ホスト=%PUDDING_REMOTE_HOST%"
```

```ini
[WTS_SESSION_UNLOCK]
CommandLine = custard.exe "xoxb-your-slack-token" "#general" "セッションがロック解除されました"
```

## Pudding 固有の環境変数

pudding はセッションイベント発生時にリモートアクセス情報を環境変数として設定します。
コマンド実行時や custard など外部ツールで `%PUDDING_REMOTE_USER%` や `%PUDDING_REMOTE_HOST%` を利用できます。

| 環境変数名              | 内容                       |
|------------------------|----------------------------|
| PUDDING_REMOTE_USER    | リモート接続元のユーザー名 |
| PUDDING_REMOTE_HOST    | リモート接続元のホスト名   |

## ビルド方法

Visual Studio 2022 以降でソリューションを開き、ビルドしてください。  
