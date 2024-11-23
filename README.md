# Artificial Sun
自然な明かりで朝起きれる小型デバイス

ここではファームウェアのArduinoスケッチと基板デザイン(KiCad)を公開しています
- プロジェクト: https://yasunori.jp/artificial-sun
- 外装部品: https://grabcad.com/library/artificial-sun-wake-up-light-for-dark-mornings-1
- 動画: https://youtu.be/eCHGJ3ehzJc

## 電子部品
必要な電子パーツはこちらのスプレッドシートにまとめてあります。すべて秋月電子で買えます。
 →[Artificial Sun 電子部品](https://docs.google.com/spreadsheets/d/1AGpVGOaxi01ax8kF4fcREE8NY4uzkjkZFQBg4IJ91os/edit?usp=sharing)

## 基板
私はPCBGOGOに発注しましたがどこでも大丈夫だと思います。5枚発注して（初回割ということもありましたが）送料込みでたった1200円でした。

## 基板実装
以下の写真を参考に実装してください。注意点としては
- 7セグ用の300Ω抵抗は一部のみ使い、それ以外は短絡してください（7セグの向きを後から変えられるようにこの実装になっています）。
- LEDの定格電流的には25Ωは全然許容範囲内なのですが、抵抗の方の発熱が大きかったので、50Ω抵抗を並列で接続して放熱板も取り付けることで発熱を抑えています。（もちろんちゃんとしたLEDドライバを使えばいいのですが、初めての基板設計ということもあり簡単な回路を優先しました）
- Arduino nanoが基板に挿さったままだとスケッチが書き込めない場合があります。
- LEDは10センチほどのワイヤで基板に接続してください。曲げやすいワイヤの方が外装パーツに挿入しやすいです。
- M3、長さ12mmのナットとボルト4本で組み立てます。

![](/photos/all.jpg)
![](/photos/board_1.jpg)
![](/photos/board_2.jpg)
![](/photos/LED.jpg)
