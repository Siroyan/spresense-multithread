# SPRESENSE MPキュー／マルチスレッド比較

SPRESENSEのコア間通信で、受信処理を専用スレッドへ分離する効果を確認する
テストです。SubCore1は100 msごとに連番をMainCoreへ送信します。MainCoreは
受信値をSDカードの`mp.csv`へ追記します。一方、アプリケーションの`loop()`は
毎回3秒間`delay()`します。

## 動作モード

`src/main/main.cpp`先頭の`USE_RECEIVER_THREAD`で動作を切り替えます。

| 設定 | 受信とSD書込み | 期待する結果 |
|---|---|---|
| `0` | `loop()`で実行 | 1件受信後に3秒停止し、受信バッファが詰まってKernel Panicになる |
| `1` | 専用pthreadで実行 | `loop()`の停止中も受信を続け、連続動作する |

スレッド有効時の受信pthreadは優先度120です。優先度100のArduino `loop()`より
先に動作できるため、`delay()`のビジーループ中でも受信を継続できます。

## Serial Monitor

通信速度は`platformio.ini`で115200 bpsに設定されています。起動直後に
選択されているマクロ値を一度表示します。この表示は結果判定ではなく、実行中の
設定を確認するためのものです。

```text
USE_RECEIVER_THREAD=1
```

その後は、受信するたびに時刻、受信値、SD書込みを表示し、3秒ごとに`loop()`の
状態を表示します。

スレッド有効時は、`loop()`の表示間にも受信ログが連続します。

```text
[    1200 ms] recv=10 -> SD
[    1300 ms] recv=11 -> SD
[    1400 ms] recv=12 -> SD
[    3000 ms] loop delay 3000 ms
```

スレッド無効時は、受信ログが1件出た後に`loop()`が3秒停止します。その間に
SubCore1からのメッセージがたまり、Kernel Panicが発生します。

```text
[     100 ms] recv=0 -> SD
[     100 ms] loop delay 3000 ms
_assert: Assertion failed panic: ...
```

## SDカードへの記録

起動時に既存の`mp.csv`を削除し、受信した連番を1行ずつ保存します。値0では
flushせず、100、200、300のように100件受信するごとにflushします。送信周期が
100 msなので、約10秒ごとにSDカードへ同期します。

## ビルドと書込み

SubCore1、MainCoreの順でビルド・書込みを行います。

```sh
pio run -e spresense_subCore1 -t upload
pio run -e spresense_mainCore -t upload
```

その後、MainCoreのSerial Monitorを開きます。

```sh
pio device monitor -e spresense_mainCore
```

モードを変更した場合はMainCoreを再ビルド・再書込みしてください。送信周期を
変更した場合はSubCore1も再書込みする必要があります。

## 注意点

この例が分離するのは、`loop()`の`delay()`と受信処理です。SDカードへの書込み
自体が長時間停止すると、その間は専用pthreadも次のメッセージを受信できません。
SDの長時間停止まで吸収する場合は、受信専用スレッドとSD書込みスレッドの間に
RAM上のリングバッファが必要です。
