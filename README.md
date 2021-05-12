# hw3:
首先開始兩個thread function，分別為gesture UI mode和tilt angle detection mode，來完成RPC loop的操作。\
在RPC loop裡面接受RPC的指令，將LED1打開，代表進入gesture UI mode: \
在這個mode裡面依照手勢選擇threshold angle，選擇期間顯示目前的threshold angle在uLCD上，選擇完畢後按下userbottom，顯示選擇好的threshold angle在screen上，並publish選擇好的threshold angle給python端，
當python端接收到此訊息，則傳送rpc指令將LED1關閉，停止gesture UI mode並回到RPC loop等待下一個RPC指令。\
使用RPC指令開啟LED2，則進入tilt angle detection mode: \
一開始先閃爍LED3 5秒，代表正在測量參考的重力加速度值，之後開始偵測角度。\
將偵測到的角度顯示在screen和uLCD上，若偵測到超過設定好的threshold angle的角度，publish "overtilt"的告知訊息和偵測到的角度到python端，
當超過角度的次數超過5次，python端傳送RPC指令將LED2關閉，結束tilt angle detection mode並回到RPC loop等待下一個RPC指令。

# 執行結果:
執行mqtt:
![image](https://user-images.githubusercontent.com/79581724/117925010-3b9b5a00-b329-11eb-8f8f-102984a2bff7.png)
\
\
進入gesture UI mode->選擇threshold angle->按下userbottom:
![image](https://user-images.githubusercontent.com/79581724/117925385-bc5a5600-b329-11eb-89f4-42c12baa222a.png)
\
\
進入tilt angle detection mode->偵測角度:
![image](https://user-images.githubusercontent.com/79581724/117925491-e7dd4080-b329-11eb-8231-f3c4165c27bc.png)
![image](https://user-images.githubusercontent.com/79581724/117925524-f88db680-b329-11eb-8446-c2c82c533b7d.png)
\
\
若超過threshold angle->publish
