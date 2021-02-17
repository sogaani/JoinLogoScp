こちらはAvxSynth(AviSynthをLinuxにポーティングしたもの)で動作させています。
AviSynthがLinuxに対応したのに合わせて、[AviSynth+Linuxで動作するJoinLogoScp](https://github.com/tobitti0/JoinLogoScpTrialSetLinux)が公開されています。
今から導入する方は上記のものを使うのをおすすめします。

# 概要
join_logo_scpをLinuxで動作させたい

## ファイル構成

* docker              : join_logo_scp動作確認環境構築用Dockerfile
* logoframe           : 透過ロゴ表示区間検出 ver1.16（要AviSynth環境）
* chapter_exe         : 無音＆シーンチェンジ検索chapter_exeの改造版
* join_logo_scp       : ロゴと無音シーンチェンジを使ったCM自動カット位置情報作成
* join_logo_scp_trial : join_logo_scp動作確認用スクリプト
