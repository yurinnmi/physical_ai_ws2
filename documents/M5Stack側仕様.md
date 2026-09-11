・起動直後から時計表示
・TIME,HH:MM:SS 受信で時刻補正 (ex. TIME,13:19:25)
・PERSON,1 で PERSON / DETECTED
・PERSON,0 で時計へ復帰

・SG92Rサーボ (ENABLE_SG92R_SERVO で有効・無効を切り替え)
　・時計表示中: 120度
　・PERSON DETECTED表示中: 30度

・複数物体検出表示 (ENABLE_MULTI_OBJECT_DISPLAY で有効・無効を切り替え。無効時は従来のPERSON,0/1表示)
　・teddy bear / cup / bottle をそれぞれ TEDDY_BEAR,0/1 ・ CUP,0/1 ・ BOTTLE,0/1 で受信
　・検出中のものをすべて画面に一覧表示（複数同時検出可）
　・すべて未検出になったら時計表示に戻る
　・ENABLE_SG92R_SERVO有効時、人物検出時と同じ角度でサーボも連動（検出中: 30度、未検出: 120度）

