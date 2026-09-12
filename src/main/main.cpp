#include <Arduino.h>
#include <File.h>
#include <MP.h>
#include <SDHCI.h>
#include <pthread.h>

// 0: loop()で受信、1: 専用スレッドで受信
#define USE_RECEIVER_THREAD 0

// SubCore1との通信とSD保存に使用する
constexpr int SUBCORE_ID = 1;
SDClass SD;
File logFile;

// SubCore1から1件受信し、SDカードへ保存する
void receiveAndWrite()
{
  int8_t id;
  uint32_t data;
  if (MP.Recv(&id, &data, SUBCORE_ID) >= 0) {
    logFile.println(data);
    Serial.printf("[%8llu ms] recv=%lu -> SD\n", millis(), data);
    // 初回の値0を除き、100件ごとにSDへ同期する
    if (data && data % 100 == 0) logFile.flush();
  }
}

// loop()とは独立して受信処理を繰り返す
void *receiver(void *) { while (true) receiveAndWrite(); return nullptr; }

void setup()
{
  // 使用中のモードを起動時に一度だけ表示する
  Serial.begin(115200);
  Serial.printf("USE_RECEIVER_THREAD=%d\n", USE_RECEIVER_THREAD);

  // SDカードをマウントし、ログファイルを作り直す
  while (!SD.begin()) delay(100);
  SD.remove("mp.csv");
  logFile = SD.open("mp.csv", FILE_WRITE);

  // SubCore1を起動し、初期化失敗時は停止する
  if (!logFile || MP.begin(SUBCORE_ID) < 0) while (true);
#if USE_RECEIVER_THREAD
  // loop()のビジーループより優先して受信できるよう優先度を120にする
  pthread_t thread; pthread_attr_t attr;
  sched_param param = {};
  pthread_attr_init(&attr);
  param.sched_priority = 120;
  pthread_attr_setschedparam(&attr, &param);
  if (pthread_create(&thread, &attr, receiver, nullptr)) while (true);
#endif
}

void loop()
{
#if !USE_RECEIVER_THREAD
  // スレッド無効時はloop()自身が受信する
  receiveAndWrite();
#endif
  Serial.printf("[%8llu ms] loop delay 3000 ms\n", millis());
  delay(3000); // アプリケーション側の重い処理を模擬する
}
